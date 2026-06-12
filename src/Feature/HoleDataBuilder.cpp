#include "Feature/HoleDataBuilder.h"
#include "Core/CollectionUtil.h"

namespace OccQtCore::Feature
{
    HoleDataBuilder::HoleDataBuilder(
        const std::vector<HoleWallCandidate>& wallCandidates,
        const std::vector<HoleEndCandidate>& endCandidates,
        const std::vector<HoleSegmentCandidate>& segmentCandidates,
        const std::vector<HoleCandidate>& holeCandidates)
        : m_wallCandidates(wallCandidates)
        , m_endCandidates(endCandidates)
        , m_segmentCandidates(segmentCandidates)
        , m_holeCandidates(holeCandidates)
    {
    }

    std::vector<Hole::Data> HoleDataBuilder::build() const
    {
        std::vector<Hole::Data> holes;

        for (const auto& candidate : m_holeCandidates)
        {
            holes.push_back(
                buildHoleData(
                    static_cast<int>(holes.size()),
                    candidate));
        }

        return holes;
    }

    Hole::Data HoleDataBuilder::buildHoleData(
        int holeIndex,
        const HoleCandidate& candidate) const
    {
        Hole::Data data;

        data.index = holeIndex;
        data.sourceHoleCandidateIndex = candidate.index;
        data.featureType = Feature::Type::Hole;
        data.holeType = classifyHole(candidate);

        data.axisPoint = candidate.axisPoint;
        data.axisDirection = candidate.axisDirection;

        for (const int segmentCandidateIndex : candidate.segmentCandidateIndices)
        {
            if (!isValidSegmentIndex(segmentCandidateIndex))
            {
                continue;
            }

            data.elements.push_back(
                buildElement(
                    static_cast<int>(data.elements.size()),
                    segmentCandidateIndex));
        }

        data.elementConnections =
            buildElementConnections(
                candidate,
                data.elements);

        return data;
    }

    Hole::Element HoleDataBuilder::buildElement(
        int elementIndex,
        int segmentCandidateIndex) const
    {
        Hole::Element element;

        if (!isValidSegmentIndex(segmentCandidateIndex))
        {
            return element;
        }

        const auto& segment =
            m_segmentCandidates[segmentCandidateIndex];

        element.index = elementIndex;
        element.sourceSegmentCandidateIndex = segmentCandidateIndex;

        if (isValidWallIndex(segment.wallCandidateIndex))
        {
            element.wall =
                buildWall(
                    m_wallCandidates[segment.wallCandidateIndex]);
        }

        for (const int endCandidateIndex : segment.endCandidateIndices)
        {
            if (!isValidEndIndex(endCandidateIndex))
            {
                continue;
            }

            element.ends.push_back(
                buildEnd(
                    m_endCandidates[endCandidateIndex]));
        }

        element.depth = segment.depth;

        return element;
    }

    Hole::Wall HoleDataBuilder::buildWall(
        const HoleWallCandidate& wallCandidate) const
    {
        Hole::Wall wall;

        wall.geometryRefs = wallCandidate.geometryRefs;
        wall.center = wallCandidate.center;
        wall.axisDirection = wallCandidate.axisDirection;
        wall.radius = wallCandidate.radius;

        return wall;
    }

    Hole::End HoleDataBuilder::buildEnd(
        const HoleEndCandidate& endCandidate) const
    {
        Hole::End end;

        end.sourceEndCandidateIndex = endCandidate.index;
        end.geometryRefs = endCandidate.geometryRefs;
        end.endType = toHoleEndType(endCandidate.type);

        return end;
    }

    std::vector<Hole::ElementConnection> HoleDataBuilder::buildElementConnections(
        const HoleCandidate& candidate,
        const std::vector<Hole::Element>& elements) const
    {
        std::vector<Hole::ElementConnection> connections;

        for (const auto& reachability : candidate.reachabilities)
        {
            const auto connection =
                buildElementConnection(
                    reachability,
                    elements);

            if (connection.lhsElementIndex < 0 ||
                connection.rhsElementIndex < 0 ||
                connection.lhsEndIndex < 0 ||
                connection.rhsEndIndex < 0 ||
                connection.type == Hole::ElementConnectionType::Unknown)
            {
                continue;
            }

            connections.push_back(connection);
        }

        return connections;
    }

