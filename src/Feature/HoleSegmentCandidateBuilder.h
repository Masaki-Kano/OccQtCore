#ifndef HOLESEGMENTCANDIDATEBUILDER_H
#define HOLESEGMENTCANDIDATEBUILDER_H

#include <vector>

#include "Feature/HoleRecognitionTypes.h"

namespace OccQtCore
{
    class GeometryModel;
}

namespace OccQtCore::Feature
{
    class HoleSegmentCandidateBuilder
    {
    public:
        HoleSegmentCandidateBuilder(
            const GeometryModel& model,
            const std::vector<HoleWallCandidate>& wallCandidates,
            const std::vector<HoleEndCandidate>& endCandidates);

        std::vector<HoleSegmentCandidate> build() const;

    private:
        double calculateSegmentDepth(const HoleSegmentCandidate& candidate) const;

        void appendVertexIndices(const GeometryRefs& refs, std::vector<int>& vertexIndeces) const;

        bool isValidEndIndex(int index) const;
        bool isValidWallIndex(int index) const;

    private:
        const GeometryModel& m_model;
        const std::vector<HoleWallCandidate>& m_wallCandidates;
        const std::vector<HoleEndCandidate>& m_endCandidates;
    };
}

#endif // HOLESEGMENTCANDIDATEBUILDER_H
