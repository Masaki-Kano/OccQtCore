#ifndef HOLESEGMENTCANDIDATEBUILDER_H
#define HOLESEGMENTCANDIDATEBUILDER_H

#include <vector>

#include "Feature/HoleRecognitionTypes.h"

namespace OccQtCore::Feature
{
    class HoleSegmentCandidateBuilder
    {
    public:
        HoleSegmentCandidateBuilder(
            const std::vector<HoleWallCandidate>& wallCandidates,
            const std::vector<HoleEndCandidate>& endCandidates);

        std::vector<HoleSegmentCandidate> build() const;

    private:
        int holeEndTypePriority(
            HoleEndCandidateType type) const;

        bool isBetterRepresentativeEnd(
            const HoleEndCandidate& current,
            const HoleEndCandidate& next) const;

        std::vector<int> selectRepresentativeEndIndices(
            const std::vector<int>& sourceEndIndices) const;

        void countEndTypes(
            const std::vector<int>& endCandidateIndices,
            int& openCount,
            int& bottomCount,
            int& wallConnectionCount) const;

        Hole::Type classifySegmentType(
            const std::vector<int>& endCandidateIndices) const;

        bool isValidEndIndex(int index) const;

    private:
        const std::vector<HoleWallCandidate>& m_wallCandidates;
        const std::vector<HoleEndCandidate>& m_endCandidates;
    };
}

#endif // HOLESEGMENTCANDIDATEBUILDER_H
