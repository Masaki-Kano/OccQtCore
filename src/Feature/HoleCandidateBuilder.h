#ifndef HOLECANDIDATEBUILDER_H
#define HOLECANDIDATEBUILDER_H

#include <vector>

#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

#include "Feature/HoleRecognitionTypes.h"

namespace OccQtCore::Feature
{
    class HoleCandidateBuilder
    {
    public:
        HoleCandidateBuilder(
            const std::vector<HoleWallCandidate>& wallCandidates,
            const std::vector<HoleEndCandidate>& endCandidates,
            const std::vector<HoleSegmentCandidate>& segmentCandidates);

        std::vector<HoleCandidate> build() const;

    private:
        enum class SegmentConnectionKind
        {
            Unknown,
            SharedEndGeometry,
            ShoulderPlane,
            AxialRangeNear
        };

        struct CandidateAxis
        {
            gp_Pnt point;
            gp_Dir direction = gp_Dir(0.0, 0.0, 1.0);
            bool isValid = false;
        };

        struct SegmentRange
        {
            int segmentCandidateIndex = -1;

            double minAxial = 0.0;
            double maxAxial = 0.0;

            int minEndCandidateIndex = -1;
            int maxEndCandidateIndex = -1;

            bool isValid = false;
        };

        struct SegmentConnection
        {
            int currentSegmentCandidateIndex = -1;
            int nextSegmentCandidateIndex = -1;

            int currentEndCandidateIndex = -1;
            int nextEndCandidateIndex = -1;

            double axialRangeGap = 0.0;

            SegmentConnectionKind kind = SegmentConnectionKind::Unknown;
        };

        struct SegmentChain
        {
            int index = -1;
            std::vector<int> segmentCandidateIndices;
        };

    private:
        std::vector<std::vector<int>> buildSameAxisSegmentGroups() const;

        CandidateAxis buildCandidateAxis(
            const std::vector<int>& segmentCandidateIndices) const;

        SegmentRange buildSegmentRange(
            const HoleSegmentCandidate& segment,
            const CandidateAxis& candidateAxis) const;

        std::vector<SegmentRange> buildSegmentRanges(
            const std::vector<int>& segmentCandidateIndices,
            const CandidateAxis& candidateAxis) const;

        std::vector<SegmentConnection> buildSegmentConnections(
            const std::vector<SegmentRange>& segmentRanges) const;

        SegmentConnection buildSegmentConnection(
            const SegmentRange& currentRange,
            const SegmentRange& nextRange) const;

        SegmentConnectionKind classifySegmentConnection(
            const SegmentRange& currentRange,
            const SegmentRange& nextRange) const;

        std::vector<SegmentChain> buildSegmentChains(
            const std::vector<SegmentRange>& segmentRanges,
            const std::vector<SegmentConnection>& connections) const;

        HoleCandidate buildCandidateFromChain(
            int candidateIndex,
            const SegmentChain& chain) const;

        bool isSameAxisSegment(
            const HoleSegmentCandidate& lhs,
            const HoleSegmentCandidate& rhs) const;

        bool isWallOnCandidateAxis(
            const CandidateAxis& candidateAxis,
            const HoleWallCandidate& wallCandidate) const;

        bool isValidSegmentIndex(int index) const;
        bool isValidWallIndex(int index) const;
        bool isValidEndIndex(int index) const;

        bool hasSharedEndGeometry(
            const HoleEndCandidate& lhs,
            const HoleEndCandidate& rhs) const;

    private:
        const std::vector<HoleWallCandidate>& m_wallCandidates;
        const std::vector<HoleEndCandidate>& m_endCandidates;
        const std::vector<HoleSegmentCandidate>& m_segmentCandidates;
    };
}

#endif // HOLECANDIDATEBUILDER_H
