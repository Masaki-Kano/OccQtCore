#include <cmath>
#include <tuple>

#include "Feature/HoleEndCandidateDetector.h"
#include "Core/CollectionUtil.h"
#include "Geometry/GeometryModel.h"
#include "Geometry/TopologyQuery.h"

namespace
{
    constexpr double AxialPositionTolerance = 1.0e-4;

    struct HoleEndCandidateKey
    {
        int wallCandidateIndex = -1;
        std::vector<int> faceIndices;

        int axialPositionKey = 0;
        bool hasAxialPositionKey = false;

        OccQtCore::Feature::HoleEndCandidateType type =
            OccQtCore::Feature::HoleEndCandidateType::Unknown;

        bool operator==(const HoleEndCandidateKey& other) const
        {
            return wallCandidateIndex == other.wallCandidateIndex &&
                   faceIndices == other.faceIndices &&
                   axialPositionKey == other.axialPositionKey &&
                   hasAxialPositionKey == other.hasAxialPositionKey &&
                   type == other.type;
        }
    };

    HoleEndCandidateKey makeHoleEndCandidateKey(
        const OccQtCore::Feature::HoleEndCandidate& candidate)
    {
        HoleEndCandidateKey key;

        key.wallCandidateIndex = candidate.wallCandidateIndex;
        key.type = candidate.type;

        if (candidate.type != OccQtCore::Feature::HoleEndCandidateType::Open)
        {
            key.faceIndices = candidate.geometryRefs.faceIndices;
            OccQtCore::CollectionUtil::sortUnique(key.faceIndices);
        }

        if (candidate.hasAxialPosition)
        {
            key.axialPositionKey =
                static_cast<int>(
                    std::round(
                        candidate.axialPosition / AxialPositionTolerance));

            key.hasAxialPositionKey = true;
        }

        return key;
    }

    void mergeEndCandidateGeometryRefs(
        OccQtCore::Feature::HoleEndCandidate& dst,
        const OccQtCore::Feature::HoleEndCandidate& src)
    {
        for (int faceIndex : src.geometryRefs.faceIndices)
        {
            OccQtCore::CollectionUtil::addUnique(
                dst.geometryRefs.faceIndices,
                faceIndex);
        }

        for (int edgeIndex : src.geometryRefs.edgeIndices)
        {
            OccQtCore::CollectionUtil::addUnique(
                dst.geometryRefs.edgeIndices,
                edgeIndex);
        }
    }

    bool isSameEndCandidate(
        const OccQtCore::Feature::HoleEndCandidate& lhs,
        const OccQtCore::Feature::HoleEndCandidate& rhs)
    {
        return makeHoleEndCandidateKey(lhs) ==
               makeHoleEndCandidateKey(rhs);
    }

    void appendOrMergeEndCandidate(
        std::vector<OccQtCore::Feature::HoleEndCandidate>& candidates,
        const OccQtCore::Feature::HoleEndCandidate& candidate)
    {
        for (auto& existingCandidate : candidates)
        {
            if (isSameEndCandidate(existingCandidate, candidate))
            {
                mergeEndCandidateGeometryRefs(
                    existingCandidate,
                    candidate);

                return;
            }
        }

        candidates.push_back(candidate);
    }
}

namespace OccQtCore::Feature
{
    HoleEndCandidateDetector::HoleEndCandidateDetector(
        const GeometryModel& model,
        const std::vector<HoleWallCandidate>& wallCandidates)
        : m_model(model)
        , m_wallCandidates(wallCandidates)
    {
    }

    std::vector<HoleEndCandidate> HoleEndCandidateDetector::detect() const
    {
        std::vector<HoleEndCandidate> candidates;

        for (const auto& wallCandidate : m_wallCandidates)
        {
            const auto connections =
                TopologyQuery::collectBoundaryConnectionsOfFaceGroup(
                    m_model,
                    wallCandidate.geometryRefs.faceIndices);

            for (const auto& connection : connections)
            {
                const auto buildCandidates =
                    buildFromWallConnection(
                        wallCandidate,
                        connection.adjacentFaceIndex,
                        connection.boundaryEdgeIndex);

                for (const auto& candidate : buildCandidates)
                {
                    appendOrMergeEndCandidate(candidates, candidate);
                }
            }
        }

        for (int i = 0; i < static_cast<int>(candidates.size()); ++i)
        {
            candidates[i].index = i;
        }

        return candidates;
    }

    std::vector<HoleEndCandidate> HoleEndCandidateDetector::buildFromWallConnection(
        const HoleWallCandidate& sourceWallCandidate,
        int adjacentFaceIndex,
        int connectionEdgeIndex) const
    {
        const auto* faceData = m_model.faceAt(adjacentFaceIndex);

        if (faceData == nullptr)
        {
            return {};
        }

        if (isTransitionSurface(faceData->info.kind))
        {
            return buildThroughTransitionSurface(
                sourceWallCandidate,
                adjacentFaceIndex,
                connectionEdgeIndex);
        }

        return buildDirectConnection(
            sourceWallCandidate,
            adjacentFaceIndex,
            connectionEdgeIndex);
    }

