#include "Feature/HoleContextSeedWallFinder.h"

#include "Feature/HoleContextCylindricalGroupBuilder.h"
#include "Feature/HoleContextGeometryGroupRegistry.h"


namespace OccQtCore::Feature
{
    HoleContextSeedWallFinder::HoleContextSeedWallFinder(
        const GeometryModel& model,
        HoleRecognitionWorkingData* workingData)
        : m_model(model)
        , m_workingData(workingData)
    {
    }
    std::optional<HoleContextGeometryGroup>
    HoleContextSeedWallFinder::findNext(
        const HoleContextGeometryGroupRegistry& groupRegistry,
        const std::set<int>& exploredWallGroupIndices) const
    {
        HoleContextCylindricalGroupBuilder cylindricalBuilder(m_model);

        const auto candidates =
            cylindricalBuilder.build();

        for (const auto& candidate : candidates)
        {
            if (candidate.kind != HoleContextGeometryGroupKind::WallCandidate)
            {
                continue;
            }

            if (isAlreadyExploredWall(
                    groupRegistry,
                    exploredWallGroupIndices,
                    candidate))
            {
                continue;
            }

            return candidate;
        }

        return std::nullopt;
    }

    bool HoleContextSeedWallFinder::isAlreadyExploredWall(
        const HoleContextGeometryGroupRegistry& groupRegistry,
        const std::set<int>& exploredWallGroupIndices,
        const HoleContextGeometryGroup& candidate) const
    {
        for (const auto& existingGroup : groupRegistry.groups())
        {
            if (existingGroup.kind != HoleContextGeometryGroupKind::WallCandidate)
            {
                continue;
            }

            if (exploredWallGroupIndices.find(existingGroup.index) ==
                exploredWallGroupIndices.end())
            {
                continue;
            }

            if (groupRegistry.isSameGroup(existingGroup, candidate))
            {
                return true;
            }
        }

        return false;
    }
}
