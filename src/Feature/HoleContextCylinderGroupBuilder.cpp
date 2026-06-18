#include "Feature/HoleContextCylindricalGroupBuilder.h"

#include "Geometry/GeometryModel.h"
#include "Geometry/SurfaceUtil.h"
#include "Geometry/TopologyQuery.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <utility>

#include <QDebug>

namespace OccQtCore::Feature
{
    namespace
    {
        constexpr double AxisDirectionTolerance = 1.0e-6;
        constexpr double AxisPointTolerance = 1.0e-4;
        constexpr double RadiusTolerance = 1.0e-4;
        constexpr double ParameterRangeTolerance = 1.0e-4;
        constexpr double FullCircumferenceTolerance = 1.0e-3;
        constexpr double TwoPi = 2.0 * M_PI;
    }

        HoleContextCylindricalGroupBuilder::HoleContextCylindricalGroupBuilder(
            const GeometryModel& model,
            HoleRecognitionWorkingData* workingData)
            : m_model(model)
            , m_workingData(workingData)
        {
        }

    std::vector<HoleContextGeometryGroup>
    HoleContextCylindricalGroupBuilder::build() const
    {
        std::vector<int> faceIndices;

        for (int faceIndex = 0; faceIndex < m_model.faceCount(); ++faceIndex)
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

            if (face->info.kind != SurfaceKind::Cylinder)
            {
                continue;
            }

            faceIndices.push_back(faceIndex);
        }