    std::vector<HoleEndCandidate> HoleEndCandidateDetector::buildDirectConnection(
        const HoleWallCandidate& sourceWallCandidate,
        int adjacentFaceIndex,
        int connectionEdgeIndex) const
    {
        const auto* faceData = m_model.faceAt(adjacentFaceIndex);

        if (faceData == nullptr)
        {
            return {};
        }

        if (isFaceOwnedByOtherWallCandidate(
                adjacentFaceIndex,
                sourceWallCandidate.index))
        {
            return {
                makeWallConnectionEndCandidateFromWall(
                    sourceWallCandidate,
                    connectionEdgeIndex,
                    adjacentFaceIndex)
            };
        }

        auto candidate =
            makeBaseEndCandidateFromWall(sourceWallCandidate);

        if (isConnectionEdgeOnInnerWireOfFace(
                adjacentFaceIndex,
                connectionEdgeIndex))
        {
            candidate.type =
                HoleEndCandidateType::Open;

            CollectionUtil::addUnique(
                candidate.geometryRefs.edgeIndices,
                connectionEdgeIndex);

            setAxialPositionFromAnyEdge(
                sourceWallCandidate,
                candidate);

            return { candidate };
        }

        candidate.type =
            HoleEndCandidateType::Bottom;

        CollectionUtil::addUnique(
            candidate.geometryRefs.faceIndices,
            adjacentFaceIndex);

        CollectionUtil::addUnique(
            candidate.geometryRefs.edgeIndices,
            connectionEdgeIndex);

        setAxialPositionFromAnyEdge(
            sourceWallCandidate,
            candidate);

        return { candidate };
    }

    bool HoleEndCandidateDetector::isFaceOwnedByOtherWallCandidate(
        int faceIndex,
        int sourceWallCandidateIndex) const
    {
        for (const auto& wallCandidate : m_wallCandidates)
        {
            if (wallCandidate.index == sourceWallCandidateIndex)
            {
                continue;
            }

            if (OccQtCore::CollectionUtil::contains(
                    wallCandidate.geometryRefs.faceIndices,
                    faceIndex))
            {
                return true;
            }
        }

        return false;
    }

    bool HoleEndCandidateDetector::isConnectionEdgeOnInnerWireOfFace(
        int faceIndex,
        int edgeIndex) const
    {
        if (!OccQtCore::TopologyQuery::isValidFaceIndex(m_model, faceIndex) ||
            !OccQtCore::TopologyQuery::isValidEdgeIndex(m_model, edgeIndex))
        {
            return false;
        }

        const auto& graph = m_model.graph();
        const auto& wires = m_model.wires();

        for (int wireIndex : graph.wiresOfFace(faceIndex))
        {
            if (!OccQtCore::TopologyQuery::isValidWireIndex(m_model, wireIndex))
            {
                continue;
            }

            const auto edgeIndices = graph.edgesOfWire(wireIndex);

            if (!OccQtCore::CollectionUtil::contains(edgeIndices, edgeIndex))
            {
                continue;
            }

            const auto& wire = wires[wireIndex];

            return wire.info.isInner && wire.info.isClosed;
        }

        return false;
    }

    bool HoleEndCandidateDetector::isTransitionSurface(
        SurfaceKind kind) const
    {
        return kind == SurfaceKind::Cone ||
               kind == SurfaceKind::Torus;
    }

    HoleEndCandidate HoleEndCandidateDetector::makeBaseEndCandidateFromWall(
        const HoleWallCandidate& wallCandidate) const
    {
        HoleEndCandidate candidate;

        candidate.wallCandidateIndex = wallCandidate.index;
        candidate.center = wallCandidate.center;
        candidate.axisDirection = wallCandidate.axisDirection;
        candidate.normalDirection = wallCandidate.axisDirection;
        candidate.radius = wallCandidate.radius;

        return candidate;
    }

    HoleEndCandidate HoleEndCandidateDetector::makeWallConnectionEndCandidateFromWall(
        const HoleWallCandidate& wallCandidate,
        int connectionEdgeIndex,
        int connectedFaceIndex) const
    {
        auto candidate = makeBaseEndCandidateFromWall(wallCandidate);

        candidate.type = HoleEndCandidateType::WallConnection;

        OccQtCore::CollectionUtil::addUnique(
            candidate.geometryRefs.edgeIndices,
            connectionEdgeIndex);

        OccQtCore::CollectionUtil::addUnique(
            candidate.geometryRefs.faceIndices,
            connectedFaceIndex);

        return candidate;
    }

