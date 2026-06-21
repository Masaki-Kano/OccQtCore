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

        for (const auto& wireRef : graph.wireRefsOfFace(faceIndex))
        {
            const int wireIndex = wireRef.wireIndex;

            if (!isValidWireIndex(model, wireIndex))
            {
                continue;
            }

            for (const auto& edgeRef : graph.edgeRefsOfWire(wireIndex))
            {
                const int edgeIndex = edgeRef.edgeIndex;

                if (!isValidEdgeIndex(model, edgeIndex))
                {
                    continue;
                }

                CollectionUtil::addUnique(
                    edgeIndices,
                    edgeIndex);
            }
        }

        CollectionUtil::sortUnique(edgeIndices);

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

        CollectionUtil::sortUnique(faceIndices);

        return faceIndices;
    }

    std::vector<int> verticesOfFace(
        const GeometryModel& model,
        int faceIndex)
    {
        std::vector<int> vertexIndices;

        if (!isValidFaceIndex(model, faceIndex))
        {
            return vertexIndices;
        }

        const auto& graph = model.graph();

        for (const int edgeIndex :
             edgesOfFace(model, faceIndex))
        {
            for (const int vertexIndex :
                 graph.verticesOfEdge(edgeIndex))
            {
                if (!isValidVertexIndex(
                        model,
                        vertexIndex))
                {
                    continue;
                }

                CollectionUtil::addUnique(
                    vertexIndices,
                    vertexIndex);
            }
        }

        CollectionUtil::sortUnique(vertexIndices);

        return vertexIndices;
    }

    int edgeUseCountInFace(
        const GeometryModel& model,
        int faceIndex,
        int edgeIndex)
    {
        if (!isValidFaceIndex(model, faceIndex) ||
            !isValidEdgeIndex(model, edgeIndex))
        {
            return 0;
        }

        int useCount = 0;

        const auto& graph = model.graph();

        for (const auto& wireRef : graph.wireRefsOfFace(faceIndex))
        {
            const int wireIndex = wireRef.wireIndex;

            if (!isValidWireIndex(model, wireIndex))
            {
                continue;
            }

            for (const auto& edgeRef : graph.edgeRefsOfWire(wireIndex))
            {
                if (edgeRef.edgeIndex == edgeIndex)
                {
                    ++useCount;
                }
            }
        }

        return useCount;
    }

    std::vector<int> sharedEdgesOfFaces(
        const GeometryModel& model,
        int lhsFaceIndex,
        int rhsFaceIndex)
    {
        std::vector<int> sharedEdgeIndices;

        if (!isValidFaceIndex(model, lhsFaceIndex) ||
            !isValidFaceIndex(model, rhsFaceIndex))
        {
            return sharedEdgeIndices;
        }

        const auto lhsEdgeIndices = edgesOfFace(model, lhsFaceIndex);

        const auto rhsEdgeIndices = edgesOfFace(model, rhsFaceIndex);

        for (const int edgeIndex : lhsEdgeIndices)
        {
            if (!CollectionUtil::contains(rhsEdgeIndices, edgeIndex))
            {
                continue;
            }

            sharedEdgeIndices.push_back(edgeIndex);
        }

        CollectionUtil::sortUnique(sharedEdgeIndices);

        return sharedEdgeIndices;
    }

    bool hasSharedEdge(
        const GeometryModel& model,
        int lhsFaceIndex,
        int rhsFaceIndex)
    {
        return !sharedEdgesOfFaces(
                    model,
                    lhsFaceIndex,
                    rhsFaceIndex)
                    .empty();
    }

    std::vector<int> adjacentFacesOfEdge(
        const GeometryModel& model,
        int edgeIndex,
        int excludeFaceIndex)
    {
        std::vector<int> adjacentFaceIndices;

        for (const int faceIndex :
             facesOfEdge(model, edgeIndex))
        {
            if (faceIndex == excludeFaceIndex)
            {
                continue;
            }

            CollectionUtil::addUnique(
                adjacentFaceIndices,
                faceIndex);
        }

        CollectionUtil::sortUnique(
            adjacentFaceIndices);

        return adjacentFaceIndices;
    }

    std::vector<int> adjacentFacesOfEdgeExcludingFaces(
        const GeometryModel& model,
        int edgeIndex,
        const std::vector<int>& excludeFaceIndices)
    {
        std::vector<int> adjacentFaceIndices;

        for (const int faceIndex :
             facesOfEdge(model, edgeIndex))
        {
            if (CollectionUtil::contains(
                    excludeFaceIndices,
                    faceIndex))
            {
                continue;
            }

            CollectionUtil::addUnique(
                adjacentFaceIndices,
                faceIndex);
        }

        CollectionUtil::sortUnique(
            adjacentFaceIndices);

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

        for (const int edgeIndex :
             edgesOfFace(model, faceIndex))
        {
            const auto edgeAdjacentFaceIndices =
                adjacentFacesOfEdge(
                    model,
                    edgeIndex,
                    faceIndex);

            for (const int adjacentFaceIndex :
                 edgeAdjacentFaceIndices)
            {
                CollectionUtil::addUnique(
                    adjacentFaceIndices,
                    adjacentFaceIndex);
            }
        }

        CollectionUtil::sortUnique(
            adjacentFaceIndices);

        return adjacentFaceIndices;
    }

    std::vector<int> edgesOfFaces(
        const GeometryModel& model,
        const std::vector<int>& faceIndices)
    {
        std::vector<int> edgeIndices;

        for (const int faceIndex : faceIndices)
        {
            if (!isValidFaceIndex(model, faceIndex))
            {
                continue;
            }

            for (const int edgeIndex :
                 edgesOfFace(model, faceIndex))
            {
                CollectionUtil::addUnique(
                    edgeIndices,
                    edgeIndex);
            }
        }

        CollectionUtil::sortUnique(edgeIndices);

        return edgeIndices;
    }

    std::vector<int> boundaryEdgesOfFaces(
        const GeometryModel& model,
        const std::vector<int>& faceIndices)
    {
        std::vector<int> boundaryEdgeIndices;

        for (const int edgeIndex :
             edgesOfFaces(model, faceIndices))
        {
            int totalUseCountInGroup = 0;

            for (const int faceIndex : faceIndices)
            {
                totalUseCountInGroup +=
                    edgeUseCountInFace(
                        model,
                        faceIndex,
                        edgeIndex);
            }

            /*
             * EdgeがFace集合内で1回だけ使われている場合、
             * そのEdgeは集合の境界に露出している。
             *
             * シームEdgeは同一Face内で2回使用されるため、
             * 境界Edgeとは判定されない。
             */
            if (totalUseCountInGroup == 1)
            {
                boundaryEdgeIndices.push_back(
                    edgeIndex);
            }
        }

        CollectionUtil::sortUnique(
            boundaryEdgeIndices);

        return boundaryEdgeIndices;
    }

    std::vector<int> adjacentFacesOfFaces(
        const GeometryModel& model,
        const std::vector<int>& faceIndices)
    {
        std::vector<int> adjacentFaceIndices;

        for (const int edgeIndex :
             boundaryEdgesOfFaces(
                 model,
                 faceIndices))
        {
            const auto outsideFaceIndices =
                adjacentFacesOfEdgeExcludingFaces(
                    model,
                    edgeIndex,
                    faceIndices);

            for (const int adjacentFaceIndex :
                 outsideFaceIndices)
            {
                CollectionUtil::addUnique(
                    adjacentFaceIndices,
                    adjacentFaceIndex);
            }
        }

        CollectionUtil::sortUnique(
            adjacentFaceIndices);

        return adjacentFaceIndices;
    }

    std::vector<FaceBoundaryConnection>
    boundaryConnectionsOfFaces(
        const GeometryModel& model,
        const std::vector<int>& faceIndices)
    {
        std::vector<FaceBoundaryConnection> connections;

        for (const int sourceFaceIndex :
             faceIndices)
        {
            if (!isValidFaceIndex(
                    model,
                    sourceFaceIndex))
            {
                continue;
            }

            for (const int edgeIndex :
                 edgesOfFace(
                     model,
                     sourceFaceIndex))
            {
                /*
                 * 同じFace内で2回以上使われるシームEdgeは、
                 * 外側境界接続として扱わない。
                 */
                if (edgeUseCountInFace(
                        model,
                        sourceFaceIndex,
                        edgeIndex) > 1)
                {
                    continue;
                }

                const auto outsideFaceIndices =
                    adjacentFacesOfEdgeExcludingFaces(
                        model,
                        edgeIndex,
                        faceIndices);

                for (const int adjacentFaceIndex :
                     outsideFaceIndices)
                {
                    FaceBoundaryConnection connection;
                    connection.sourceFaceIndex =
                        sourceFaceIndex;
                    connection.boundaryEdgeIndex =
                        edgeIndex;
                    connection.adjacentFaceIndex =
                        adjacentFaceIndex;

                    connections.push_back(connection);
                }
            }
        }

        return connections;
    }
}
