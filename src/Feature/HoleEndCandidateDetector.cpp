#include "Feature/HoleEndCandidateDetector.h"
#include "Core/CollectionUtil.h"
#include "Geometry/GeometryModel.h"
#include "Geometry/TopologyQuery.h"

namespace
{
    struct WallBoundaryLoop
    {
        int index = -1;
        std::vector<int>edgeIndices;
    };

    bool hasSharedIndex(const std::vector<int>& lhs, const std::vector<int>& rhs)
    {
        for (int index : lhs)
        {
            if (OccQtCore::CollectionUtil::contains(rhs, index))
            {
                return true;
            }
        }

        return false;
    }

    bool areEdgesConnectedByVertex(const OccQtCore::GeometryModel& model, int lhsEdgeIndex, int rhsEdgeIndex)
    {
        if (!OccQtCore::TopologyQuery::isValidEdgeIndex(model, lhsEdgeIndex) ||
            !OccQtCore::TopologyQuery::isValidEdgeIndex(model, rhsEdgeIndex))
        {
            return false;
        }

        const auto& graph = model.graph();

        return hasSharedIndex(graph.verticesOfEdge(lhsEdgeIndex), graph.verticesOfEdge(rhsEdgeIndex));
    }

    std::vector<int> collectBoundaryEdgeIndices(const std::vector<OccQtCore::TopologyQuery::FaceGroupBoundaryConnection>& connections)
    {
        std::vector<int> edgeIndices;

        for (const auto& connection : connections)
        {
            OccQtCore::CollectionUtil::addUnique(edgeIndices, connection.boundaryEdgeIndex);
        }

        return edgeIndices;
    }

    std::vector<WallBoundaryLoop> buildWallBoundaryLoops(const OccQtCore::GeometryModel& model, const std::vector<OccQtCore::TopologyQuery::FaceGroupBoundaryConnection>& connections)
    {
        const auto boundaryEdgeIndices =
            collectBoundaryEdgeIndices(connections);

        std::vector<WallBoundaryLoop> loops;
        std::vector<int> assignedEdgeIndices;

        for (int seedEdgeIndex : boundaryEdgeIndices)
        {
            if (OccQtCore::CollectionUtil::contains(
                    assignedEdgeIndices,
                    seedEdgeIndex))
            {
                continue;
            }

            WallBoundaryLoop loop;
            loop.index =
                static_cast<int>(loops.size());

            std::vector<int> stack;
            stack.push_back(seedEdgeIndex);

            OccQtCore::CollectionUtil::addUnique(
                assignedEdgeIndices,
                seedEdgeIndex);

            while (!stack.empty())
            {
                const int currentEdgeIndex =
                    stack.back();

                stack.pop_back();

                OccQtCore::CollectionUtil::addUnique(
                    loop.edgeIndices,
                    currentEdgeIndex);

                for (int otherEdgeIndex : boundaryEdgeIndices)
                {
                    if (OccQtCore::CollectionUtil::contains(
                            assignedEdgeIndices,
                            otherEdgeIndex))
                    {
                        continue;
                    }

                    if (!areEdgesConnectedByVertex(
                            model,
                            currentEdgeIndex,
                            otherEdgeIndex))
                    {
                        continue;
                    }

                    OccQtCore::CollectionUtil::addUnique(
                        assignedEdgeIndices,
                        otherEdgeIndex);

                    stack.push_back(otherEdgeIndex);
                }
            }

            loops.push_back(loop);
        }

        return loops;
    }

    int endTypePriority(OccQtCore::Feature::HoleEndCandidateType type)
    {
        using Type = OccQtCore::Feature::HoleEndCandidateType;

        switch (type)
        {
        case Type::Open:
            return 3;

        case Type::WallConnection:
            return 2;

        case Type::Bottom:
            return 1;

        case Type::Unknown:
        default:
            return 0;
        }
    }

    void mergeEndCandidateGeometryRefs(OccQtCore::Feature::HoleEndCandidate& dst, const OccQtCore::Feature::HoleEndCandidate& src)
    {
        for (int faceIndex : src.geometryRefs.faceIndices)
        {
            OccQtCore::CollectionUtil::addUnique(dst.geometryRefs.faceIndices, faceIndex);
        }

        for (int wireIndex : src.geometryRefs.wireIndices)
        {
            OccQtCore::CollectionUtil::addUnique(dst.geometryRefs.wireIndices, wireIndex);
        }

        for (int edgeIndex : src.geometryRefs.edgeIndices)
        {
            OccQtCore::CollectionUtil::addUnique(dst.geometryRefs.edgeIndices, edgeIndex);
        }

        for (int vertexIndex : src.geometryRefs.vertexIndices)
        {
            OccQtCore::CollectionUtil::addUnique(dst.geometryRefs.vertexIndices, vertexIndex);
        }

        if (endTypePriority(src.type) > endTypePriority(dst.type))
        {
            dst.type = src.type;
        }
    }

    bool isMergeableEndCandidate(const OccQtCore::Feature::HoleEndCandidate& lhs, const OccQtCore::Feature::HoleEndCandidate& rhs)
    {
        if (lhs.wallCandidateIndex != rhs.wallCandidateIndex)
        {
            return false;
        }

        if (lhs.wallBoundaryLoopIndex < 0 || rhs.wallBoundaryLoopIndex < 0)
        {
            return false;
        }

        return lhs.wallBoundaryLoopIndex == rhs.wallBoundaryLoopIndex;
    }


