#include "Feature/HoleContextGeometryGrouper.h"

#include "Core/CollectionUtil.h"

#include "Geometry/TopologyQuery.h"
#include "Geometry/GeometryModel.h"
#include "Geometry/SurfaceUtil.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <vector>

#include <gp_Vec.hxx>

#include <BRep_Tool.hxx>
#include <TopAbs.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Vertex.hxx>

#include <QDebug>

namespace
{
    constexpr double AxisDirectionTolerance = 1.0e-6;
    constexpr double AxisDistanceTolerance = 1.0e-4;
    constexpr double RadiusTolerance = 1.0e-4;
    constexpr double AxialRangeTolerance = 1.0e-4;
    constexpr double PlaneNormalTolerance = 1.0e-6;
    constexpr double PlaneDistanceTolerance = 1.0e-4;

    constexpr double TwoPi = 6.28318530717958647692;
    constexpr double CircumferentialCoverageTolerance = 1.0e-3;

    double normalizeAngle(double angle)
    {
        while (angle < 0.0)
        {
            angle += TwoPi;
        }

        while (angle >= TwoPi)
        {
            angle -= TwoPi;
        }

        return angle;
    }

    bool containsFaceIndex(
        const std::vector<int>& faceIndices,
        int faceIndex)
    {
        return std::find(
                   faceIndices.begin(),
                   faceIndices.end(),
                   faceIndex) != faceIndices.end();
    }
}

namespace OccQtCore::Feature
{
    HoleContextGeometryGrouper::HoleContextGeometryGrouper(
        const GeometryModel& model)
        : m_model(model)
    {
    }

    std::vector<HoleContextGeometryGroup>
    HoleContextGeometryGrouper::group() const
    {
        std::vector<HoleContextGeometryGroup> groups;

        const auto cylindricalGroups = buildCylindricalGroups();

        for (const auto& cylindricalGroup : cylindricalGroups)
        {
            groups.push_back(cylindricalGroup);
        }

        for (int i = 0; i < static_cast<int>(groups.size()); ++i)
        {
            groups[i].index = i;
        }

        return groups;
    }

    std::vector<HoleContextGeometryGroup>
    HoleContextGeometryGrouper::groupFromGeometryRefs(
        const GeometryRefs& geometryRefs,
        const HoleContextGeometryGroup& parentGroup) const
    {
        for (const int faceIndex : geometryRefs.faceIndices)
        {
            const auto* face = m_model.faceAt(faceIndex);

            if (face == nullptr)
            {
                continue;
            }
        }

        std::vector<HoleContextGeometryGroup> groups;

        const auto planarGroups =
            buildPlanarGroupsFromFaces(
                geometryRefs.faceIndices,
                parentGroup);

        groups.insert(
            groups.end(),
            planarGroups.begin(),
            planarGroups.end());

        for (int i = 0; i < static_cast<int>(groups.size()); ++i)
        {
            groups[i].index = i;
        }

        return groups;
    }

    std::vector<HoleContextGeometryGroup>
    HoleContextGeometryGrouper::buildCylindricalGroups() const
    {
        std::vector<CylindricalGroup> workingGroups;

        for (const auto& face : m_model.faces())
        {
            if (face.info.kind != SurfaceKind::Cylinder)
            {
                continue;
            }

            if (!face.info.cylinder.has_value())
            {
                continue;
            }

            if (!isInnerCylindricalFace(face))
            {
                continue;
            }

            bool merged = false;

            for (auto& group : workingGroups)
            {
                if (!canMergeCylindricalFace(group, face.index))
                {
                    continue;
                }

                mergeCylindricalFace(group, face.index);
                merged = true;
                break;
            }

            if (!merged)
            {
                workingGroups.push_back(
                    createCylindricalGroup(
                        face.index,
                        static_cast<int>(workingGroups.size())));
            }
        }

        std::vector<HoleContextGeometryGroup> groups;

        for (const auto& workingGroup : workingGroups)
        {
            if (!isValidCylindricalGroup(workingGroup))
            {
                continue;
            }

            HoleContextGeometryGroup group = workingGroup.group;
            group.index = static_cast<int>(groups.size());
            group.kind = HoleContextGeometryGroupKind::Cylindrical;
            group.hasAxis = true;
            group.axialPosition =
                0.5 * (group.axialMin + group.axialMax);

            groups.push_back(group);
        }

        return groups;
    }

