#ifndef HOLEDATABUILDER_H
#define HOLEDATABUILDER_H

#include <vector>

#include "Feature/FeatureTypes.h"
#include "Feature/HoleRecognitionTypes.h"

namespace OccQtCore::Feature
{
class HoleDataBuilder
{
    public:
        HoleDataBuilder(
            const std::vector<HoleWallCandidate>& wallCandidates,
            const std::vector<HoleEndCandidate>& endCandidates,
            const std::vector<HoleSegmentCandidate>& segmentCandidates,
            const std::vector<HoleCandidate>& holeCandidates);

        std::vector<Hole::Data> build() const;

    private:
        Hole::Data buildHoleData(
            int holeIndex,
            const HoleCandidate& candidate) const;

        Hole::Element buildElement(
            int elementIndex,
            int segmentCandidateIndex) const;

        Hole::Wall buildWall(
            const HoleWallCandidate& wallCandidate) const;

        Hole::End buildEnd(
            const HoleEndCandidate& endCandidate) const;

        std::vector<Hole::ElementConnection> buildElementConnections(
            const HoleCandidate& candidate,
            const std::vector<Hole::Element>& elements) const;

        Hole::ElementConnection buildElementConnection(
            const HoleReachability& reachability,
            const std::vector<Hole::Element>& elements) const;

        Hole::Type classifyHole(
            const HoleCandidate& candidate) const;

        int countOpenEnds(
            const HoleCandidate& candidate) const;

        Hole::EndType toHoleEndType(
            HoleEndCandidateType type) const;

        Hole::ElementConnectionType toElementConnectionType(
            HoleReachabilityReason reason) const;

        int findElementIndexBySegmentCandidateIndex(
            const std::vector<Hole::Element>& elements,
            int segmentCandidateIndex) const;

        int findEndIndexBySourceEndCandidateIndex(
            const Hole::Element& element,
            int endCandidateIndex) const;

        bool isValidWallIndex(int index) const;
        bool isValidEndIndex(int index) const;
        bool isValidSegmentIndex(int index) const;

    private:
        const std::vector<HoleWallCandidate>& m_wallCandidates;
        const std::vector<HoleEndCandidate>& m_endCandidates;
        const std::vector<HoleSegmentCandidate>& m_segmentCandidates;
        const std::vector<HoleCandidate>& m_holeCandidates;
};
}

#endif // HOLEDATABUILDER_H
