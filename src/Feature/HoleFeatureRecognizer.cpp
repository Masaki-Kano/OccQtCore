#include <algorithm>
#include <cmath>

#include "Feature/HoleFeatureRecognizer.h"

#include "Core/CollectionUtil.h"
#include "Feature/HoleRecognitionUtil.h"
#include "Geometry/GeometryModel.h"
#include "Geometry/TopologyQuery.h"

namespace
{
    constexpr double SegmentEndAxialTolerance = 1.0e-3;

    int holeEndTypePriority(
        OccQtCore::Feature::HoleEndCandidateType type)
    {
        using OccQtCore::Feature::HoleEndCandidateType;

        switch (type)
        {
        case HoleEndCandidateType::Open:
            return 3;

        case HoleEndCandidateType::WallConnection:
            return 2;

        case HoleEndCandidateType::Bottom:
            return 1;

        case HoleEndCandidateType::Unknown:
        default:
            return 0;
        }
    }

    bool isBetterRepresentativeEnd(
        const OccQtCore::Feature::HoleEndCandidate& current,
        const OccQtCore::Feature::HoleEndCandidate& next)
    {
        return holeEndTypePriority(next.type) >
               holeEndTypePriority(current.type);
    }

    std::vector<int> selectRepresentativeEndIndicesForSegment(
        const std::vector<OccQtCore::Feature::HoleEndCandidate>& endCandidates,
        const std::vector<int>& sourceEndIndices)
    {
        struct EndGroup
        {
            double axialPosition = 0.0;
            int representativeEndIndex = -1;
        };

        std::vector<EndGroup> groups;

        for (int endIndex : sourceEndIndices)
        {
            if (endIndex < 0 ||
                endIndex >= static_cast<int>(endCandidates.size()))
            {
                continue;
            }

            const auto& end = endCandidates[endIndex];

            if (!end.hasAxialPosition)
            {
                continue;
            }

            auto groupIt = std::find_if(
                groups.begin(),
                groups.end(),
                [&](const EndGroup& group)
                {
                    return std::abs(group.axialPosition - end.axialPosition) <
                           SegmentEndAxialTolerance;
                });

            if (groupIt == groups.end())
            {
                groups.push_back(
                    EndGroup{
                             end.axialPosition,
                             endIndex});
                continue;
            }

            const auto& currentRepresentative =
                endCandidates[groupIt->representativeEndIndex];

            if (isBetterRepresentativeEnd(
                    currentRepresentative,
                    end))
            {
                groupIt->representativeEndIndex = endIndex;
            }
        }

        std::sort(
            groups.begin(),
            groups.end(),
            [](const EndGroup& lhs, const EndGroup& rhs)
            {
                return lhs.axialPosition < rhs.axialPosition;
            });

        std::vector<int> selectedEndIndices;

        if (!groups.empty())
        {
            selectedEndIndices.push_back(
                groups.front().representativeEndIndex);
        }

        if (groups.size() >= 2)
        {
            selectedEndIndices.push_back(
                groups.back().representativeEndIndex);
        }

        return selectedEndIndices;
    }

