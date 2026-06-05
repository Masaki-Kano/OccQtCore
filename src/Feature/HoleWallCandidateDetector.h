#ifndef HOLEWALLCANDIDATEDETECTOR_H
#define HOLEWALLCANDIDATEDETECTOR_H

#include <vector>

#include "Feature/HoleRecognitionTypes.h"

namespace OccQtCore
{
    class GeometryModel;
}

namespace OccQtCore::Feature
{
    class HoleWallCandidateDetector
    {
    public:
        std::vector<HoleWallCandidate> detect(
            const GeometryModel& model) const;

    private:
        std::vector<HoleWallCandidate> collectRawCylinderCandidates(
            const GeometryModel& model) const;

        std::vector<int> collectConnectedSameCylinderGroup(
            const GeometryModel& model,
            const std::vector<HoleWallCandidate>& rawCandidates,
            int baseCandidateIndex,
            std::vector<bool>& used) const;

        HoleWallCandidate buildWallCandidateFromGroup(
            const std::vector<HoleWallCandidate>& rawCandidates,
            const std::vector<int>& groupCAndidateIndices) const;
    };
}

#endif // HOLEWALLCANDIDATEDETECTOR_H
