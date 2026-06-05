#include "Feature/HoleSegmentCandidateBuilder.h"

#include <algorithm>
#include <cmath>

#include "Core/CollectionUtil.h"

namespace
{
    constexpr double SegmentEndAxialTolerance = 1.0e-3;
}

namespace OccQtCore::Feature
{
    HoleSegmentCandidateBuilder::HoleSegmentCandidateBuilder(
        const std::vector<HoleWallCandidate>& wallCandidates,
        const std::vector<HoleEndCandidate>& endCandidates)
        : m_wallCandidates(wallCandidates)
        , m_endCandidates(endCandidates)
    {
    }

    std::vector<HoleSegmentCandidate> HoleSegmentCandidateBuilder::build() const
    {
        std::vector<HoleSegmentCandidate> candidates;

        for (const auto& wallCandidate : m_wallCandidates)
        {
            HoleSegmentCandidate candidate;

            candidate.index = static_cast<int>(candidates.size());
            candidate.wallCandidateIndex = wallCandidate.index;

            for (const auto& endCandidate : m_endCandidates)
            {
                if (endCandidate.wallCandidateIndex != wallCandidate.index)
                {
                    continue;
                }

                OccQtCore::CollectionUtil::addUnique(candidate.endCandidateIndices, endCandidate.index);
            }

            if (candidate.endCandidateIndices.empty())
            {
                continue;
            }

            candidate.endCandidateIndices =
                selectRepresentativeEndIndices(candidate.endCandidateIndices);

            if (candidate.endCandidateIndices.empty())
            {
                continue;
            }

            candidate.type = classifySegmentType(candidate.endCandidateIndices);

            candidates.push_back(candidate);
        }

        return candidates;
    }

    int HoleSegmentCandidateBuilder::holeEndTypePriority(HoleEndCandidateType type) const
    {
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

    bool HoleSegmentCandidateBuilder::isBetterRepresentativeEnd(
        const HoleEndCandidate& current,
        const HoleEndCandidate& next) const
    {
        return holeEndTypePriority(next.type) > holeEndTypePriority(current.type);
    }

    std::vector<int> HoleSegmentCandidateBuilder::selectRepresentativeEndIndices(const std::vector<int>& sourceEndIndices) const
    {
        struct EndGroup
        {
            double axialPosition = 0.0;
            int representativeEndIndex = -1;
        };

        std::vector<EndGroup> groups;

        for (int endIndex : sourceEndIndices)
        {
            if (!isValidEndIndex(endIndex))
            {
                continue;
            }

            const auto& end = m_endCandidates[endIndex];

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
                             endIndex });

                continue;
            }

            const auto& currentRepresentative =
                m_endCandidates[groupIt->representativeEndIndex];

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

    void HoleSegmentCandidateBuilder::countEndTypes(
        const std::vector<int>& endCandidateIndices,
        int& openCount,
        int& bottomCount,
        int& wallConnectionCount) const
    {
        openCount = 0;
        bottomCount = 0;
        wallConnectionCount = 0;

        for (int endIndex : endCandidateIndices)
        {
            if (!isValidEndIndex(endIndex))
            {
                continue;
            }

            const auto& end = m_endCandidates[endIndex];

            if (end.type == HoleEndCandidateType::Open)
            {
                ++openCount;
            }
            else if (end.type == HoleEndCandidateType::Bottom)
            {
                ++bottomCount;
            }
            else if (end.type == HoleEndCandidateType::WallConnection)
            {
                ++wallConnectionCount;
            }
        }
    }

    Hole::Type HoleSegmentCandidateBuilder::classifySegmentType(
        const std::vector<int>& endCandidateIndices) const
    {
        int openCount = 0;
        int bottomCount = 0;
        int wallConnectionCount = 0;

        countEndTypes(
            endCandidateIndices,
            openCount,
            bottomCount,
            wallConnectionCount);

        if (openCount == 2 &&
            bottomCount == 0 &&
            wallConnectionCount == 0)
        {
            return Hole::Type::SimpleThrough;
        }

        if (openCount == 1 &&
            bottomCount == 1 &&
            wallConnectionCount == 0)
        {
            return Hole::Type::SimpleBlind;
        }

        return Hole::Type::Unknown;
    }

    bool HoleSegmentCandidateBuilder::isValidEndIndex(int index) const
    {
        return index >= 0 &&
               index < static_cast<int>(m_endCandidates.size());
    }
}
