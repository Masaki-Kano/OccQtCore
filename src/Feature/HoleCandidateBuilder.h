#ifndef HOLECANDIDATEBUILDER_H
#define HOLECANDIDATEBUILDER_H

#include <vector>

#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

#include "Feature/HoleRecognitionTypes.h"

namespace OccQtCore
{
    class GeometryModel;
}

namespace OccQtCore::Feature
{
    class HoleCandidateBuilder
    {
    public:
        HoleCandidateBuilder(
            const GeometryModel& model,
            const std::vector<HoleWallCandidate>& wallCandidates,
            const std::vector<HoleEndCandidate>& endCandidates,
            const std::vector<HoleSegmentCandidate>& segmentCandidates);

        std::vector<HoleCandidate> build() const;

    private:
        struct CandidateAxis
        {
            gp_Pnt point;
            gp_Dir direction = gp_Dir(0.0, 0.0, 1.0);
            bool isValid = false;
        };

    private:
        std::vector<std::vector<int>> buildSameAxisSegmentGroups() const;

        CandidateAxis buildCandidateAxis(const std::vector<int>& segmentCandidateIndices) const;

        std::vector<std::vector<int>> buildReachableSegmentGroups(
            const std::vector<int>& sameAxisSegmentIndices,
            const CandidateAxis& candidateAxis,
            std::vector<HoleReachability>& reachabilities) const;

        HoleCandidate buildCandidateFromSegmentGroup(int candidateIndex,
                                                     const std::vector<int>& segmentCandidateIndices,
                                                     const std::vector<HoleReachability>& reachabilities) const;

        bool tryBuildSegmentReachability(
            int lhsSegmentCandidateIndex,
            int rhsSegmentCandidateIndex,
            const CandidateAxis& candidateAxis,
            HoleReachability& reachability) const;

        bool tryBuildEndReachability(
            int lhsEndCandidateIndex,
            int rhsEndCandidateIndex,
            const CandidateAxis& candidateAxis,
            HoleReachability& reachability) const;

        bool tryBuildSharedGeometryRefReachability(
            const HoleEndCandidate& lhsEnd,
            const HoleEndCandidate& rhsEnd,
            HoleReachability& reachability) const;

        bool tryBuildSharedAdjacentFaceReachability(
            const HoleEndCandidate& lhsEnd,
            const HoleEndCandidate& rhsEnd,
            const CandidateAxis& candidateAxis,
            HoleReachability& reachability) const;

        std::vector<int> collectAdjacentHolePathFaceIndicesOfEnd(const HoleEndCandidate& end, const CandidateAxis& candidateAxis) const;

        bool isAllowedHolePathFace(int faceIndex, const CandidateAxis& candidateAxis) const;

        std::vector<HoleReachability> filterReachabilitiesForSegmentGroup(
            const std::vector<int>& segmentCandidateIndices,
            const std::vector<HoleReachability>& reachabilities) const;

    private:
        bool isSameAxisSegment(const HoleSegmentCandidate& lhs, const HoleSegmentCandidate& rhs) const;

        bool isWallOnCandidateAxis(const CandidateAxis& candidateAxis, const HoleWallCandidate& wallCandidate) const;

        bool isValidSegmentIndex(int index) const;
        bool isValidWallIndex(int index) const;
        bool isValidEndIndex(int index) const;

    private:
        const GeometryModel& m_model;

        const std::vector<HoleWallCandidate>& m_wallCandidates;
        const std::vector<HoleEndCandidate>& m_endCandidates;
        const std::vector<HoleSegmentCandidate>& m_segmentCandidates;
    };
}

#endif // HOLECANDIDATEBUILDER_H
