#include <algorithm>

#include "Feature/HoleCandidateBuilder.h"
#include "Geometry/SurfaceUtil.h"

namespace
{
    constexpr double AxisLineTolerance = 1.0e-4;
    constexpr double DirectionTolerance = 1.0e-6;
    constexpr double AxialRangeGapTolerance = 0.6;

    bool containsIndex(
        const std::vector<int>& indices,
        int target)
    {
        return std::find(
                   indices.begin(),
                   indices.end(),
                   target) != indices.end();
    }

    bool hasSharedIndex(
        const std::vector<int>& lhs,
        const std::vector<int>& rhs)
    {
        for (const int index : lhs)
        {
            if (containsIndex(rhs, index))
            {
                return true;
            }
        }

        return false;
    }
}

namespace OccQtCore::Feature
{
    HoleCandidateBuilder::HoleCandidateBuilder(
        const std::vector<HoleWallCandidate>& wallCandidates,
        const std::vector<HoleEndCandidate>& endCandidates,
        const std::vector<HoleSegmentCandidate>& segmentCandidates)
        : m_wallCandidates(wallCandidates)
        , m_endCandidates(endCandidates)
        , m_segmentCandidates(segmentCandidates)
    {
    }

    std::vector<HoleCandidate> HoleCandidateBuilder::build() const
    {
        std::vector<HoleCandidate> candidates;

        const auto sameAxisGroup =
            buildSameAxisSegmentGroups();

        for (const auto& group : sameAxisGroup)
        {
            const auto candidateAxis =
                buildCandidateAxis(group);

            if (!candidateAxis.isValid)
            {
                continue;
            }

            auto segmentRanges = buildSegmentRanges(group, candidateAxis);

            if (segmentRanges.empty())
            {
                continue;
            }

            std::sort(
                segmentRanges.begin(),
                segmentRanges.end(),
                [](const SegmentRange& lhs, const SegmentRange& rhs)
                {
                    return lhs.minAxial < rhs.minAxial;
                });

            const auto connections = buildSegmentConnections(segmentRanges);

            const auto chains = buildSegmentChains(segmentRanges, connections);

            for (const auto& chain : chains)
            {
                candidates.push_back(buildCandidateFromChain(static_cast<int>(candidates.size()), chain));
            }
        }

        return candidates;
    }

    std::vector<std::vector<int>> HoleCandidateBuilder::buildSameAxisSegmentGroups() const
    {
        std::vector<std::vector<int>> groups;

        for (const auto& segment : m_segmentCandidates)
        {
            if (!isValidSegmentIndex(segment.index))
            {
                continue;
            }

            bool merged = false;

            for (auto& group : groups)
            {
                if (group.empty())
                {
                    continue;
                }

                const int representativeSegmentIndex = group.front();

                if (!isValidSegmentIndex(representativeSegmentIndex))
                {
                    continue;
                }

                const auto& representativeSegment =
                    m_segmentCandidates[representativeSegmentIndex];

                if (isSameAxisSegment(
                        representativeSegment,
                        segment))
                {
                    group.push_back(segment.index);
                    merged = true;
                    break;
                }
            }

            if (!merged)
            {
                groups.push_back({ segment.index });
            }
        }

        return groups;
    }

    HoleCandidateBuilder::CandidateAxis HoleCandidateBuilder::buildCandidateAxis(const std::vector<int>& segmentCandidateIndices) const
    {
        CandidateAxis axis;

        for (const int segmentIndex : segmentCandidateIndices)
        {
            if (!isValidSegmentIndex(segmentIndex))
            {
                continue;
            }

            const auto& segment = m_segmentCandidates[segmentIndex];

            if (!isValidWallIndex(segment.wallCandidateIndex))
            {
                continue;
            }

            const auto& wall = m_wallCandidates[segment.wallCandidateIndex];

            axis.point = wall.center;
            axis.direction = wall.axisDirection;
            axis.isValid = true;
            break;
        }

        if (!axis.isValid)
        {
            return axis;
        }

        for (const int segmentIndex : segmentCandidateIndices)
        {
            if (!isValidSegmentIndex(segmentIndex))
            {
                axis.isValid = false;
                return axis;
            }

            const auto& segment = m_segmentCandidates[segmentIndex];

            if (!isValidWallIndex(segment.wallCandidateIndex))
            {
                axis.isValid = false;
                return axis;
            }

            const auto& wall = m_wallCandidates[segment.wallCandidateIndex];

            if (!isWallOnCandidateAxis(axis, wall))
            {
                axis.isValid = false;
                return axis;
            }
        }

        return axis;
    }

