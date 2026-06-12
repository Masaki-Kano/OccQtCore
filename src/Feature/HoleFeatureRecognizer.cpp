#include "Feature/HoleFeatureRecognizer.h"
#include "Geometry/GeometryModel.h"
#include "Geometry/TopologyQuery.h"

#include "Feature/HoleWallCandidateDetector.h"
#include "Feature/HoleEndCandidateDetector.h"
#include "Feature/HoleCandidateBuilder.h"
#include "Feature/HoleSegmentCandidateBuilder.h"
#include "Feature/HoleDataBuilder.h"

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
                model,
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

    HoleRecognitionResult HoleFeatureRecognizer::recognizeCandidates(const GeometryModel& model) const
    {
        HoleRecognitionResult result;

        result.wallCandidates = detectWallCandidates(model);

        result.endCandidates = detectEndCandidates(model, result.wallCandidates);

        result.segmentCandidates = buildSegmentCandidates(model, result.wallCandidates, result.endCandidates);

        result.holeCandidates = buildHoleCandidatesFromSegments(model, result.wallCandidates, result.endCandidates, result.segmentCandidates);

        result.holes = buildHoleDataFromCandidates(result.wallCandidates, result.endCandidates, result.segmentCandidates, result.holeCandidates);

        return result;
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
        HoleSegmentCandidateBuilder builder(
            model,
            wallCandidates,
            endCandidates);

        return builder.build();
    }

    std::vector<HoleCandidate>
    HoleFeatureRecognizer::buildHoleCandidatesFromSegments(
        const GeometryModel& model,
        const std::vector<HoleWallCandidate>& wallCandidates,
        const std::vector<HoleEndCandidate>& endCandidates,
        const std::vector<HoleSegmentCandidate>& segmentCandidates) const
    {
        HoleCandidateBuilder builder(
            model,
            wallCandidates,
            endCandidates,
            segmentCandidates);

        return builder.build();
    }

    std::vector<Hole::Data> HoleFeatureRecognizer::buildHoleDataFromCandidates(
        const std::vector<HoleWallCandidate>& wallCandidates,
        const std::vector<HoleEndCandidate>& endCandidates,
        const std::vector<HoleSegmentCandidate>& segmentCandidates,
        const std::vector<HoleCandidate>& holeCandidates) const
    {
        HoleDataBuilder builder(
            wallCandidates,
            endCandidates,
            segmentCandidates,
            holeCandidates);

        return builder.build();
    }
}