    bool HoleContextGeometryGrouper::canMergeCylindricalFace(
        const CylindricalGroup& group,
        int faceIndex) const
    {
        if (group.group.kind != HoleContextGeometryGroupKind::Cylindrical)
        {
            return false;
        }

        if (!group.group.hasAxis)
        {
            return false;
        }

        if (!TopologyQuery::isValidFaceIndex(m_model, faceIndex))
        {
            return false;
        }

        const auto* face = m_model.faceAt(faceIndex);

        if (face->info.kind != SurfaceKind::Cylinder)
        {
            return false;
        }

        if (!face->info.cylinder.has_value())
        {
            return false;
        }

        const auto& cylinder = face->info.cylinder.value();

        if (!SurfaceUtil::isSameCylinderAxisAndRadius(
                group.group.axisPoint,
                group.group.axisDirection,
                group.group.radius,
                cylinder.axis.Location(),
                cylinder.axis.Direction(),
                cylinder.radius,
                RadiusTolerance,
                AxisDistanceTolerance,
                AxisDirectionTolerance))
        {
            return false;
        }

        double faceAxialMin = 0.0;
        double faceAxialMax = 0.0;

        if (!computeFaceAxialRange(
                *face,
                group.group.axisPoint,
                group.group.axisDirection,
                faceAxialMin,
                faceAxialMax))
        {
            return false;
        }

        return isAxialRangeConnected(
            group.group.axialMin,
            group.group.axialMax,
            faceAxialMin,
            faceAxialMax);
    }

    void HoleContextGeometryGrouper::mergeCylindricalFace(
        CylindricalGroup& group,
        int faceIndex) const
    {
        if (!TopologyQuery::isValidFaceIndex(m_model, faceIndex))
        {
            return;
        }

        const auto* face = m_model.faceAt(faceIndex);

        CollectionUtil::addUnique(
            group.group.geometryRefs.faceIndices,
            faceIndex);

        CollectionUtil::addUnique(
            group.faceIndices,
            faceIndex);

        double faceAxialMin = 0.0;
        double faceAxialMax = 0.0;

        if (computeFaceAxialRange(
                *face,
                group.group.axisPoint,
                group.group.axisDirection,
                faceAxialMin,
                faceAxialMax))
        {
            group.group.axialMin =
                std::min(group.group.axialMin, faceAxialMin);

            group.group.axialMax =
                std::max(group.group.axialMax, faceAxialMax);

            group.group.axialPosition =
                0.5 * (group.group.axialMin + group.group.axialMax);
        }
    }

    HoleContextGeometryGrouper::CylindricalGroup
    HoleContextGeometryGrouper::createCylindricalGroup(
        int faceIndex,
        int groupIndex) const
    {
        CylindricalGroup group;

        group.group.index = groupIndex;
        group.group.kind = HoleContextGeometryGroupKind::Unknown;
        group.group.hasAxis = false;

        if (!TopologyQuery::isValidFaceIndex(m_model, faceIndex))
        {
            return group;
        }

        const auto* face = m_model.faceAt(faceIndex);

        if (face->info.kind != SurfaceKind::Cylinder)
        {
            return group;
        }

        if (!face->info.cylinder.has_value())
        {
            return group;
        }

        const auto& cylinder = face->info.cylinder.value();

        group.group.index = groupIndex;
        group.group.kind = HoleContextGeometryGroupKind::Cylindrical;

        group.group.geometryRefs.faceIndices.push_back(faceIndex);
        group.faceIndices.push_back(faceIndex);

        group.group.hasAxis = true;
        group.group.axisPoint = cylinder.axis.Location();
        group.group.axisDirection = cylinder.axis.Direction();
        group.group.radius = cylinder.radius;

        group.group.note =
            "Created as cylindrical hole-context geometry group.";

        double axialMin = 0.0;
        double axialMax = 0.0;

        if (computeFaceAxialRange(
                *face,
                group.group.axisPoint,
                group.group.axisDirection,
                axialMin,
                axialMax))
        {
            group.group.axialMin = axialMin;
            group.group.axialMax = axialMax;
            group.group.axialPosition = 0.5 * (axialMin + axialMax);
        }

        return group;
    }