        return buildFromFaces(faceIndices);
    }

    std::vector<HoleContextGeometryGroup>
    HoleContextCylindricalGroupBuilder::build(
        const std::vector<int>& faceIndices) const
    {
        return buildFromFaces(faceIndices);
    }

    std::vector<HoleContextGeometryGroup>
    HoleContextCylindricalGroupBuilder::buildFromFaces(
        const std::vector<int>& faceIndices) const
    {
        qDebug()
        << "[CylBuilder] buildFromFaces"
        << "workingData=" << (m_workingData != nullptr)
        << "faceCount=" << faceIndices.size();

        debugDumpCylinderFaceGraph(faceIndices);

        std::vector<WorkingGroup> workingGroups;

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

            if (face->info.kind != SurfaceKind::Cylinder)
            {
                continue;
            }

            if (!face->info.cylinder.has_value())
            {
                continue;
            }

            bool merged = false;

            for (auto& group : workingGroups)
            {
                if (canMergeFace(group, faceIndex))
                {
                    mergeFace(group, faceIndex);
                    merged = true;
                    break;
                }
            }

            if (merged)
            {
                continue;
            }

            auto group =
                createWorkingGroup(
                    faceIndex,
                    static_cast<int>(workingGroups.size()));

            if (group.group.geometryRefs.faceIndices.empty())
            {
                continue;
            }

            workingGroups.push_back(std::move(group));
        }

        std::vector<HoleContextGeometryGroup> groups;

        for (const auto& workingGroup : workingGroups)
        {
            const auto promotion =
                evaluateWallCandidatePromotion(workingGroup);

            recordCylindricalWorkingGroupDebugInfo(
                workingGroup,
                promotion);

            if (!promotion.accepted)
            {
                continue;
            }

            groups.push_back(workingGroup.group);
        }

        return groups;
    }

    bool HoleContextCylindricalGroupBuilder::canMergeFace(
        const WorkingGroup& group,
        int faceIndex) const
    {   
        if (group.group.kind != HoleContextGeometryGroupKind::WallCandidate)
        {
            return false;
        }

        if (!group.group.hasReferencePoint ||
            !group.group.hasReferenceDirection)
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

        if (face->info.kind != SurfaceKind::Cylinder)
        {
            return false;
        }

        if (!face->info.cylinder.has_value())
        {
            return false;
        }

        const auto& cylinder = face->info.cylinder.value();

        if (!SurfaceUtil::isSameAxis(
                group.group.referencePoint,
                group.group.referenceDirection,
                cylinder.axis.Location(),
                cylinder.axis.Direction(),
                AxisPointTolerance,
                AxisDirectionTolerance))
        {
            return false;
        }

        if (std::abs(group.group.radius - cylinder.radius) > RadiusTolerance)
        {
            return false;
        }

        double faceParameterMin = 0.0;
        double faceParameterMax = 0.0;

        if (!computeFaceParameterRange(
                *face,
                group.group.referencePoint,
                group.group.referenceDirection,
                faceParameterMin,
                faceParameterMax))
        {
            return false;
        }

        if (!isParameterRangeConnected(
                group.group.parameterMin,
                group.group.parameterMax,
                faceParameterMin,
                faceParameterMax))
        {
            return false;
        }



        return isFaceConnectedToGroup(group, faceIndex);
    }

    void HoleContextCylindricalGroupBuilder::mergeFace(
        WorkingGroup& group,
        int faceIndex) const
    {
        if (!TopologyQuery::isValidFaceIndex(m_model, faceIndex))
        {
            return;
        }

        const auto* face = m_model.faceAt(faceIndex);

        if (face == nullptr)
        {
            return;
        }

        if (std::find(
                group.group.geometryRefs.faceIndices.begin(),
                group.group.geometryRefs.faceIndices.end(),
                faceIndex) == group.group.geometryRefs.faceIndices.end())
        {
            group.group.geometryRefs.faceIndices.push_back(faceIndex);
        }

        if (std::find(
                group.faceIndices.begin(),
                group.faceIndices.end(),
                faceIndex) == group.faceIndices.end())
        {
            group.faceIndices.push_back(faceIndex);
        }

        double faceParameterMin = 0.0;
        double faceParameterMax = 0.0;

        if (computeFaceParameterRange(
                *face,
                group.group.referencePoint,
                group.group.referenceDirection,
                faceParameterMin,
                faceParameterMax))
        {
            group.group.parameterMin =
                std::min(group.group.parameterMin, faceParameterMin);

            group.group.parameterMax =
                std::max(group.group.parameterMax, faceParameterMax);

            group.group.parameterPosition =
                0.5 * (group.group.parameterMin + group.group.parameterMax);
        }
    }

    HoleContextCylindricalGroupBuilder::WorkingGroup
    HoleContextCylindricalGroupBuilder::createWorkingGroup(
        int faceIndex,
        int groupIndex) const
    {
        WorkingGroup workingGroup;

        workingGroup.group.index = groupIndex;
        workingGroup.group.kind = HoleContextGeometryGroupKind::Unknown;

        if (!TopologyQuery::isValidFaceIndex(m_model, faceIndex))
        {
            return workingGroup;
        }

        const auto* face = m_model.faceAt(faceIndex);

        if (face == nullptr)
        {
            return workingGroup;
        }

        if (face->info.kind != SurfaceKind::Cylinder)
        {
            return workingGroup;
        }

        if (!face->info.cylinder.has_value())
        {
            return workingGroup;
        }

        const auto& cylinder = face->info.cylinder.value();

        double parameterMin = 0.0;
        double parameterMax = 0.0;

        if (!computeFaceParameterRange(
                *face,
                cylinder.axis.Location(),
                cylinder.axis.Direction(),
                parameterMin,
                parameterMax))
        {
            return workingGroup;
        }

        workingGroup.group.kind = HoleContextGeometryGroupKind::WallCandidate;
        workingGroup.group.geometryRefs.faceIndices.push_back(faceIndex);

        workingGroup.group.hasReferencePoint = true;
        workingGroup.group.referencePoint = cylinder.axis.Location();

        workingGroup.group.hasReferenceDirection = true;
        workingGroup.group.referenceDirection = cylinder.axis.Direction();

        workingGroup.group.radius = cylinder.radius;

        workingGroup.group.parameterMin = parameterMin;
        workingGroup.group.parameterMax = parameterMax;
        workingGroup.group.parameterPosition =
            0.5 * (parameterMin + parameterMax);

        workingGroup.group.note =
            "Created as wall candidate hole-context geometry group from cylindrical faces.";

        workingGroup.faceIndices.push_back(faceIndex);

        return workingGroup;
    }

    bool HoleContextCylindricalGroupBuilder::isValidWallCandidate(
        const WorkingGroup& group) const
    {
      return evaluateWallCandidatePromotion(group).accepted;
    }

    CylindricalWallPromotionResult
    HoleContextCylindricalGroupBuilder::evaluateWallCandidatePromotion(
        const WorkingGroup& group) const
    {
        CylindricalWallPromotionResult result;

        if (group.group.kind != HoleContextGeometryGroupKind::WallCandidate)
        {
            result.rejectReason =
                CylindricalWallPromotionRejectReason::NotWallCandidateKind;
            return result;
        }

        if (group.group.geometryRefs.faceIndices.empty())
        {
            result.rejectReason =
                CylindricalWallPromotionRejectReason::EmptyFaces;
            return result;
        }

        if (!group.group.hasReferencePoint ||
            !group.group.hasReferenceDirection)
        {
            result.rejectReason =
                CylindricalWallPromotionRejectReason::MissingReference;
            return result;
        }

        if (group.group.radius <= 0.0)
        {
            result.rejectReason =
                CylindricalWallPromotionRejectReason::InvalidRadius;
            return result;
        }

        if (!hasFullCircumferentialCoverage(group))
        {
            result.rejectReason =
                CylindricalWallPromotionRejectReason::
                InsufficientCircumferentialCoverage;
            return result;
        }

        if (!hasTopologicalCircumferentialLoop(group))
        {
            result.rejectReason =
                CylindricalWallPromotionRejectReason::
                MissingTopologicalCircumferentialLoop;
            return result;
        }

        if (!hasInnerCylindricalFace(group))
        {
            result.rejectReason =
                CylindricalWallPromotionRejectReason::NoInnerCylindricalFace;
            return result;
        }

        result.accepted = true;
        result.rejectReason = CylindricalWallPromotionRejectReason::None;
        return result;
    }

    void HoleContextCylindricalGroupBuilder::recordCylindricalWorkingGroupDebugInfo(
        const WorkingGroup& group,
        const CylindricalWallPromotionResult& promotion) const
    {
        qDebug()
        << "[CylBuilder] record"
        << "workingData=" << (m_workingData != nullptr)
        << "faces=" << group.faceIndices.size()
        << "accepted=" << promotion.accepted;

        if (m_workingData == nullptr)
        {
            return;
        }

        CylindricalWorkingGroupDebugInfo debugInfo;

        debugInfo.index = group.group.index;
        debugInfo.faceIndices = group.faceIndices;
        debugInfo.radius = group.group.radius;
        debugInfo.promotion = promotion;
        debugInfo.note =
            "Cylindrical working group promotion evaluation.";

        m_workingData->cylindricalWorkingGroups.push_back(
            std::move(debugInfo));
    }

    bool HoleContextCylindricalGroupBuilder::hasFullCircumferentialCoverage(
        const WorkingGroup& group) const
    {
        if (group.faceIndices.empty())
        {
            return false;
        }

        double totalULength = 0.0;

        for (const int faceIndex : group.faceIndices)
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

            if (face->info.kind != SurfaceKind::Cylinder)
            {
                continue;
            }

            if (!face->info.cylinder.has_value())
            {
                continue;
            }

            double uLength =
                std::abs(face->info.uMax - face->info.uMin);

            while (uLength > TwoPi)
            {
                uLength -= TwoPi;
            }

            totalULength += uLength;
        }

        return totalULength >= TwoPi - FullCircumferenceTolerance;
    }

    bool HoleContextCylindricalGroupBuilder::hasTopologicalCircumferentialLoop(
        const WorkingGroup& group) const
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
                        group.group.referenceDirection))
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
                    const auto it =
                        std::find(
                            group.faceIndices.begin(),
                            group.faceIndices.end(),
                            adjacentFaceIndex);

                    if (it == group.faceIndices.end())
                    {
                        continue;
                    }

                    const auto adjacentLocalIndex =
                        static_cast<int>(
                            std::distance(group.faceIndices.begin(), it));

                    adjacency[i].insert(adjacentLocalIndex);
                    adjacency[static_cast<size_t>(adjacentLocalIndex)]
                        .insert(static_cast<int>(i));
                }
            }
        }

        for (const auto& connectedIndices : adjacency)
        {
            if (connectedIndices.empty())
            {
                return false;
            }
        }

        return true;
    }

    bool HoleContextCylindricalGroupBuilder::isFaceConnectedToGroup(
        const WorkingGroup& group,
        int faceIndex) const
    {
        if (group.faceIndices.empty())
        {
            return true;
        }

        if (!TopologyQuery::isValidFaceIndex(m_model, faceIndex))
        {
            return false;
        }

        const auto candidateEdgeIndices =
            TopologyQuery::edgesOfFace(m_model, faceIndex);

        for (const int candidateEdgeIndex : candidateEdgeIndices)
        {
            /*
            if (!isAxialEdgeOfCylinder(
                    candidateEdgeIndex,
                    group.group.referenceDirection))
            {
                continue;
            }
            */

            for (const int groupFaceIndex : group.faceIndices)
            {
                if (!TopologyQuery::isValidFaceIndex(m_model, groupFaceIndex))
                {
                    continue;
                }

                const auto groupEdgeIndices =
                    TopologyQuery::edgesOfFace(m_model, groupFaceIndex);

                if (std::find(
                        groupEdgeIndices.begin(),
                        groupEdgeIndices.end(),
                        candidateEdgeIndex) != groupEdgeIndices.end())
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool HoleContextCylindricalGroupBuilder::hasInnerCylindricalFace(
        const WorkingGroup& group) const
    {
        for (const int faceIndex : group.faceIndices)
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

            if (isInnerCylindricalFace(*face))
            {
                return true;
            }
        }

        return false;
    }

    bool HoleContextCylindricalGroupBuilder::isInnerCylindricalFace(
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

        const auto& cylinder =
            face.info.cylinder.value();

        return SurfaceUtil::isCylinderFaceInwardOriented(
            face.shape,
            cylinder.axis,
            face.info.uMin,
            face.info.uMax,
            face.info.vMin,
            face.info.vMax);
    }

    bool HoleContextCylindricalGroupBuilder::isAxialEdgeOfCylinder(
        int edgeIndex,
        const gp_Dir& referenceDirection) const
    {
        if (!TopologyQuery::isValidEdgeIndex(m_model, edgeIndex))
        {
            return false;
        }

        const auto* edge = m_model.edgeAt(edgeIndex);

        if (edge == nullptr)
        {
            return false;
        }

        if (!edge->info.line.has_value())
        {
            return false;
        }

        const auto& line = edge->info.line.value();

        return SurfaceUtil::isSameDirectionOrReverse(
            line.direction,
            referenceDirection,
            AxisDirectionTolerance);
    }

    bool HoleContextCylindricalGroupBuilder::computeFaceParameterRange(
        const FaceData& face,
        const gp_Pnt& referencePoint,
        const gp_Dir& referenceDirection,
        double& parameterMin,
        double& parameterMax) const
    {
        const auto vertexIndices =
            TopologyQuery::verticesOfFace(
                m_model,
                face.index);

        if (vertexIndices.empty())
        {
            return false;
        }

        parameterMin = std::numeric_limits<double>::max();
        parameterMax = -std::numeric_limits<double>::max();

        const gp_Vec referenceDirectionVector(referenceDirection);

        for (const int vertexIndex : vertexIndices)
        {
            if (!TopologyQuery::isValidVertexIndex(m_model, vertexIndex))
            {
                continue;
            }

            const auto* vertex = m_model.vertexAt(vertexIndex);

            if (vertex == nullptr)
            {
                continue;
            }

            const gp_Vec vector(referencePoint, vertex->info.point);

            const double parameter =
                vector.Dot(referenceDirectionVector);

            parameterMin = std::min(parameterMin, parameter);
            parameterMax = std::max(parameterMax, parameter);
        }

        if (parameterMin == std::numeric_limits<double>::max() ||
            parameterMax == -std::numeric_limits<double>::max())
        {
            return false;
        }

        return true;
    }

    bool HoleContextCylindricalGroupBuilder::isParameterRangeConnected(
        double min1,
        double max1,
        double min2,
        double max2) const
    {
        return min1 <= max2 + ParameterRangeTolerance &&
               min2 <= max1 + ParameterRangeTolerance;
    }

    bool HoleContextCylindricalGroupBuilder::canConnectCylinderFaces(
        int lhsFaceIndex,
        int rhsFaceIndex) const
    {
        if (!TopologyQuery::isValidFaceIndex(m_model, lhsFaceIndex) ||
            !TopologyQuery::isValidFaceIndex(m_model, rhsFaceIndex))
        {
            return false;
        }

        const auto* lhsFace = m_model.faceAt(lhsFaceIndex);
        const auto* rhsFace = m_model.faceAt(rhsFaceIndex);

        if (lhsFace == nullptr || rhsFace == nullptr)
        {
            return false;
        }

        if (lhsFace->info.kind != SurfaceKind::Cylinder ||
            rhsFace->info.kind != SurfaceKind::Cylinder)
        {
            return false;
        }

        if (!lhsFace->info.cylinder.has_value() ||
            !rhsFace->info.cylinder.has_value())
        {
            return false;
        }

        const auto& lhsCylinder = lhsFace->info.cylinder.value();
        const auto& rhsCylinder = rhsFace->info.cylinder.value();

        if (!SurfaceUtil::isSameAxis(
                lhsCylinder.axis.Location(),
                lhsCylinder.axis.Direction(),
                rhsCylinder.axis.Location(),
                rhsCylinder.axis.Direction(),
                AxisPointTolerance,
                AxisDirectionTolerance))
        {
            return false;
        }

        if (std::abs(lhsCylinder.radius - rhsCylinder.radius) > RadiusTolerance)
        {
            return false;
        }

        return true;
    }

    void HoleContextCylindricalGroupBuilder::debugDumpCylinderFaceGraph(
        const std::vector<int>& faceIndices) const
    {
        std::set<int> cylinderFaceSet;

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

            if (face->info.kind != SurfaceKind::Cylinder)
            {
                continue;
            }

            if (!face->info.cylinder.has_value())
            {
                continue;
            }

            cylinderFaceSet.insert(faceIndex);

            const auto& cylinder = face->info.cylinder.value();
        }

        std::map<int, std::set<int>> adjacency;

        for (const int faceIndex : cylinderFaceSet)
        {
            adjacency[faceIndex];

            const auto edgeIndices =
                TopologyQuery::edgesOfFace(m_model, faceIndex);

            for (const int edgeIndex : edgeIndices)
            {
                const auto adjacentFaceIndices =
                    TopologyQuery::adjacentFacesOfEdge(
                        m_model,
                        edgeIndex,
                        faceIndex);

                for (const int adjacentFaceIndex : adjacentFaceIndices)
                {
                    if (cylinderFaceSet.find(adjacentFaceIndex) ==
                        cylinderFaceSet.end())
                    {
                        continue;
                    }

                    adjacency[faceIndex].insert(adjacentFaceIndex);
                    adjacency[adjacentFaceIndex].insert(faceIndex);
                }
            }
        }

        std::set<int> visited;
        int componentIndex = 0;

        for (const int seedFaceIndex : cylinderFaceSet)
        {
            if (visited.find(seedFaceIndex) != visited.end())
            {
                continue;
            }

            std::vector<int> componentFaces;
            std::vector<int> stack;

            stack.push_back(seedFaceIndex);
            visited.insert(seedFaceIndex);

            while (!stack.empty())
            {
                const int currentFaceIndex = stack.back();
                stack.pop_back();

                componentFaces.push_back(currentFaceIndex);

                for (const int nextFaceIndex : adjacency[currentFaceIndex])
                {
                    if (visited.find(nextFaceIndex) != visited.end())
                    {
                        continue;
                    }

                    visited.insert(nextFaceIndex);
                    stack.push_back(nextFaceIndex);
                }
            }

            std::sort(componentFaces.begin(), componentFaces.end());

            QStringList faceTexts;

            for (const int componentFaceIndex : componentFaces)
            {
                faceTexts << QString::number(componentFaceIndex);
            }

            ++componentIndex;
        }
    }
}
