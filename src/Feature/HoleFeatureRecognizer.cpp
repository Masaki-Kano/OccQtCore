#include "Feature/HoleFeatureRecognizer.h"
#include "Geometry/GeometryModel.h"
#include "Geometry/TopologyQuery.h"

#include "Feature/HoleWallCandidateDetector.h"
#include "Feature/HoleEndCandidateDetector.h"
#include "Feature/HoleCandidateBuilder.h"
#include "Feature/HoleSegmentCandidateBuilder.h"

namespace OccQtCore::Feature
{

    std::vector<Hole::Data> HoleFeatureRecognizer::recognize(
        const GeometryModel& model) const
    {
        const auto wallCandidates =
            detectWallCandidates(model);

        const auto endCandidates =
            detectEndCandidates(
                model,
                wallCandidates);

        const auto segmentCandidates =
            buildSegmentCandidates(
                model,
                wallCandidates,
                endCandidates);

        const auto holeCandidates =
            buildHoleCandidatesFromSegments(
                wallCandidates,
                endCandidates,
                segmentCandidates);

        // TODO:
        // const auto holes =
        //     buildHoleDataFromCandidates(
        //         holeCandidates,
        //         segmentCandidates,
        //         wallCandidates,
        //         endCandidates);

        (void)holeCandidates;

        return {};
    }

    std::vector<HoleWallCandidate> HoleFeatureRecognizer::detectWallCandidates(
        const GeometryModel& model) const
    {
        HoleWallCandidateDetector detector;

        return detector.detect(model);
    }

    std::vector<HoleEndCandidate> HoleFeatureRecognizer::detectEndCandidates(
        const GeometryModel& model,
        const std::vector<HoleWallCandidate>& wallCandidates) const
    {
        HoleEndCandidateDetector detector(
            model,
            wallCandidates);

        return detector.detect();
    }

    std::vector<HoleSegmentCandidate> HoleFeatureRecognizer::buildSegmentCandidates(
        const GeometryModel& model,
        const std::vector<HoleWallCandidate>& wallCandidates,
        const std::vector<HoleEndCandidate>& endCandidates) const
    {
        (void)model;

        HoleSegmentCandidateBuilder builder(
            wallCandidates,
            endCandidates);

        return builder.build();
    }

    std::vector<HoleCandidate>
    HoleFeatureRecognizer::buildHoleCandidatesFromSegments(
        const std::vector<HoleWallCandidate>& wallCandidates,
        const std::vector<HoleEndCandidate>& endCandidates,
        const std::vector<HoleSegmentCandidate>& segmentCandidates) const
    {
        HoleCandidateBuilder builder(
            wallCandidates,
            endCandidates,
            segmentCandidates);

        return builder.build();
    }
}