    void HoleEndCandidateDetector::setAxialPositionFromEdge(
        const HoleWallCandidate& wallCandidate,
        int edgeIndex,
        HoleEndCandidate& candidate) const
    {
        const auto* edgeData = m_model.edgeAt(edgeIndex);

        if (edgeData == nullptr)
        {
            return;
        }

        if (edgeData->info.kind != CurveKind::Circle ||
            !edgeData->info.circle.has_value())
        {
            return;
        }

        const auto& circle = edgeData->info.circle.value();

        candidate.center = circle.center;

        const auto vectorFromWallCenter =
            circle.center.XYZ() - wallCandidate.center.XYZ();

        candidate.axialPosition =
            vectorFromWallCenter.Dot(wallCandidate.axisDirection.XYZ());

        candidate.hasAxialPosition = true;
    }

    void HoleEndCandidateDetector::setAxialPositionFromAnyEdge(
        const HoleWallCandidate& wallCandidate,
        HoleEndCandidate& candidate) const
    {
        if (candidate.hasAxialPosition)
        {
            return;
        }

        for (int edgeIndex : candidate.geometryRefs.edgeIndices)
        {
            setAxialPositionFromEdge(
                wallCandidate,
                edgeIndex,
                candidate);

            if (candidate.hasAxialPosition)
            {
                return;
            }
        }
    }

    std::vector<HoleEndCandidate> HoleEndCandidateDetector::buildThroughTransitionSurface(
        const HoleWallCandidate& sourceWallCandidate,
        int transitionFaceIndex,
        int wallConnectionEdgeIndex) const
    {
        const auto* transitionFaceData =
            m_model.faceAt(transitionFaceIndex);

        if (transitionFaceData == nullptr)
        {
            return {};
        }

        if (!isTransitionSurface(transitionFaceData->info.kind))
        {
            return {};
        }

        std::vector<HoleEndCandidate> candidates;

        const auto transitionEdgeIndices =
            TopologyQuery::edgesOfFace(
                m_model,
                transitionFaceIndex);

        bool foundNextConnection = false;

        for (int nextEdgeIndex : transitionEdgeIndices)
        {
            if (nextEdgeIndex == wallConnectionEdgeIndex)
            {
                continue;
            }

            const auto nextFaceIndices =
                TopologyQuery::adjacentFacesOfEdge(
                    m_model,
                    nextEdgeIndex,
                    transitionFaceIndex);

            for (int nextFaceIndex : nextFaceIndices)
            {
                const auto* nextFaceData =
                    m_model.faceAt(nextFaceIndex);

                if (nextFaceData == nullptr)
                {
                    continue;
                }

                foundNextConnection = true;

                auto candidate =
                    makeBaseEndCandidateFromWall(sourceWallCandidate);

                CollectionUtil::addUnique(
                    candidate.geometryRefs.faceIndices,
                    transitionFaceIndex);

                CollectionUtil::addUnique(
                    candidate.geometryRefs.edgeIndices,
                    wallConnectionEdgeIndex);

                CollectionUtil::addUnique(
                    candidate.geometryRefs.edgeIndices,
                    nextEdgeIndex);

                if (isFaceOwnedByOtherWallCandidate(
                        nextFaceIndex,
                        sourceWallCandidate.index))
                {
                    candidate.type =
                        HoleEndCandidateType::WallConnection;

                    CollectionUtil::addUnique(
                        candidate.geometryRefs.faceIndices,
                        nextFaceIndex);

                    setAxialPositionFromAnyEdge(
                        sourceWallCandidate,
                        candidate);

                    candidates.push_back(candidate);
                    continue;
                }

                if (isConnectionEdgeOnInnerWireOfFace(
                        nextFaceIndex,
                        nextEdgeIndex))
                {
                    candidate.type =
                        HoleEndCandidateType::Open;

                    setAxialPositionFromAnyEdge(
                        sourceWallCandidate,
                        candidate);

                    candidates.push_back(candidate);
                    continue;
                }

                candidate.type =
                    HoleEndCandidateType::Bottom;

                CollectionUtil::addUnique(
                    candidate.geometryRefs.faceIndices,
                    nextFaceIndex);

                setAxialPositionFromAnyEdge(
                    sourceWallCandidate,
                    candidate);

                candidates.push_back(candidate);
            }
        }

        if (candidates.empty() &&
            !foundNextConnection &&
            transitionFaceData->info.kind == SurfaceKind::Cone)
        {
            auto candidate =
                makeBaseEndCandidateFromWall(sourceWallCandidate);

            candidate.type =
                HoleEndCandidateType::Bottom;

            CollectionUtil::addUnique(
                candidate.geometryRefs.faceIndices,
                transitionFaceIndex);

            CollectionUtil::addUnique(
                candidate.geometryRefs.edgeIndices,
                wallConnectionEdgeIndex);

            setAxialPositionFromAnyEdge(
                sourceWallCandidate,
                candidate);

            candidates.push_back(candidate);
        }

        return candidates;
    }
}
