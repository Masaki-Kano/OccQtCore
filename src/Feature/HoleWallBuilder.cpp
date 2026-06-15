#include "Feature/HoleWallBuilder.h"

#include "Core/CollectionUtil.h"

#include "Geometry/TopologyQuery.h"
#include "Geometry/GeometryModel.h"
#include "Geometry/SurfaceUtil.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <vector>

#include <BRep_Tool.hxx>
#include <TopAbs.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_Vec.hxx>

namespace
{
    constexpr double AxisDirectionTolerance = 1.0e-6;
    constexpr double AxisDistanceTolerance = 1.0e-4;
    constexpr double RadiusTolerance = 1.0e-4;
    constexpr double AxialRangeTolerance = 1.0e-4;

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

    bool containsFaceIndex(const std::vector<int>& faceIndices, int faceIndex)
    {
        return std::find(faceIndices.begin(), faceIndices.end(), faceIndex) != faceIndices.end();
    }
}

namespace OccQtCore::Feature
{
    std::vector<HoleWall> HoleWallBuilder::build(const GeometryModel& model) const
    {
        std::vector<WallGroup> groups;

        for (const auto& face : model.faces())
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

            bool merge = false;

            for (auto& group : groups)
            {
                if (!canMerge(model, group, face.index))
                {
                    continue;
                }

                mergeFace(model, group, face.index);
                merge = true;
                break;
            }

            if (!merge)
            {
                groups.push_back(createGroup(model, face.index, static_cast<int>(groups.size())));
            }
        }

        std::vector<HoleWall> walls;

        for (const auto& group : groups)
        {
            if (!isValidHoleWallGroup(model, group))
            {
                continue;
            }

            HoleWall wall = group.wall;
            wall.index = static_cast<int>(walls.size());
            wall.isValid = true;

            walls.push_back(wall);
        }

