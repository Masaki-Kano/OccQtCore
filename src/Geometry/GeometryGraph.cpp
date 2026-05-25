#include "Geometry/GeometryGraph.h"

#include <algorithm>

namespace OccQtCore
{
    void GeometryGraph::clear()
    {
        m_faceToEdges.clear();
        m_edgeToFaces.clear();
        m_edgeToVertices.clear();
        m_vertexToEdges.clear();
    }

    void GeometryGraph::resize(int faceCount, int edgeCount, int vertexCount)
    {
        m_faceToEdges.assign(faceCount, {});
        m_edgeToFaces.assign(edgeCount, {});
        m_edgeToVertices.assign(edgeCount, {});
        m_vertexToEdges.assign(vertexCount, {});
    }

    void GeometryGraph::addFaceEdgeRelation(int faceIndex, int edgeIndex)
    {
        addRelation(m_faceToEdges, faceIndex, edgeIndex);
    }

    void GeometryGraph::addEdgeFaceRelation(int edgeIndex, int faceIndex)
    {
        addRelation(m_edgeToFaces, edgeIndex, faceIndex);
    }

    void GeometryGraph::addEdgeVertexRelation(int edgeIndex, int vertexIndex)
    {
        addRelation(m_edgeToVertices, edgeIndex, vertexIndex);
    }

    void GeometryGraph::addVertexEdgeRelation(int vertexIndex, int edgeIndex)
    {
        addRelation(m_vertexToEdges, vertexIndex, edgeIndex);
    }

    const std::vector<int>& GeometryGraph::edgesOfFace(int faceIndex) const
    {
        return listOrEmpty(m_faceToEdges, faceIndex);
    }

    const std::vector<int>& GeometryGraph::facesOfEdge(int edgeIndex) const
    {
        return listOrEmpty(m_edgeToFaces, edgeIndex);
    }

    const std::vector<int>& GeometryGraph::verticesOfEdge(int edgeIndex) const
    {
        return listOrEmpty(m_edgeToVertices, edgeIndex);
    }

    const std::vector<int>& GeometryGraph::edgesOfVertex(int vertexIndex) const
    {
        return listOrEmpty(m_vertexToEdges, vertexIndex);
    }

    int GeometryGraph::faceCount() const
    {
        return static_cast<int>(m_faceToEdges.size());
    }

    int GeometryGraph::edgeCount() const
    {
        return static_cast<int>(m_edgeToFaces.size());
    }

    int GeometryGraph::vertexCount() const
    {
        return static_cast<int>(m_vertexToEdges.size());
    }

    bool GeometryGraph::isValidIndex(int index, int count)
    {
        return index >= 0 && index < count;
    }

    const std::vector<int>& GeometryGraph::listOrEmpty(
        const std::vector<std::vector<int>>& lists,
        int index)
    {
        if (!isValidIndex(index, static_cast<int>(lists.size())))
        {
            return emptyList();
        }

        return lists[static_cast<std::size_t>(index)];
    }

    void GeometryGraph::addRelation(
        std::vector<std::vector<int>>& relations,
        int fromIndex,
        int toIndex)
    {
        if (!isValidIndex(fromIndex, static_cast<int>(relations.size())))
        {
            return;
        }

        addUnique(relations[static_cast<std::size_t>(fromIndex)], toIndex);
    }

    void GeometryGraph::addUnique(std::vector<int>& values, int value)
    {
        if (value < 0)
        {
            return;
        }

        if (std::find(values.begin(), values.end(), value) != values.end())
        {
            return;
        }

        values.push_back(value);
    }

    const std::vector<int>& GeometryGraph::emptyList()
    {
        static const std::vector<int> empty;
        return empty;
    }
}