    bool HoleContextGeometryGrouper::isValidCylindricalGroup(
        const CylindricalGroup& group) const
    {
        if (group.group.kind != HoleContextGeometryGroupKind::Cylindrical)
        {
            return false;
        }

        if (!group.group.hasAxis)
        {
            return false;
        }

        if (group.faceIndices.empty())
        {
            return false;
        }

        if (!hasFullCircumferentialCoverage(group))
        {
            return false;
        }

        if (!hasTopologicalCircumferentialLoop(group))
        {
            return false;
        }

        return true;
    }

    bool HoleContextGeometryGrouper::hasFullCircumferentialCoverage(
        const CylindricalGroup& group) const
    {
        std::vector<std::pair<double, double>> intervals;

        for (const int faceIndex : group.faceIndices)
        {
            if (!TopologyQuery::isValidFaceIndex(m_model, faceIndex))
            {
                continue;
            }

            const auto* face = m_model.faceAt(faceIndex);

            const double rawMin = face->info.uMin;
            const double rawMax = face->info.uMax;
            const double rawSpan =
                SurfaceUtil::parameterSpan(rawMin, rawMax);

            if (rawSpan >= TwoPi - CircumferentialCoverageTolerance)
            {
                return true;
            }

            const double uMin = normalizeAngle(rawMin);
            const double uMax = normalizeAngle(rawMax);

            if (uMin <= uMax)
            {
                intervals.emplace_back(uMin, uMax);
            }
            else
            {
                intervals.emplace_back(uMin, TwoPi);
                intervals.emplace_back(0.0, uMax);
            }
        }

        if (intervals.empty())
        {
            return false;
        }

        std::sort(
            intervals.begin(),
            intervals.end(),
            [](const auto& lhs, const auto& rhs)
            {
                return lhs.first < rhs.first;
            });

        double coverage = 0.0;

        double currentMin = intervals.front().first;
        double currentMax = intervals.front().second;

        for (size_t i = 1; i < intervals.size(); ++i)
        {
            const auto& interval = intervals[i];

            if (interval.first <=
                currentMax + CircumferentialCoverageTolerance)
            {
                currentMax = std::max(currentMax, interval.second);
            }
            else
            {
                coverage += currentMax - currentMin;
                currentMin = interval.first;
                currentMax = interval.second;
            }
        }

        coverage += currentMax - currentMin;

        return coverage >= TwoPi - CircumferentialCoverageTolerance;
    }

    bool HoleContextGeometryGrouper::hasTopologicalCircumferentialLoop(
        const CylindricalGroup& group) const
    {
        if (group.faceIndices.empty())
        {
            return false;
        }

        if (group.faceIndices.size() == 1)
        {
            return hasFullCircumferentialCoverage(group);
        }

        std::vector<std::set<int>> adjacency(group.faceIndices.size());

        for (size_t i = 0; i < group.faceIndices.size(); ++i)
        {
            const int faceIndex = group.faceIndices[i];

            if (!TopologyQuery::isValidFaceIndex(m_model, faceIndex))
            {
                continue;
            }

            const auto edgeIndices =
                TopologyQuery::edgesOfFace(m_model, faceIndex);

            for (const int edgeIndex : edgeIndices)
            {
                if (!isAxialEdgeOfCylinder(
                        edgeIndex,
                        group.group.axisDirection))
                {
                    continue;
                }

                const auto adjacentFaceIndices =
                    TopologyQuery::adjacentFacesOfEdge(
                        m_model,
                        edgeIndex,
                        faceIndex);

                for (const int adjacentFaceIndex : adjacentFaceIndices)
                {
                    if (!containsFaceIndex(
                            group.faceIndices,
                            adjacentFaceIndex))
                    {
                        continue;
                    }

                    const auto adjacentIt =
                        std::find(
                            group.faceIndices.begin(),
                            group.faceIndices.end(),
                            adjacentFaceIndex);

                    if (adjacentIt == group.faceIndices.end())
                    {
                        continue;
                    }

                    const size_t adjacentLocalIndex =
                        static_cast<size_t>(
                            std::distance(
                                group.faceIndices.begin(),
                                adjacentIt));

                    if (adjacentLocalIndex == i)
                    {
                        continue;
                    }

                    adjacency[i].insert(
                        static_cast<int>(adjacentLocalIndex));

                    adjacency[adjacentLocalIndex].insert(
                        static_cast<int>(i));
                }
            }
        }

        std::vector<bool> visited(group.faceIndices.size(), false);
        std::vector<int> stack;

        stack.push_back(0);
        visited[0] = true;

        while (!stack.empty())
        {
            const int current = stack.back();
            stack.pop_back();

            for (const int next : adjacency[static_cast<size_t>(current)])
            {
                if (visited[static_cast<size_t>(next)])
                {
                    continue;
                }

                visited[static_cast<size_t>(next)] = true;
                stack.push_back(next);
            }
        }

        return std::all_of(
            visited.begin(),
            visited.end(),
            [](bool value)
            {
                return value;
            });
    }