        return walls;
    }

    bool HoleWallBuilder::canMerge(const GeometryModel& model, const WallGroup& group, int faceIndex) const
    {
        if (!group.wall.isValid)
        {
            return false;
        }

        if (!TopologyQuery::isValidFaceIndex(model, faceIndex))
        {
            return false;
        }

        const auto* face = model.faceAt(faceIndex);

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
                group.wall.axisPoint,
                group.wall.axisDirection,
                group.wall.radius,
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
                group.wall.axisPoint,
                group.wall.axisDirection,
                faceAxialMin,
                faceAxialMax))
        {
            return false;
        }

        return isAxialRangeConnected(group.wall.axialMin, group.wall.axialMax, faceAxialMin, faceAxialMax);
    }

    void HoleWallBuilder::mergeFace(const GeometryModel& model, WallGroup& group, int faceIndex) const
    {
        if (!TopologyQuery::isValidFaceIndex(model, faceIndex))
        {
            return;
        }

        const auto* face = model.faceAt(faceIndex);

        CollectionUtil::addUnique(group.wall.geometryRefs.faceIndices, faceIndex);
        CollectionUtil::addUnique(group.faceIndices, faceIndex);

        double faceAxialMin = 0.0;
        double faceAxialMax = 0.0;

        if (computeFaceAxialRange(*face, group.wall.axisPoint, group.wall.axisDirection, faceAxialMin, faceAxialMax))
        {
            group.wall.axialMin = std::min(group.wall.axialMin, faceAxialMin);
            group.wall.axialMax = std::max(group.wall.axialMax, faceAxialMax);
        }
    }

    HoleWallBuilder::WallGroup HoleWallBuilder::createGroup(const GeometryModel& model, int faceIndex, int groupIndex) const
    {
        WallGroup group;

        group.wall.index = groupIndex;
        group.wall.isValid = false;

        if (!TopologyQuery::isValidFaceIndex(model, faceIndex))
        {
            return group;
        }

        const auto* face = model.faceAt(faceIndex);

        if (face->info.kind != SurfaceKind::Cylinder)
        {
            return group;
        }

        if (!face->info.cylinder.has_value())
        {
            return group;
        }

        const auto& cylinder = face->info.cylinder.value();

        group.wall.index = groupIndex;
        group.wall.isValid = true;

        group.wall.geometryRefs.faceIndices.push_back(faceIndex);
        group.faceIndices.push_back(faceIndex);

        group.wall.axisPoint = cylinder.axis.Location();
        group.wall.axisDirection = cylinder.axis.Direction();
        group.wall.radius = cylinder.radius;

        double axialMin = 0.0;
        double axialMax = 0.0;

        if (computeFaceAxialRange(
                *face,
                group.wall.axisPoint,
                group.wall.axisDirection,
                axialMin,
                axialMax))
        {
            group.wall.axialMin = axialMin;
            group.wall.axialMax = axialMax;
        }

        return group;
    }

    bool HoleWallBuilder::isValidHoleWallGroup(const GeometryModel& model, const WallGroup& group) const
    {
        if (!group.wall.isValid)
        {
            return false;
        }

        if (group.faceIndices.empty())
        {
            return false;
        }

        if (!hasFullCircumferentialCoverage(model, group))
        {
            return false;
        }

        if (!hasTopologicalCircumferentialLoop(model, group))
        {
            return false;
        }

        return true;
    }

    bool HoleWallBuilder::hasFullCircumferentialCoverage(const GeometryModel& model, const WallGroup& group) const
    {
        std::vector<std::pair<double, double>> intervals;

        for (const int faceIndex : group.faceIndices)
        {
            if (!TopologyQuery::isValidFaceIndex(model, faceIndex))
            {
                continue;
            }

            const auto* face = model.faceAt(faceIndex);

            const double rawMin = face->info.uMin;
            const double rawMax = face->info.uMax;
            const double rawSpan = SurfaceUtil::parameterSpan(rawMin, rawMax);

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

            if (interval.first <= currentMax + CircumferentialCoverageTolerance)
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

    bool HoleWallBuilder::hasTopologicalCircumferentialLoop(const GeometryModel& model, const WallGroup& group) const
    {
        if (group.faceIndices.empty())
        {
            return false;
        }

        // 1枚Faceで360度を表す円筒は、幾何カバレッジだけでOK。
        if (group.faceIndices.size() == 1)
        {
            return hasFullCircumferentialCoverage(model, group);
        }

        std::vector<std::set<int>> adjacency(group.faceIndices.size());

        for (size_t i = 0; i < group.faceIndices.size(); ++i)
        {
            const int faceIndex = group.faceIndices[i];

            if (!TopologyQuery::isValidFaceIndex(model, faceIndex))
            {
                continue;
            }

            const auto edgeIndices = TopologyQuery::edgesOfFace(model, faceIndex);

            for (const int edgeIndex : edgeIndices)
            {
                if (!isAxialEdgeOfCylinder(model, edgeIndex, group.wall.axisDirection))
                {
                    continue;
                }

                const auto adjacentFaceIndices =
                    TopologyQuery::adjacentFacesOfEdge(model, edgeIndex, faceIndex);

                for (const int adjacentFaceIndex : adjacentFaceIndices)
                {
                    if (!containsFaceIndex(group.faceIndices, adjacentFaceIndex))
                    {
                        continue;
                    }

                    const auto adjacentIt = std::find(
                        group.faceIndices.begin(),
                        group.faceIndices.end(),
                        adjacentFaceIndex);

                    if (adjacentIt == group.faceIndices.end())
                    {
                        continue;
                    }

                    const size_t adjacentLocalIndex =
                        static_cast<size_t>(
                            std::distance(group.faceIndices.begin(), adjacentIt));

                    if (adjacentLocalIndex == i)
                    {
                        continue;
                    }

                    adjacency[i].insert(static_cast<int>(adjacentLocalIndex));
                    adjacency[adjacentLocalIndex].insert(static_cast<int>(i));
                }
            }
        }

        // ここでは「各Faceが隣接2つ以上」を要求しない。
        // 2分割円筒では Face A <-> Face B となり、各Faceの隣接Face数は1になる。
        //
        // 円周360度を覆っているかは hasFullCircumferentialCoverage() 側で確認済み。
        // この関数では、円周方向にFace群がバラバラでないことだけを見る。

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

    bool HoleWallBuilder::isInnerCylindricalFace(const FaceData& face) const
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

    bool HoleWallBuilder::isAxialEdgeOfCylinder(
        const GeometryModel& model,
        int edgeIndex,
        const gp_Dir& axisDirection) const
    {
        if (!TopologyQuery::isValidEdgeIndex(model, edgeIndex))
        {
            return false;
        }

        const auto* edge = model.edgeAt(edgeIndex);

        if (edge->info.kind != CurveKind::Line)
        {
            return false;
        }

        if (!edge->info.line.has_value())
        {
            return false;
        }

        const gp_Dir edgeDirection = edge->info.line.value().direction;

        return SurfaceUtil::isSameDirectionOrReverse(
            edgeDirection,
            axisDirection,
            AxisDirectionTolerance);
    }

    bool HoleWallBuilder::computeFaceAxialRange(
        const FaceData& face,
        const gp_Pnt& axisPoint,
        const gp_Dir& axisDirection,
        double& axialMin,
        double& axialMax) const
    {
        double minValue = std::numeric_limits<double>::max();
        double maxValue = -std::numeric_limits<double>::max();

        bool hasVertex = false;

        for (TopExp_Explorer explorer(face.shape, TopAbs_VERTEX); explorer.More(); explorer.Next())
        {
            const TopoDS_Vertex vertex = TopoDS::Vertex(explorer.Current());
            const gp_Pnt point = BRep_Tool::Pnt(vertex);

            const double axial =
                SurfaceUtil::projectPointToAxis(axisPoint, axisDirection, point);

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

    bool HoleWallBuilder::isAxialRangeConnected(
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
}