    void countEndTypes(
        const std::vector<OccQtCore::Feature::HoleEndCandidate>& endCandidates,
        const std::vector<int>& endCandidateIndices,
        int& openCount,
        int& bottomCount,
        int& wallConnectionCount)
    {
        openCount = 0;
        bottomCount = 0;
        wallConnectionCount = 0;

        for (int endIndex : endCandidateIndices)
        {
            if (endIndex < 0 ||
                endIndex >= static_cast<int>(endCandidates.size()))
            {
                continue;
            }

            const auto& end = endCandidates[endIndex];

            if (end.type == OccQtCore::Feature::HoleEndCandidateType::Open)
            {
                ++openCount;
            }
            else if (end.type == OccQtCore::Feature::HoleEndCandidateType::Bottom)
            {
                ++bottomCount;
            }
            else if (end.type == OccQtCore::Feature::HoleEndCandidateType::WallConnection)
            {
                ++wallConnectionCount;
            }
        }
    }
}

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
            buildHoleCandidates(
                model,
                wallCandidates,
                endCandidates,
                segmentCandidates);

        (void)holeCandidates;

        return {};
    }

    std::vector<HoleWallCandidate> HoleFeatureRecognizer::detectWallCandidates(
        const GeometryModel& model) const
    {
        std::vector<HoleWallCandidate> rawCandidates;

        // 1. 円筒Faceを1枚単位の穴壁候補として抽出する。
        for (const auto& face : model.faces())
        {
            if (face.info.kind != SurfaceKind::Cylinder)
            {
                continue;
            }

            if (!face.info.cylinder.has_value())
            {
                continue;
            }

            const auto& cylinder = face.info.cylinder.value();

            HoleWallCandidate candidate;
            candidate.index = static_cast<int>(rawCandidates.size());

            OccQtCore::CollectionUtil::addUnique(
                candidate.geometryRefs.faceIndices,
                face.index);

            candidate.center = cylinder.axis.Location();
            candidate.axisDirection = cylinder.axis.Direction();
            candidate.radius = cylinder.radius;
            candidate.depth = 0.0;

            rawCandidates.push_back(candidate);
        }

        std::vector<HoleWallCandidate> wallCandidates;
        std::vector<bool> used(rawCandidates.size(), false);

        // 2. 同軸・同半径・トポロジー的に接続している円筒Face群をまとめる。
        for (int baseCandidateIndex = 0;
             baseCandidateIndex < static_cast<int>(rawCandidates.size());
             ++baseCandidateIndex)
        {
            if (used[baseCandidateIndex])
            {
                continue;
            }

            const auto& baseCandidate =
                rawCandidates[baseCandidateIndex];

            std::vector<int> groupCandidateIndices;
            std::vector<int> stack;

            used[baseCandidateIndex] = true;
            stack.push_back(baseCandidateIndex);

            while (!stack.empty())
            {
                const int currentCandidateIndex = stack.back();
                stack.pop_back();

                groupCandidateIndices.push_back(currentCandidateIndex);

                const auto& currentCandidate =
                    rawCandidates[currentCandidateIndex];

                for (int nextCandidateIndex = 0;
                     nextCandidateIndex < static_cast<int>(rawCandidates.size());
                     ++nextCandidateIndex)
                {
                    if (used[nextCandidateIndex])
                    {
                        continue;
                    }

                    const auto& nextCandidate =
                        rawCandidates[nextCandidateIndex];

                    if (!HoleRecognitionUtil::isSameCylinderCandidate(
                            currentCandidate,
                            nextCandidate))
                    {
                        continue;
                    }

                    if (currentCandidate.geometryRefs.faceIndices.empty() ||
                        nextCandidate.geometryRefs.faceIndices.empty())
                    {
                        continue;
                    }

                    const int currentFaceIndex =
                        currentCandidate.geometryRefs.faceIndices.front();

                    const int nextFaceIndex =
                        nextCandidate.geometryRefs.faceIndices.front();

                    if (!TopologyQuery::hasSharedEdge(
                            model,
                            currentFaceIndex,
                            nextFaceIndex))
                    {
                        continue;
                    }

                    used[nextCandidateIndex] = true;
                    stack.push_back(nextCandidateIndex);
                }
            }

            if (!HoleRecognitionUtil::isClosedCylinderWallGroup(
                    model,
                    rawCandidates,
                    groupCandidateIndices))
            {
                continue;
            }

            HoleWallCandidate wallCandidate;

            for (int candidateIndex : groupCandidateIndices)
            {
                for (int faceIndex :
                     rawCandidates[candidateIndex].geometryRefs.faceIndices)
                {
                    OccQtCore::CollectionUtil::addUnique(
                        wallCandidate.geometryRefs.faceIndices,
                        faceIndex);
                }
            }

            wallCandidate.center = baseCandidate.center;
            wallCandidate.axisDirection = baseCandidate.axisDirection;
            wallCandidate.radius = baseCandidate.radius;
            wallCandidate.depth = 0.0;

            if (!HoleRecognitionUtil::isLikelyHoleWallByOrientation(
                    model,
                    wallCandidate))
            {
                continue;
            }

            wallCandidate.index = static_cast<int>(wallCandidates.size());
            wallCandidates.push_back(wallCandidate);
        }

        return wallCandidates;
    }

    std::vector<HoleEndCandidate> HoleFeatureRecognizer::detectEndCandidates(
        const GeometryModel& model,
        const std::vector<HoleWallCandidate>& wallCandidates) const
    {
        std::vector<HoleEndCandidate> candidates;

        for (const auto& wallCandidate : wallCandidates)
        {
            const auto connections =
                TopologyQuery::collectBoundaryConnectionsOfFaceGroup(
                    model,
                    wallCandidate.geometryRefs.faceIndices);

            for (const auto& connection : connections)
            {
                const auto candidateOpt =
                    HoleRecognitionUtil::buildEndCandidateFromWallConnection(
                        model,
                        wallCandidate,
                        wallCandidates,
                        connection.adjacentFaceIndex,
                        connection.boundaryEdgeIndex);

                if (!candidateOpt.has_value())
                {
                    continue;
                }

                HoleRecognitionUtil::appendOrMergeEndCandidate(
                    candidates,
                    candidateOpt.value());
            }
        }

        for (int i = 0; i < static_cast<int>(candidates.size()); ++i)
        {
            candidates[i].index = i;
        }

        return candidates;
    }

    std::vector<HoleSegmentCandidate> HoleFeatureRecognizer::buildSegmentCandidates(
        const GeometryModel& model,
        const std::vector<HoleWallCandidate>& wallCandidates,
        const std::vector<HoleEndCandidate>& endCandidates) const
    {
        (void)model;

        std::vector<HoleSegmentCandidate> candidates;

        for (const auto& wallCandidate : wallCandidates)
        {
            HoleSegmentCandidate candidate;

            candidate.index = static_cast<int>(candidates.size());
            candidate.wallCandidateIndex = wallCandidate.index;

            for (const auto& endCandidate : endCandidates)
            {
                if (endCandidate.wallCandidateIndex != wallCandidate.index)
                {
                    continue;
                }

                OccQtCore::CollectionUtil::addUnique(
                    candidate.endCandidateIndices,
                    endCandidate.index);
            }

            if (candidate.endCandidateIndices.empty())
            {
                continue;
            }

            candidate.endCandidateIndices =
                selectRepresentativeEndIndicesForSegment(
                    endCandidates,
                    candidate.endCandidateIndices);

            if (candidate.endCandidateIndices.empty())
            {
                continue;
            }

            int openCount = 0;
            int bottomCount = 0;
            int wallConnectionCount = 0;

            countEndTypes(
                endCandidates,
                candidate.endCandidateIndices,
                openCount,
                bottomCount,
                wallConnectionCount);

            if (openCount == 2 &&
                bottomCount == 0 &&
                wallConnectionCount == 0)
            {
                candidate.type = Hole::Type::SimpleThrough;
            }
            else if (openCount == 1 &&
                     bottomCount == 1 &&
                     wallConnectionCount == 0)
            {
                candidate.type = Hole::Type::SimpleBlind;
            }
            else
            {
                candidate.type = Hole::Type::Unknown;
            }

            candidates.push_back(candidate);
        }

        return candidates;
    }

    std::vector<HoleCandidate> HoleFeatureRecognizer::buildHoleCandidates(
        const GeometryModel& model,
        const std::vector<HoleWallCandidate>& wallCandidates,
        const std::vector<HoleEndCandidate>& endCandidates,
        const std::vector<HoleSegmentCandidate>& segmentCandidates) const
    {
        (void)model;
        (void)wallCandidates;
        (void)endCandidates;

        std::vector<HoleCandidate> candidates;

        for (const auto& segmentCandidate : segmentCandidates)
        {
            HoleCandidate candidate;

            candidate.index = static_cast<int>(candidates.size());

            OccQtCore::CollectionUtil::addUnique(
                candidate.segmentCandidateIndices,
                segmentCandidate.index);

            candidate.type = segmentCandidate.type;

            candidates.push_back(candidate);
        }

        return candidates;
    }
}
