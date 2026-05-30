#ifndef GEOMETRYGRAPH_H
#define GEOMETRYGRAPH_H

#include <vector>

namespace OccQtCore
{
    class GeometryGraph
    {
    public:
        void clear();
        void resize(int faceCount, int wireCount, int edgeCount, int vertexCount);

        void addFaceWireRelation(int faceIndex, int wireIndex);
        void addWireFaceRelation(int wireIndex, int faceIndex);

        void addWireEdgeRelation(int wireIndex, int edgeIndex);
        void addEdgeWireRelation(int edgeIndex, int wireIndex);

        void addEdgeVertexRelation(int edgeIndex, int vertexIndex);
        void addVertexEdgeRelation(int vertexIndex, int edgeIndex);

        const std::vector<int>& wiresOfFace(int faceIndex) const;
        const std::vector<int>& facesOfWire(int wireIndex) const;

        const std::vector<int>& edgesOfWire(int wireIndex) const;
        const std::vector<int>& wiresOfEdge(int edgeIndex) const;

        const std::vector<int>& verticesOfEdge(int edgeIndex) const;
        const std::vector<int>& edgesOfVertex(int vertexIndex) const;

        int faceCount() const;
        int wireCount() const;
        int edgeCount() const;
        int vertexCount() const;

    private:
        std::vector<std::vector<int>> m_faceToWires;
        std::vector<std::vector<int>> m_wireToFaces;

        std::vector<std::vector<int>> m_wireToEdges;
        std::vector<std::vector<int>> m_edgeToWires;

        std::vector<std::vector<int>> m_edgeToVertices;
        std::vector<std::vector<int>> m_vertexToEdges;
    };
}

#endif // GEOMETRYGRAPH_H