    HoleCandidateBuilder::SegmentRange HoleCandidateBuilder::buildSegmentRange(
        const HoleSegmentCandidate& segment,
        const CandidateAxis& candidateAxis) const
    {
        SegmentRange range;
        range.segmentCandidateIndex = segment.index;

        if (!candidateAxis.isValid)
        {
            return range;
        }

        bool hasValue = false;

        for (const int endIndex : segment.endCandidateIndices)
        {
            if (!isValidEndIndex(endIndex))
            {
                continue;
            }

            const auto& end = m_endCandidates[endIndex];

            const double axial = SurfaceUtil::projectPointToAxis(
                candidateAxis.point,
                candidateAxis.direction,
                end.center);

            if (!hasValue)
            {
                range.minAxial = axial;
                range.maxAxial = axial;
                range.minEndCandidateIndex = endIndex;
                range.maxEndCandidateIndex = endIndex;
                hasValue = true;
                continue;
            }

            if (axial < range.minAxial)
            {
                range.minAxial = axial;
                range.minEndCandidateIndex = endIndex;
            }

            if (axial > range.maxAxial)
            {
                range.maxAxial = axial;
                range.maxEndCandidateIndex = endIndex;
            }
        }

        range.isValid = hasValue;
        return range;
    }

    std::vector<HoleCandidateBuilder::SegmentRange> HoleCandidateBuilder::buildSegmentRanges(
        const std::vector<int>& segmentCandidateIndices,
        const CandidateAxis& candidateAxis) const
    {
        std::vector<SegmentRange> ranges;

        for (const int segmentIndex : segmentCandidateIndices)
        {
            if (!isValidSegmentIndex(segmentIndex))
            {
                continue;
            }

            auto range =
                buildSegmentRange(
                    m_segmentCandidates[segmentIndex],
                    candidateAxis);

            if (range.isValid)
            {
                ranges.push_back(range);
            }
        }

        return ranges;
    }

    std::vector<HoleCandidateBuilder::SegmentConnection> HoleCandidateBuilder::buildSegmentConnections(
        const std::vector<SegmentRange>& segmentRanges) const
    {
        std::vector<SegmentConnection> connections;

        if (segmentRanges.size() < 2)
        {
            return connections;
        }

        connections.reserve(segmentRanges.size() - 1);

        for (int i = 0;
             i + 1 < static_cast<int>(segmentRanges.size());
             ++i)
        {
            connections.push_back(
                buildSegmentConnection(segmentRanges[i], segmentRanges[i + 1]));
        }

        return connections;
    }

    HoleCandidateBuilder::SegmentConnection HoleCandidateBuilder::buildSegmentConnection(
        const SegmentRange& currentRange,
        const SegmentRange& nextRange) const
    {
        SegmentConnection connection;

        connection.currentSegmentCandidateIndex =
            currentRange.segmentCandidateIndex;
        connection.nextSegmentCandidateIndex =
            nextRange.segmentCandidateIndex;

        connection.currentEndCandidateIndex =
            currentRange.maxEndCandidateIndex;
        connection.nextEndCandidateIndex =
            nextRange.minEndCandidateIndex;

        connection.axialRangeGap =
            nextRange.minAxial - currentRange.maxAxial;

        connection.kind =
            classifySegmentConnection(
                currentRange,
                nextRange);

        return connection;
    }

    HoleCandidateBuilder::SegmentConnectionKind HoleCandidateBuilder::classifySegmentConnection(
        const SegmentRange& currentRange,
        const SegmentRange& nextRange) const
    {
        if (!currentRange.isValid || !nextRange.isValid)
        {
            return SegmentConnectionKind::Unknown;
        }

        const int currentEndIndex = currentRange.maxEndCandidateIndex;
        const int nextEndIndex = nextRange.minEndCandidateIndex;

        if (!isValidEndIndex(currentEndIndex) ||
            !isValidEndIndex(nextEndIndex))
        {
            return SegmentConnectionKind::Unknown;
        }

        const auto& currentEnd = m_endCandidates[currentEndIndex];
        const auto& nextEnd = m_endCandidates[nextEndIndex];

        if (hasSharedEndGeometry(currentEnd, nextEnd))
        {
            return SegmentConnectionKind::SharedEndGeometry;
        }

        const double axialRangeGap = nextRange.minAxial - currentRange.maxAxial;

        if (axialRangeGap >= 0 &&
            axialRangeGap <= AxialRangeGapTolerance)
        {
            return SegmentConnectionKind::AxialRangeNear;
        }

        return SegmentConnectionKind::Unknown;
    }

