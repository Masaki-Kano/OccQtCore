#ifndef HOLECONTEXTSEEDWALLFINDER_H
#define HOLECONTEXTSEEDWALLFINDER_H

#include "Feature/HoleRecognitionModel.h"
#include "Feature/HoleRecognitionWorkingData.h"

#include <optional>
#include <set>

namespace OccQtCore
{
    class GeometryModel;
}

namespace OccQtCore::Feature
{
    class HoleContextGeometryGroupRegistry;

    class HoleContextSeedWallFinder
    {
    public:
        explicit HoleContextSeedWallFinder(
            const GeometryModel& model,
            HoleRecognitionWorkingData* workingData = nullptr);

        std::optional<HoleContextGeometryGroup> findNext(
            const HoleContextGeometryGroupRegistry& groupRegistry,
            const std::set<int>& exploredWallGroupIndices) const;

    private:
        bool isAlreadyExploredWall(
            const HoleContextGeometryGroupRegistry& groupRegistry,
            const std::set<int>& exploredWallGroupIndices,
            const HoleContextGeometryGroup& candidate) const;

    private:
        const GeometryModel& m_model;
        HoleRecognitionWorkingData* m_workingData = nullptr;
    };
}

#endif // HOLECONTEXTSEEDWALLFINDER_H