    void appendOrMergeEndCandidate(
        std::vector<OccQtCore::Feature::HoleEndCandidate>& candidates,
        const OccQtCore::Feature::HoleEndCandidate& candidate)
    {
        for (auto& existingCandidate : candidates)
        {
            if (!isMergeableEndCandidate(existingCandidate, candidate))
            {
                continue;
            }

            mergeEndCandidateGeometryRefs(existingCandidate, candidate);

            return;
        }

        candidates.push_back(candidate);
    }

    int findWallBoundaryLoopIndex(const std::vector<WallBoundaryLoop>& loops, int boundaryEdgeIndex)
    {
        for (const auto& loop : loops)
        {
            if (OccQtCore::CollectionUtil::contains(loop.edgeIndices, boundaryEdgeIndex))
            {
                return loop.index;
            }
        }

        return -1;
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

            const auto boundaryLoops =
                buildWallBoundaryLoops(
                    m_model,
                    connections);

            for (const auto& connection : connections)
            {
                const int wallBoundaryLoopIndex =
                    findWallBoundaryLoopIndex(
                        boundaryLoops,
                        connection.boundaryEdgeIndex);

                const auto buildCandidates =
                    buildFromWallConnection(
                        wallCandidate,
                        connection.adjacentFaceIndex,
                        connection.boundaryEdgeIndex,
                        wallBoundaryLoopIndex);

                for (const auto& candidate : buildCandidates)
                {
                    appendOrMergeEndCandidate(
                        candidates,
                        candidate);
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
        int connectionEdgeIndex,
        int wallBoundaryLoopIndex) const
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
                connectionEdgeIndex,
                wallBoundaryLoopIndex);
        }

        return buildDirectConnection(
            sourceWallCandidate,
            adjacentFaceIndex,
            connectionEdgeIndex,
            wallBoundaryLoopIndex);
    }

    std::vector<HoleEndCandidate> HoleEndCandidateDetector::buildDirectConnection(
        const HoleWallCandidate& sourceWallCandidate,
        int adjacentFaceIndex,
        int connectionEdgeIndex,
        int wallBoundaryLoopIndex) const
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
                    adjacentFaceIndex,
                    wallBoundaryLoopIndex)
            };
        }

        auto candidate =
            makeBaseEndCandidateFromWall(
                sourceWallCandidate,
                wallBoundaryLoopIndex);

        if (isConnectionEdgeOnInnerWireOfFace(
                adjacentFaceIndex,
                connectionEdgeIndex))
        {
            candidate.type =
                HoleEndCandidateType::Open;

            CollectionUtil::addUnique(
                candidate.geometryRefs.edgeIndices,
                connectionEdgeIndex);

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
        const HoleWallCandidate& wallCandidate,
        int wallBoundaryLoopIndex) const
    {
        HoleEndCandidate candidate;

        candidate.wallCandidateIndex =
            wallCandidate.index;

        candidate.wallBoundaryLoopIndex =
            wallBoundaryLoopIndex;

        return candidate;
    }

    HoleEndCandidate HoleEndCandidateDetector::makeWallConnectionEndCandidateFromWall(
        const HoleWallCandidate& wallCandidate,
        int connectionEdgeIndex,
        int connectedFaceIndex,
        int wallBoundaryLoopIndex) const
    {
        auto candidate =
            makeBaseEndCandidateFromWall(
                wallCandidate,
                wallBoundaryLoopIndex);

        candidate.type =
            HoleEndCandidateType::WallConnection;

        OccQtCore::CollectionUtil::addUnique(
            candidate.geometryRefs.edgeIndices,
            connectionEdgeIndex);

        OccQtCore::CollectionUtil::addUnique(
            candidate.geometryRefs.faceIndices,
            connectedFaceIndex);

        return candidate;
    }

    std::vector<HoleEndCandidate> HoleEndCandidateDetector::buildThroughTransitionSurface(
        const HoleWallCandidate& sourceWallCandidate,
        int transitionFaceIndex,
        int wallConnectionEdgeIndex,
        int wallBoundaryLoopIndex) const
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
                    makeBaseEndCandidateFromWall(
                        sourceWallCandidate,
                        wallBoundaryLoopIndex);

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

                    candidates.push_back(candidate);
                    continue;
                }

                if (isConnectionEdgeOnInnerWireOfFace(
                        nextFaceIndex,
                        nextEdgeIndex))
                {
                    candidate.type =
                        HoleEndCandidateType::Open;

                    candidates.push_back(candidate);
                    continue;
                }

                candidate.type =
                    HoleEndCandidateType::Bottom;

                CollectionUtil::addUnique(
                    candidate.geometryRefs.faceIndices,
                    nextFaceIndex);

                candidates.push_back(candidate);
            }
        }

        if (candidates.empty() &&
            !foundNextConnection &&
            transitionFaceData->info.kind == SurfaceKind::Cone)
        {
            auto candidate =
                makeBaseEndCandidateFromWall(
                    sourceWallCandidate,
                    wallBoundaryLoopIndex);

            candidate.type =
                HoleEndCandidateType::Bottom;

            CollectionUtil::addUnique(
                candidate.geometryRefs.faceIndices,
                transitionFaceIndex);

            CollectionUtil::addUnique(
                candidate.geometryRefs.edgeIndices,
                wallConnectionEdgeIndex);

            candidates.push_back(candidate);
        }

        return candidates;
    }
}