    bool HoleContextGeometryGrouper::isInnerCylindricalFace(
        const FaceData& face) const
    {
        if (face.info.kind != SurfaceKind::Cylinder)
        {
            return false;
        }

        if (!face.info.cylinder.has_value())
        {
            return false;
        }

        const auto& cylinder = face.info.cylinder.value();

        return SurfaceUtil::isCylinderFaceInwardOriented(
            face.shape,
            cylinder.axis,
            face.info.uMin,
            face.info.uMax,
            face.info.vMin,
            face.info.vMax);
    }

    bool HoleContextGeometryGrouper::isAxialEdgeOfCylinder(
        int edgeIndex,
        const gp_Dir& axisDirection) const
    {
        if (!TopologyQuery::isValidEdgeIndex(m_model, edgeIndex))
        {
            return false;
        }

        const auto* edge = m_model.edgeAt(edgeIndex);

        if (edge->info.kind != CurveKind::Line)
        {
            return false;
        }

        if (!edge->info.line.has_value())
        {
            return false;
        }

        const gp_Dir edgeDirection =
            edge->info.line.value().direction;

        return SurfaceUtil::isSameDirectionOrReverse(
            edgeDirection,
            axisDirection,
            AxisDirectionTolerance);
    }

    bool HoleContextGeometryGrouper::computeFaceAxialRange(
        const FaceData& face,
        const gp_Pnt& axisPoint,
        const gp_Dir& axisDirection,
        double& axialMin,
        double& axialMax) const
    {
        double minValue = std::numeric_limits<double>::max();
        double maxValue = -std::numeric_limits<double>::max();

        bool hasVertex = false;

        for (TopExp_Explorer explorer(face.shape, TopAbs_VERTEX);
             explorer.More();
             explorer.Next())
        {
            const TopoDS_Vertex vertex =
                TopoDS::Vertex(explorer.Current());

            const gp_Pnt point =
                BRep_Tool::Pnt(vertex);

            const double axial =
                SurfaceUtil::projectPointToAxis(
                    axisPoint,
                    axisDirection,
                    point);

            minValue = std::min(minValue, axial);
            maxValue = std::max(maxValue, axial);

            hasVertex = true;
        }

        if (!hasVertex)
        {
            axialMin = 0.0;
            axialMax = 0.0;
            return false;
        }

        axialMin = minValue;
        axialMax = maxValue;

        return true;
    }

    bool HoleContextGeometryGrouper::isAxialRangeConnected(
        double min1,
        double max1,
        double min2,
        double max2) const
    {
        if (max1 + AxialRangeTolerance < min2)
        {
            return false;
        }

        if (max2 + AxialRangeTolerance < min1)
        {
            return false;
        }

        return true;
    }

