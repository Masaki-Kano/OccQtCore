#include "Geometry/GeometryGraph.h"

#include <algorithm>

namespace OccQtCore
{
    void GeometryGraph::clear()
    {
        m_faceToWires.clear();
        m_wireToFaces.clear();

        m_wireToEdges.clear();
        m_edgeToWires.clear();

        m_edgeToVertices.clear();
        m_vertexToEdges.clear();
    }

    void GeometryGraph::resize(int faceCount, int wireCount, int edgeCount, int vertexCount)
    {
        clear();

        m_faceToWires.assign(static_cast<std::size_t>(faceCount), {});
        m_wireToFaces.assign(static_cast<std::size_t>(wireCount), {});

        m_wireToEdges.assign(static_cast<std::size_t>(wireCount), {});
        m_edgeToWires.assign(static_cast<std::size_t>(edgeCount), {});

        m_edgeToVertices.assign(static_cast<std::size_t>(edgeCount), {});
        m_vertexToEdges.assign(static_cast<std::size_t>(vertexCount), {});
    }

    void GeometryGraph::addFaceWireRelation(int faceIndex, int wireIndex)
    {
        addRelation(m_faceToWires, faceIndex, wireIndex);
    }

    void GeometryGraph::addWireFaceRelation(int wireIndex, int faceIndex)
    {
        addRelation(m_wireToFaces, wireIndex, faceIndex);
    }

    void GeometryGraph::addWireEdgeRelation(int wireIndex, int edgeIndex)
    {
        // Wire -> Edge は輪郭順序が意味をもつ可能性があるため、
        // addUniqueではなく登録順を保持する
        if (!isValidIndex(wireIndex, static_cast<int>(m_wireToEdges.size())))
        {
            return;
        }

        if (edgeIndex < 0)
        {
            return;
        }

        m_wireToEdges[static_cast<std::size_t>(wireIndex)].push_back(edgeIndex);
    }

    void GeometryGraph::addEdgeWireRelation(int edgeIndex, int wireIndex)
    {
        addRelation(m_edgeToWires, edgeIndex, wireIndex);
    }

    void GeometryGraph::addEdgeVertexRelation(int edgeIndex, int vertexIndex)
    {
        addRelation(m_edgeToVertices, edgeIndex, vertexIndex);
    }

    void GeometryGraph::addVertexEdgeRelation(int vertexIndex, int edgeIndex)
    {
        addRelation(m_vertexToEdges, vertexIndex, edgeIndex);
    }

    const std::vector<int>& GeometryGraph::wiresOfFace(int faceIndex) const
    {
        return listOrEmpty(m_faceToWires, faceIndex);
    }

    const std::vector<int>& GeometryGraph::facesOfWire(int wireIndex) const
    {
        return listOrEmpty(m_wireToFaces, wireIndex);
    }

    const std::vector<int>& GeometryGraph::edgesOfWire(int wireIndex) const
    {
        return listOrEmpty(m_wireToEdges, wireIndex);
    }

    const std::vector<int>& GeometryGraph::wiresOfEdge(int edgeIndex) const
    {
        return listOrEmpty(m_edgeToWires, edgeIndex);
    }

    const std::vector<int>& GeometryGraph::verticesOfEdge(int edgeIndex) const
    {
        return listOrEmpty(m_edgeToVertices, edgeIndex);
    }

    const std::vector<int>& GeometryGraph::edgesOfVertex(int vertexIndex) const
    {
        return listOrEmpty(m_vertexToEdges, vertexIndex);
    }

    std::vector<int> GeometryGraph::adjacentFacesOfFace(int faceIndex) const
    {
        std::vector<int> adjacentFaceIndices;

        const auto& wireIndices = wiresOfFace(faceIndex);

        for (int wireIndex : wireIndices)
        {
            const auto& edgeIndices = edgesOfWire(wireIndex);

            for (int edgeIndex : edgeIndices)
            {
                const auto& connectedWireIndices = wiresOfEdge(edgeIndex);

                for (int connectedWireIndex : connectedWireIndices)
                {
                    if (connectedWireIndex == wireIndex)
                    {
                        continue;
                    }

                    const auto& connectedFaceIndices = facesOfWire(connectedWireIndex);

                    for (int connectedFaceIndex : connectedFaceIndices)
                    {
                        if (connectedFaceIndex == faceIndex)
                        {
                            continue;
                        }

                        addUnique(adjacentFaceIndices, connectedFaceIndex);
                    }
                }
            }
        }

        return adjacentFaceIndices;
    }

    std::vector<int> GeometryGraph::facesOfEdge(int edgeIndex) const
    {
        std::vector<int> faceIndices;

        const auto& wireIndices = wiresOfEdge(edgeIndex);

        for (int wireIndex : wireIndices)
        {
            const auto& connectedFaceIndices = facesOfWire(wireIndex);

            for (int faceIndex : connectedFaceIndices)
            {
                addUnique(faceIndices, faceIndex);
            }
        }

        return faceIndices;
    }

    int GeometryGraph::faceCount() const
    {
        return static_cast<int>(m_faceToWires.size());
    }

    int GeometryGraph::wireCount() const
    {
        return static_cast<int>(m_wireToEdges.size());
    }

    int GeometryGraph::edgeCount() const
    {
        return static_cast<int>(m_edgeToVertices.size());
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