    std::vector<HoleCandidateBuilder::SegmentChain> HoleCandidateBuilder::buildSegmentChains(
        const std::vector<SegmentRange>& segmentRanges,
        const std::vector<SegmentConnection>& connections) const
    {
        std::vector<SegmentChain> chains;

        if (segmentRanges.empty())
        {
            return chains;
        }

        SegmentChain currentChain;
        currentChain.index = 0;
        currentChain.segmentCandidateIndices.push_back(
            segmentRanges.front().segmentCandidateIndex);

        for (const auto& connection : connections)
        {
            const bool isConnected =
                connection.kind == SegmentConnectionKind::SharedEndGeometry ||
                connection.kind == SegmentConnectionKind::ShoulderPlane ||
                connection.kind == SegmentConnectionKind::AxialRangeNear;

            if (isConnected)
            {
                currentChain.segmentCandidateIndices.push_back(
                    connection.nextSegmentCandidateIndex);
                continue;
            }

            chains.push_back(currentChain);

            currentChain = SegmentChain{};
            currentChain.index = static_cast<int>(chains.size());
            currentChain.segmentCandidateIndices.push_back(
                connection.nextSegmentCandidateIndex);
        }

        chains.push_back(currentChain);

        return chains;
    }

    HoleCandidate HoleCandidateBuilder::buildCandidateFromChain(
        int candidateIndex,
        const SegmentChain& chain) const
    {
        HoleCandidate candidate;

        candidate.index = candidateIndex;
        candidate.segmentCandidateIndices = chain.segmentCandidateIndices;

        return candidate;
    }

    bool HoleCandidateBuilder::isSameAxisSegment(
        const HoleSegmentCandidate& lhs,
        const HoleSegmentCandidate& rhs) const
    {
        if (!isValidWallIndex(lhs.wallCandidateIndex) ||
            !isValidWallIndex(rhs.wallCandidateIndex))
        {
            return false;
        }

        const auto& lhsWall = m_wallCandidates[lhs.wallCandidateIndex];

        const auto& rhsWall = m_wallCandidates[rhs.wallCandidateIndex];

        return SurfaceUtil::isSameAxis(
            lhsWall.center,
            lhsWall.axisDirection,
            rhsWall.center,
            rhsWall.axisDirection,
            AxisLineTolerance,
            DirectionTolerance);
    }

    bool HoleCandidateBuilder::isWallOnCandidateAxis(
        const CandidateAxis& candidateAxis,
        const HoleWallCandidate& wallCandidate) const
    {
        if (!candidateAxis.isValid)
        {
            return false;
        }

        return SurfaceUtil::isSameAxis(
            candidateAxis.point,
            candidateAxis.direction,
            wallCandidate.center,
            wallCandidate.axisDirection,
            AxisLineTolerance,
            DirectionTolerance);
    }

    bool HoleCandidateBuilder::isValidSegmentIndex(int index) const
    {
        return index >= 0 &&
               index < static_cast<int>(m_segmentCandidates.size());
    }

    bool HoleCandidateBuilder::isValidWallIndex(int index) const
    {
        return index >= 0 &&
               index < static_cast<int>(m_wallCandidates.size());
    }

    bool HoleCandidateBuilder::isValidEndIndex(int index) const
    {
        return index >= 0 &&
               index < static_cast<int>(m_endCandidates.size());
    }

    bool HoleCandidateBuilder::hasSharedEndGeometry(
        const HoleEndCandidate& lhs,
        const HoleEndCandidate& rhs) const
    {
        if (hasSharedIndex(
                lhs.geometryRefs.faceIndices,
                rhs.geometryRefs.faceIndices))
        {
            return true;
        }

        if (hasSharedIndex(
                lhs.geometryRefs.edgeIndices,
                rhs.geometryRefs.edgeIndices))
        {
            return true;
        }

        return false;
    }
}