    std::vector<HoleContextGeometryGroup> HoleContextGeometryGrouper::buildPlanarGroupsFromFaces(
        const std::vector<int>& faceIndices,
        const HoleContextGeometryGroup& parentGroup) const
    {
        std::vector<HoleContextGeometryGroup> groups;

        for (const int faceIndex : faceIndices)
        {
            if (!TopologyQuery::isValidFaceIndex(m_model, faceIndex))
            {
                continue;
            }

            const auto* face = m_model.faceAt(faceIndex);

            if (face == nullptr)
            {
                continue;
            }

            if (face->info.kind != SurfaceKind::Plane)
            {
                continue;
            }

            if (!face->info.plane.has_value())
            {
                continue;
            }

            bool merged = false;

            for (auto& group : groups)
            {
                if (!canMergePlanarFace(group, faceIndex))
                {
                    continue;
                }

                mergePlanarFace(group, faceIndex);
                merged = true;
                break;
            }

            if (!merged)
            {
                groups.push_back(
                    createPlanarGroup(
                        faceIndex,
                        parentGroup));
            }
        }

        for (int i = 0; i < static_cast<int>(groups.size()); ++i)
        {
            groups[i].index = i;
        }

        return groups;
    }

    bool HoleContextGeometryGrouper::canMergePlanarFace(
        const HoleContextGeometryGroup& group,
        int faceIndex) const
    {
        if (group.kind != HoleContextGeometryGroupKind::Planar)
        {
            return false;
        }

        if (!group.hasAxis)
        {
            return false;
        }

        if (!TopologyQuery::isValidFaceIndex(m_model, faceIndex))
        {
            return false;
        }

        const auto* face = m_model.faceAt(faceIndex);

        if (face == nullptr)
        {
            return false;
        }

        if (face->info.kind != SurfaceKind::Plane)
        {
            return false;
        }

        if (!face->info.plane.has_value())
        {
            return false;
        }

        const auto& plane = face->info.plane.value();

        return isSamePlane(group.axisPoint, group.axisDirection, plane.origin, plane.normal);
    }

    void HoleContextGeometryGrouper::mergePlanarFace(HoleContextGeometryGroup& group, int faceIndex) const
    {
        if (!TopologyQuery::isValidFaceIndex(m_model, faceIndex))
        {
            return;
        }

        CollectionUtil::addUnique(group.geometryRefs.faceIndices, faceIndex);
    }

    HoleContextGeometryGroup HoleContextGeometryGrouper::createPlanarGroup(int faceIndex, const HoleContextGeometryGroup& parentGroup) const
    {
        HoleContextGeometryGroup group;

        group.index = -1;
        group.kind = HoleContextGeometryGroupKind::Unknown;
        group.hasAxis = false;

        if (!TopologyQuery::isValidFaceIndex(m_model, faceIndex))
        {
            return group;
        }

        const auto* face = m_model.faceAt(faceIndex);

        if (face == nullptr)
        {
            return group;
        }

        if (face->info.kind != SurfaceKind::Plane)
        {
            return group;
        }

        if (!face->info.plane.has_value())
        {
            return group;
        }

        const auto& plane = face->info.plane.value();

        group.kind = HoleContextGeometryGroupKind::Planar;
        group.geometryRefs.faceIndices.push_back(faceIndex);

        // 平面では axisPoint / axisDirection を
        // 代表点 / 法線として使う。
        group.hasAxis = true;
        group.axisPoint = plane.origin;
        group.axisDirection = plane.normal;

        group.note =
            "Created as planar hole-context geometry group from trace outside geometry.";

        if (parentGroup.kind == HoleContextGeometryGroupKind::Cylindrical &&
            parentGroup.hasAxis)
        {
            group.note +=
                " Parent group is cylindrical.";

            if (SurfaceUtil::isSameDirectionOrReverse(
                    group.axisDirection,
                    parentGroup.axisDirection,
                    AxisDirectionTolerance))
            {
                group.note +=
                    " Plane normal is parallel to parent cylinder axis.";
            }
        }

        return group;
    }

    bool HoleContextGeometryGrouper::isSamePlane(
        const gp_Pnt& pointA,
        const gp_Dir& normalA,
        const gp_Pnt& pointB,
        const gp_Dir& normalB) const
    {
        if (!SurfaceUtil::isSameDirectionOrReverse(
                normalA,
                normalB,
                PlaneNormalTolerance))
        {
            return false;
        }

        const gp_Vec vectorAB(pointA, pointB);
        const gp_Vec normalVector(normalA);

        const double distance =
            std::abs(vectorAB.Dot(normalVector));

        return distance <= PlaneDistanceTolerance;
    }
}
