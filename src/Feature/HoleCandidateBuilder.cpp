#include "Feature/HoleCandidateBuilder.h"

#include <algorithm>
#include <cmath>

#include "Geometry/GeometryModel.h"
#include "Geometry/SurfaceUtil.h"
#include "Geometry/TopologyQuery.h"
#include "Core/CollectionUtil.h"

namespace
{
    constexpr double AxisLineTolerance = 1.0e-4;
    constexpr double DirectionTolerance = 1.0e-6;
    constexpr double HolePathPlaneAxisDotTolerance = 0.99;

    class DisjointSet
    {
    public:
        explicit DisjointSet(int size)
            : m_parent(size)
            , m_rank(size, 0)
        {
            for (int i = 0; i < size; ++i)
            {
                m_parent[i] = i;
            }
        }

        int find(int value)
        {
            if (m_parent[value] == value)
            {
                return value;
            }

            m_parent[value] = find(m_parent[value]);
            return m_parent[value];
        }

        void unite(int lhs, int rhs)
        {
            int lhsRoot = find(lhs);
            int rhsRoot = find(rhs);

            if (lhsRoot == rhsRoot)
            {
                return;
            }

            if (m_rank[lhsRoot] < m_rank[rhsRoot])
            {
                std::swap(lhsRoot, rhsRoot);
            }

            m_parent[rhsRoot] = lhsRoot;

            if (m_rank[lhsRoot] == m_rank[rhsRoot])
            {
                ++m_rank[lhsRoot];
            }
        }

    private:
        std::vector<int> m_parent;
        std::vector<int> m_rank;
    };
}

namespace OccQtCore::Feature
{
    HoleCandidateBuilder::HoleCandidateBuilder(
        const GeometryModel& model,
        const std::vector<HoleWallCandidate>& wallCandidates,
        const std::vector<HoleEndCandidate>& endCandidates,
        const std::vector<HoleSegmentCandidate>& segmentCandidates)
        : m_model(model)
        , m_wallCandidates(wallCandidates)
        , m_endCandidates(endCandidates)
        , m_segmentCandidates(segmentCandidates)
    {
    }

