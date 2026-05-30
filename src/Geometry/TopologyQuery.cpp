#include "Geometry/TopologyQuery.h"

#include "Core/CollectionUtil.h"

#include "Geometry/GeometryModel.h"
#include "Geometry/GeometryGraph.h"

namespace OccQtCore::TopologyQuery
{
    bool isValidFaceIndex(
        const GeometryModel& model,
        int faceIndex)
    {
        const auto& graph = model.graph();

        return faceIndex >= 0 &&
               faceIndex < graph.faceCount();
    }

    bool isValidWireIndex(
        const GeometryModel& model,
        int wireIndex)
    {
        const auto& graph = model.graph();

        return wireIndex >= 0 &&
               wireIndex < graph.wireCount();
    }

    bool isValidEdgeIndex(
        const GeometryModel& model,
        int edgeIndex)
    {
        const auto& graph = model.graph();

        return edgeIndex >= 0 &&
               edgeIndex < graph.edgeCount();
    }

    bool isValidVertexIndex(
        const GeometryModel& model,
        int vertexIndex)
    {
        const auto& graph = model.graph();

        return vertexIndex >= 0 &&
               vertexIndex < graph.vertexCount();
    }

    std::vector<int> edgesOfFace(
        const GeometryModel& model,
        int faceIndex)
    {
        std::vector<int> edgeIndices;

        if (!isValidFaceIndex(model, faceIndex))
        {
            return edgeIndices;
        }

        const auto& graph = model.graph();

        for (int wireIndex : graph.wiresOfFace(faceIndex))
        {
            if (!isValidWireIndex(model, wireIndex))
            {
                continue;
            }

            for (int edgeIndex : graph.edgesOfWire(wireIndex))
            {
                if (!isValidEdgeIndex(model, edgeIndex))
                {
                    continue;
                }

                CollectionUtil::addUnique(edgeIndices, edgeIndex);
            }
        }

        return edgeIndices;
    }

    std::vector<int> facesOfEdge(
        const GeometryModel& model,
        int edgeIndex)
    {
        std::vector<int> faceIndices;

        if (!isValidEdgeIndex(model, edgeIndex))
        {
            return faceIndices;
        }

        const auto& graph = model.graph();

        for (int wireIndex : graph.wiresOfEdge(edgeIndex))
        {
            if (!isValidWireIndex(model, wireIndex))
            {
                continue;
            }

            for (int faceIndex : graph.facesOfWire(wireIndex))
            {
                if (!isValidFaceIndex(model, faceIndex))
                {
                    continue;
                }

                CollectionUtil::addUnique(faceIndices, faceIndex);
            }
        }

        return faceIndices;
    }

    std::vector<int> adjacentFacesOfEdge(
        const GeometryModel& model,
        int edgeIndex,
        int excludeFaceIndex)
    {
        std::vector<int> adjacentFaceIndices;

        const auto faceIndices = facesOfEdge(model, edgeIndex);

        for (int faceIndex : faceIndices)
        {
            if (faceIndex == excludeFaceIndex)
            {
                continue;
            }

            CollectionUtil::addUnique(adjacentFaceIndices, faceIndex);
        }

        return adjacentFaceIndices;
    }

    std::vector<int> adjacentFacesOfFace(
        const GeometryModel& model,
        int faceIndex)
    {
        std::vector<int> adjacentFaceIndices;

        if (!isValidFaceIndex(model, faceIndex))
        {
            return adjacentFaceIndices;
        }

        const auto edgeIndices = edgesOfFace(model, faceIndex);

        for (int edgeIndex : edgeIndices)
        {
            const auto edgeAdjacentFaceIndices =
                adjacentFacesOfEdge(
                    model,
                    edgeIndex,
                    faceIndex);

            for (int adjacentFaceIndex : edgeAdjacentFaceIndices)
            {
                CollectionUtil::addUnique(
                    adjacentFaceIndices,
                    adjacentFaceIndex);
            }
        }

        return adjacentFaceIndices;
    }

    std::vector<FaceGroupBoundaryConnection> collectBoundaryConnectionsOfFaceGroup(
        const GeometryModel& model,
        const std::vector<int>& faceIndices)
    {
        std::vector<FaceGroupBoundaryConnection> connections;

        for (int sourceFaceIndex : faceIndices)
        {
            if (!isValidFaceIndex(model, sourceFaceIndex))
            {
                continue;
            }

            const auto edgeIndices = edgesOfFace(model, sourceFaceIndex);

            for (int edgeIndex : edgeIndices)
            {
                const auto adjacentFaceIndices =
                    adjacentFacesOfEdge(
                        model,
                        edgeIndex,
                        sourceFaceIndex);

                for (int adjacentFaceIndex : adjacentFaceIndices)
                {
                    // FaceGroup 内の Face へ戻る接続は、外側境界接続ではない。
                    if (CollectionUtil::contains(faceIndices, adjacentFaceIndex))
                    {
                        continue;
                    }

                    FaceGroupBoundaryConnection connection;
                    connection.sourceFaceIndex = sourceFaceIndex;
                    connection.boundaryEdgeIndex = edgeIndex;
                    connection.adjacentFaceIndex = adjacentFaceIndex;

                    connections.push_back(connection);
                }
            }
        }

        return connections;
    }
}