    Hole::ElementConnection HoleDataBuilder::buildElementConnection(
        const HoleReachability& reachability,
        const std::vector<Hole::Element>& elements) const
    {
        Hole::ElementConnection connection;

        connection.lhsElementIndex =
            findElementIndexBySegmentCandidateIndex(
                elements,
                reachability.lhsSegmentCandidateIndex);

        connection.rhsElementIndex =
            findElementIndexBySegmentCandidateIndex(
                elements,
                reachability.rhsSegmentCandidateIndex);

        if (connection.lhsElementIndex < 0 ||
            connection.rhsElementIndex < 0)
        {
            return connection;
        }

        const auto& lhsElement =
            elements[connection.lhsElementIndex];

        const auto& rhsElement =
            elements[connection.rhsElementIndex];

        connection.lhsEndIndex =
            findEndIndexBySourceEndCandidateIndex(
                lhsElement,
                reachability.lhsEndCandidateIndex);

        connection.rhsEndIndex =
            findEndIndexBySourceEndCandidateIndex(
                rhsElement,
                reachability.rhsEndCandidateIndex);

        connection.type =
            toElementConnectionType(
                reachability.reason);

        connection.geometryRefs =
            reachability.sharedGeometryRefs;

        return connection;
    }

    Hole::Type HoleDataBuilder::classifyHole(
        const HoleCandidate& candidate) const
    {
        if (candidate.segmentCandidateIndices.empty())
        {
            return Hole::Type::Unknown;
        }

        if (candidate.segmentCandidateIndices.size() >= 2)
        {
            return Hole::Type::Complex;
        }

        if (countOpenEnds(candidate) >= 2)
        {
            return Hole::Type::SimpleThrough;
        }

        return Hole::Type::SimpleBlind;
    }

    int HoleDataBuilder::countOpenEnds(
        const HoleCandidate& candidate) const
    {
        std::vector<int> endCandidateIndices;

        for (const int segmentCandidateIndex : candidate.segmentCandidateIndices)
        {
            if (!isValidSegmentIndex(segmentCandidateIndex))
            {
                continue;
            }

            const auto& segment =
                m_segmentCandidates[segmentCandidateIndex];

            for (const int endCandidateIndex : segment.endCandidateIndices)
            {
                if (!isValidEndIndex(endCandidateIndex))
                {
                    continue;
                }

                CollectionUtil::addUnique(
                    endCandidateIndices,
                    endCandidateIndex);
            }
        }

        int openEndCount = 0;

        for (const int endCandidateIndex : endCandidateIndices)
        {
            const auto& end =
                m_endCandidates[endCandidateIndex];

            if (end.type == HoleEndCandidateType::Open)
            {
                ++openEndCount;
            }
        }

        return openEndCount;
    }

    Hole::EndType HoleDataBuilder::toHoleEndType(
        HoleEndCandidateType type) const
    {
        switch (type)
        {
        case HoleEndCandidateType::Open:
            return Hole::EndType::Open;

        case HoleEndCandidateType::Bottom:
            return Hole::EndType::Bottom;

        case HoleEndCandidateType::Connected:
            return Hole::EndType::Connected;

        case HoleEndCandidateType::Unknown:
        default:
            return Hole::EndType::Unknown;
        }
    }

    Hole::ElementConnectionType HoleDataBuilder::toElementConnectionType(
        HoleReachabilityReason reason) const
    {
        switch (reason)
        {
        case HoleReachabilityReason::SharedGeometryRef:
            return Hole::ElementConnectionType::Direct;

        case HoleReachabilityReason::SharedAdjacentFace:
            return Hole::ElementConnectionType::SharedPathFace;

        case HoleReachabilityReason::Unknown:
        default:
            return Hole::ElementConnectionType::Unknown;
        }
    }

    int HoleDataBuilder::findElementIndexBySegmentCandidateIndex(
        const std::vector<Hole::Element>& elements,
        int segmentCandidateIndex) const
    {
        for (int i = 0; i < static_cast<int>(elements.size()); ++i)
        {
            if (elements[i].sourceSegmentCandidateIndex == segmentCandidateIndex)
            {
                return i;
            }
        }

        return -1;
    }

    int HoleDataBuilder::findEndIndexBySourceEndCandidateIndex(
        const Hole::Element& element,
        int endCandidateIndex) const
    {
        for (int i = 0; i < static_cast<int>(element.ends.size()); ++i)
        {
            if (element.ends[i].sourceEndCandidateIndex == endCandidateIndex)
            {
                return i;
            }
        }

        return -1;
    }

    bool HoleDataBuilder::isValidWallIndex(int index) const
    {
        return 0 <= index &&
               index < static_cast<int>(m_wallCandidates.size());
    }

    bool HoleDataBuilder::isValidEndIndex(int index) const
    {
        return 0 <= index &&
               index < static_cast<int>(m_endCandidates.size());
    }

    bool HoleDataBuilder::isValidSegmentIndex(int index) const
    {
        return 0 <= index &&
               index < static_cast<int>(m_segmentCandidates.size());
    }
}