    std::vector<HoleCandidate> HoleCandidateBuilder::build() const
    {
        std::vector<HoleCandidate> candidates;

        const auto sameAxisGroups = buildSameAxisSegmentGroups();

        for (const auto& sameAxisGroup : sameAxisGroups)
        {
            const auto candidateAxis = buildCandidateAxis(sameAxisGroup);

            if (!candidateAxis.isValid)
            {
                continue;
            }

            std::vector<HoleReachability> reachabilities;

            const auto reachableGroups =
                buildReachableSegmentGroups(
                    sameAxisGroup,
                    candidateAxis,
                    reachabilities);

            for (const auto& reachableGroup : reachableGroups)
            {
                const auto candidateReachabilities =
                    filterReachabilitiesForSegmentGroup(
                        reachableGroup,
                        reachabilities);

                candidates.push_back(
                    buildCandidateFromSegmentGroup(
                        static_cast<int>(candidates.size()),
                        reachableGroup,
                        candidateAxis,
                        candidateReachabilities));
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

                const auto& representativeSegment = m_segmentCandidates[representativeSegmentIndex];

                if (isSameAxisSegment(representativeSegment, segment))
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

    std::vector<std::vector<int>> HoleCandidateBuilder::buildReachableSegmentGroups(
        const std::vector<int>& sameAxisSegmentIndices,
        const CandidateAxis& candidateAxis,
        std::vector<HoleReachability>& reachabilities) const
    {
        std::vector<std::vector<int>> groups;

        if (!candidateAxis.isValid ||
            sameAxisSegmentIndices.empty())
        {
            return groups;
        }

        DisjointSet disjointSet(
            static_cast<int>(sameAxisSegmentIndices.size()));

        for (int i = 0; i < static_cast<int>(sameAxisSegmentIndices.size()); ++i)
        {
            for (int j = i + 1; j < static_cast<int>(sameAxisSegmentIndices.size()); ++j)
            {
                const int lhsSegmentIndex = sameAxisSegmentIndices[i];
                const int rhsSegmentIndex = sameAxisSegmentIndices[j];

                HoleReachability reachability;

                if (tryBuildSegmentReachability(
                        lhsSegmentIndex,
                        rhsSegmentIndex,
                        candidateAxis,
                        reachability))
                {
                    disjointSet.unite(i, j);
                    reachabilities.push_back(reachability);
                }
            }
        }

        std::vector<int> roots;

        for (int i = 0; i < static_cast<int>(sameAxisSegmentIndices.size()); ++i)
        {
            const int root = disjointSet.find(i);

            auto rootIt =
                std::find(
                    roots.begin(),
                    roots.end(),
                    root);

            if (rootIt == roots.end())
            {
                roots.push_back(root);
                groups.push_back({});
                rootIt = roots.end() - 1;
            }

            const int groupIndex =
                static_cast<int>(rootIt - roots.begin());

            groups[groupIndex].push_back(sameAxisSegmentIndices[i]);
        }

        return groups;
    }

    HoleCandidate HoleCandidateBuilder::buildCandidateFromSegmentGroup(
        int candidateIndex,
        const std::vector<int>& segmentCandidateIndices,
        const CandidateAxis& candidateAxis,
        const std::vector<HoleReachability>& reachabilities) const
    {
        HoleCandidate candidate;

        candidate.index = candidateIndex;
        candidate.segmentCandidateIndices = segmentCandidateIndices;

        candidate.axisPoint = candidateAxis.point;
        candidate.axisDirection = candidateAxis.direction;

        candidate.reachabilities = reachabilities;

        return candidate;
    }

    bool HoleCandidateBuilder::tryBuildSegmentReachability(
        int lhsSegmentCandidateIndex,
        int rhsSegmentCandidateIndex,
        const CandidateAxis& candidateAxis,
        HoleReachability& reachability) const
    {
        if (!candidateAxis.isValid ||
            !isValidSegmentIndex(lhsSegmentCandidateIndex) ||
            !isValidSegmentIndex(rhsSegmentCandidateIndex))
        {
            return false;
        }

        if (lhsSegmentCandidateIndex == rhsSegmentCandidateIndex)
        {
            return false;
        }

        const auto& lhsSegment =
            m_segmentCandidates[lhsSegmentCandidateIndex];

        const auto& rhsSegment =
            m_segmentCandidates[rhsSegmentCandidateIndex];

        if (!isValidWallIndex(lhsSegment.wallCandidateIndex) ||
            !isValidWallIndex(rhsSegment.wallCandidateIndex))
        {
            return false;
        }

        const auto& lhsWall =
            m_wallCandidates[lhsSegment.wallCandidateIndex];

        const auto& rhsWall =
            m_wallCandidates[rhsSegment.wallCandidateIndex];

        if (!isWallOnCandidateAxis(candidateAxis, lhsWall) ||
            !isWallOnCandidateAxis(candidateAxis, rhsWall))
        {
            return false;
        }

        for (const int lhsEndIndex : lhsSegment.endCandidateIndices)
        {
            if (!isValidEndIndex(lhsEndIndex))
            {
                continue;
            }

            for (const int rhsEndIndex : rhsSegment.endCandidateIndices)
            {
                if (!isValidEndIndex(rhsEndIndex))
                {
                    continue;
                }

                HoleReachability endReachability;

                if (!tryBuildEndReachability(
                        lhsEndIndex,
                        rhsEndIndex,
                        candidateAxis,
                        endReachability))
                {
                    continue;
                }

                endReachability.lhsSegmentCandidateIndex =
                    lhsSegmentCandidateIndex;

                endReachability.rhsSegmentCandidateIndex =
                    rhsSegmentCandidateIndex;

                reachability = endReachability;
                return true;
            }
        }

        return false;
    }

    bool HoleCandidateBuilder::tryBuildEndReachability(
        int lhsEndCandidateIndex,
        int rhsEndCandidateIndex,
        const CandidateAxis& candidateAxis,
        HoleReachability& reachability) const
    {
        if (!candidateAxis.isValid ||
            !isValidEndIndex(lhsEndCandidateIndex) ||
            !isValidEndIndex(rhsEndCandidateIndex))
        {
            return false;
        }

        if (lhsEndCandidateIndex == rhsEndCandidateIndex)
        {
            return false;
        }

        const auto& lhsEnd =
            m_endCandidates[lhsEndCandidateIndex];

        const auto& rhsEnd =
            m_endCandidates[rhsEndCandidateIndex];

        if (lhsEnd.wallCandidateIndex == rhsEnd.wallCandidateIndex)
        {
            return false;
        }

        reachability = HoleReachability{};
        reachability.lhsEndCandidateIndex = lhsEndCandidateIndex;
        reachability.rhsEndCandidateIndex = rhsEndCandidateIndex;

        if (tryBuildSharedGeometryRefReachability(
                lhsEnd,
                rhsEnd,
                reachability))
        {
            return true;
        }

        if (tryBuildSharedAdjacentFaceReachability(
                lhsEnd,
                rhsEnd,
                candidateAxis,
                reachability))
        {
            return true;
        }

        return false;
    }

    bool HoleCandidateBuilder::tryBuildSharedGeometryRefReachability(
        const HoleEndCandidate& lhsEnd,
        const HoleEndCandidate& rhsEnd,
        HoleReachability& reachability) const
    {
        GeometryRefs sharedRefs;

        for (const int faceIndex : lhsEnd.geometryRefs.faceIndices)
        {
            if (CollectionUtil::contains(
                    rhsEnd.geometryRefs.faceIndices,
                    faceIndex))
            {
                CollectionUtil::addUnique(
                    sharedRefs.faceIndices,
                    faceIndex);
            }
        }

        for (const int edgeIndex : lhsEnd.geometryRefs.edgeIndices)
        {
            if (CollectionUtil::contains(
                    rhsEnd.geometryRefs.edgeIndices,
                    edgeIndex))
            {
                CollectionUtil::addUnique(
                    sharedRefs.edgeIndices,
                    edgeIndex);
            }
        }

        for (const int vertexIndex : lhsEnd.geometryRefs.vertexIndices)
        {
            if (CollectionUtil::contains(
                    rhsEnd.geometryRefs.vertexIndices,
                    vertexIndex))
            {
                CollectionUtil::addUnique(
                    sharedRefs.vertexIndices,
                    vertexIndex);
            }
        }

        if (sharedRefs.faceIndices.empty() &&
            sharedRefs.edgeIndices.empty() &&
            sharedRefs.vertexIndices.empty())
        {
            return false;
        }

        reachability.reason =
            HoleReachabilityReason::SharedGeometryRef;

        reachability.sharedGeometryRefs =
            sharedRefs;

        return true;
    }

    bool HoleCandidateBuilder::tryBuildSharedAdjacentFaceReachability(
        const HoleEndCandidate& lhsEnd,
        const HoleEndCandidate& rhsEnd,
        const CandidateAxis& candidateAxis,
        HoleReachability& reachability) const
    {
        const auto lhsAdjacentFaces =
            collectAdjacentHolePathFaceIndicesOfEnd(
                lhsEnd,
                candidateAxis);

        const auto rhsAdjacentFaces =
            collectAdjacentHolePathFaceIndicesOfEnd(
                rhsEnd,
                candidateAxis);

        GeometryRefs sharedRefs;

        for (const int faceIndex : lhsAdjacentFaces)
        {
            if (CollectionUtil::contains(
                    rhsAdjacentFaces,
                    faceIndex))
            {
                CollectionUtil::addUnique(
                    sharedRefs.faceIndices,
                    faceIndex);
            }
        }

        if (sharedRefs.faceIndices.empty())
        {
            return false;
        }

        reachability.reason =
            HoleReachabilityReason::SharedAdjacentFace;

        reachability.sharedGeometryRefs =
            sharedRefs;

        return true;
    }


    std::vector<int> HoleCandidateBuilder::collectAdjacentHolePathFaceIndicesOfEnd(
        const HoleEndCandidate& end,
        const CandidateAxis& candidateAxis) const
    {
        std::vector<int> faceIndices;

        if (!candidateAxis.isValid ||
            !isValidWallIndex(end.wallCandidateIndex))
        {
            return faceIndices;
        }

        const auto& wall =
            m_wallCandidates[end.wallCandidateIndex];

        for (const int edgeIndex : end.geometryRefs.edgeIndices)
        {
            if (!TopologyQuery::isValidEdgeIndex(m_model, edgeIndex))
            {
                continue;
            }

            const auto connectedFaceIndices =
                TopologyQuery::facesOfEdge(
                    m_model,
                    edgeIndex);

            for (const int faceIndex : connectedFaceIndices)
            {
                if (CollectionUtil::contains(
                        wall.geometryRefs.faceIndices,
                        faceIndex))
                {
                    continue;
                }

                if (!isAllowedHolePathFace(
                        faceIndex,
                        candidateAxis))
                {
                    continue;
                }

                CollectionUtil::addUnique(
                    faceIndices,
                    faceIndex);
            }
        }

        return faceIndices;
    }

    bool HoleCandidateBuilder::isAllowedHolePathFace(
        int faceIndex,
        const CandidateAxis& candidateAxis) const
    {
        if (!candidateAxis.isValid ||
            !TopologyQuery::isValidFaceIndex(m_model, faceIndex))
        {
            return false;
        }

        const auto* face =
            m_model.faceAt(faceIndex);

        if (face == nullptr)
        {
            return false;
        }

        if (face->info.kind != SurfaceKind::Plane ||
            !face->info.plane.has_value())
        {
            return false;
        }

        const double axisDot =
            std::abs(
                face->info.plane->normal.Dot(
                    candidateAxis.direction));

        return axisDot >= HolePathPlaneAxisDotTolerance;
    }

    std::vector<HoleReachability> HoleCandidateBuilder::filterReachabilitiesForSegmentGroup(
        const std::vector<int>& segmentCandidateIndices,
        const std::vector<HoleReachability>& reachabilities) const
    {
        std::vector<HoleReachability> filteredReachabilities;

        for (const auto& reachability : reachabilities)
        {
            if (!CollectionUtil::contains(
                    segmentCandidateIndices,
                    reachability.lhsSegmentCandidateIndex))
            {
                continue;
            }

            if (!CollectionUtil::contains(
                    segmentCandidateIndices,
                    reachability.rhsSegmentCandidateIndex))
            {
                continue;
            }

            filteredReachabilities.push_back(reachability);
        }

        return filteredReachabilities;
    }

    bool HoleCandidateBuilder::isSameAxisSegment(const HoleSegmentCandidate& lhs, const HoleSegmentCandidate& rhs) const
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

    bool HoleCandidateBuilder::isWallOnCandidateAxis(const CandidateAxis& candidateAxis, const HoleWallCandidate& wallCandidate) const
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
}
