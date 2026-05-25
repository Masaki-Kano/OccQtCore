#ifndef GEOMETRYGRAPH_H
#define GEOMETRYGRAPH_H

#include <vector>

namespace OccQtCore
{
    class GeometryGraph
    {
    public:
        void clear();
        void resize(int faceCount, int edgeCount, int vertexCount);

        void addFaceEdgeRelation(int faceIndex, int edgeIndex);
        void addEdgeFaceRelation(int edgeIndex, int faceIndex);

        void addEdgeVertexRelation(int edgeIndex, int vertexIndex);
        void addVertexEdgeRelation(int vertexIndex, int edgeIndex);

        const std::vector<int>& edgesOfFace(int faceIndex) const;
        const std::vector<int>& facesOfEdge(int edgeIndex) const;

        const std::vector<int>& verticesOfEdge(int edgeIndex) const;
        const std::vector<int>& edgesOfVertex(int vertexIndex) const;

        int faceCount() const;
        int edgeCount() const;
        int vertexCount() const;

    private:
        std::vector<std::vector<int>> m_faceToEdges;
        std::vector<std::vector<int>> m_edgeToFaces;

        std::vector<std::vector<int>> m_edgeToVertices;
        std::vector<std::vector<int>> m_vertexToEdges;

        static bool isValidIndex(int index, int count);

        static const std::vector<int>& listOrEmpty(
            const std::vector<std::vector<int>>& lists,
            int index);

        static void addRelation(
            std::vector<std::vector<int>>& relations,
            int fromIndex,
            int toIndex);

        static void addUnique(std::vector<int>& values, int value);

        static const std::vector<int>& emptyList();
    };
}

#endif // GEOMETRYGRAPH_H
