#include "Feature/HoleContextPlanarGroupBuilder.h"

#include "Geometry/GeometryModel.h"
#include "Geometry/SurfaceUtil.h"
#include "Geometry/TopologyQuery.h"

#include <algorithm>
#include <cmath>

namespace OccQtCore::Feature
{
    namespace
    {
        constexpr double PlaneNormalTolerance = 1.0e-6;
        constexpr double PlaneDistanceTolerance = 1.0e-4;
        constexpr double ReferenceDirectionTolerance = 1.0e-6;
    }

    HoleContextPlanarGroupBuilder::HoleContextPlanarGroupBuilder(
        const GeometryModel& model)
        : m_model(model)
    {
    }

    std::vector<HoleContextGeometryGroup>
    HoleContextPlanarGroupBuilder::build(
        const std::vector<int>& faceIndices) const
    {
        return buildFromFaces(faceIndices);
    }

    std::vector<HoleContextGeometryGroup>
    HoleContextPlanarGroupBuilder::buildFromFaces(
        const std::vector<int>& faceIndices) const
    {
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

            if (face->info.kind != SurfaceKind::Plane)
            {
                continue;
            }

            if (!face->info.plane.has_value())
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
            if (!isValidBoundaryCandidate(workingGroup))
            {
                continue;
            }

            groups.push_back(workingGroup.group);
        }

        return groups;
    }

    bool HoleContextPlanarGroupBuilder::canMergeFace(
        const WorkingGroup& group,
        int faceIndex) const
    {
        if (group.group.kind != HoleContextGeometryGroupKind::BoundaryCandidate)
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

        if (face->info.kind != SurfaceKind::Plane)
        {
            return false;
        }

        if (!face->info.plane.has_value())
        {
            return false;
        }

        const auto& plane = face->info.plane.value();

        return isSamePlane(
            group.group.referencePoint,
            group.group.referenceDirection,
            plane.origin,
            plane.normal);
    }

    void HoleContextPlanarGroupBuilder::mergeFace(
        WorkingGroup& group,
        int faceIndex) const
    {
        if (!TopologyQuery::isValidFaceIndex(m_model, faceIndex))
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
    }

    HoleContextPlanarGroupBuilder::WorkingGroup
    HoleContextPlanarGroupBuilder::createWorkingGroup(
        int faceIndex,
        int groupIndex) const
    {
        WorkingGroup workingGroup;

        workingGroup.group.index = groupIndex;
        workingGroup.group.kind = HoleContextGeometryGroupKind::Unknown;
        workingGroup.group.hasReferencePoint = false;
        workingGroup.group.hasReferenceDirection = false;

        if (!TopologyQuery::isValidFaceIndex(m_model, faceIndex))
        {
            return workingGroup;
        }

        const auto* face = m_model.faceAt(faceIndex);

        if (face == nullptr)
        {
            return workingGroup;
        }

        if (face->info.kind != SurfaceKind::Plane)
        {
            return workingGroup;
        }

        if (!face->info.plane.has_value())
        {
            return workingGroup;
        }

        const auto& plane = face->info.plane.value();

        workingGroup.group.kind =
            HoleContextGeometryGroupKind::BoundaryCandidate;

        workingGroup.group.geometryRefs.faceIndices.push_back(faceIndex);

        workingGroup.group.hasReferencePoint = true;
        workingGroup.group.referencePoint = plane.origin;

        workingGroup.group.hasReferenceDirection = true;
        workingGroup.group.referenceDirection = plane.normal;

        workingGroup.group.note =
            "Created as boundary candidate hole-context geometry group from planar faces.";

        workingGroup.faceIndices.push_back(faceIndex);

        return workingGroup;
    }

    bool HoleContextPlanarGroupBuilder::isValidBoundaryCandidate(
        const WorkingGroup& group) const
    {
        if (group.group.kind != HoleContextGeometryGroupKind::BoundaryCandidate)
        {
            return false;
        }

        if (group.group.geometryRefs.faceIndices.empty())
        {
            return false;
        }

        if (group.faceIndices.empty())
        {
            return false;
        }

        if (!group.group.hasReferencePoint ||
            !group.group.hasReferenceDirection)
        {
            return false;
        }

        return true;
    }

    bool HoleContextPlanarGroupBuilder::isSamePlane(
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
