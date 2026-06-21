#include "Geometry/GeometryGraph.h"
#include "Core/CollectionUtil.h"

namespace
{
    bool isValidIndex(int index, int count)
    {
        return index >= 0 && index < count;
    }

    const std::vector<int>& emptyIndexList()
    {
        static const std::vector<int> empty;
        return empty;
    }

    const std::vector<OccQtCore::OrientedWireRef>& emptyWireRefList()
    {
        static const std::vector<OccQtCore::OrientedWireRef> empty;
        return empty;
    }

    const std::vector<OccQtCore::OrientedEdgeRef>& emptyEdgeRefList()
    {
        static const std::vector<OccQtCore::OrientedEdgeRef> empty;
        return empty;
    }

    const std::vector<int>& listOrEmpty(
        const std::vector<std::vector<int>>& lists,
        int index)
    {
        if (!isValidIndex(index, static_cast<int>(lists.size())))
        {
            return emptyIndexList();
        }

        return lists[static_cast<std::size_t>(index)];
    }

    void addRelation(
        std::vector<std::vector<int>>& relations,
        int fromIndex,
        int toIndex)
    {
        if (!isValidIndex(
                fromIndex,
                static_cast<int>(relations.size())))
        {
            return;
        }

        if (toIndex < 0)
        {
            return;
        }

        OccQtCore::CollectionUtil::addUnique(
            relations[static_cast<std::size_t>(fromIndex)],
            toIndex);
    }
}


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

    void GeometryGraph::addFaceWireRelation(
        int faceIndex,
        int wireIndex,
        TopAbs_Orientation orientation,
        bool isOuter,
        bool isInner)
    {
        if (!isValidIndex(faceIndex, static_cast<int>(m_faceToWires.size())))
        {
            return;
        }

        if (!isValidIndex(wireIndex, static_cast<int>(m_wireToFaces.size())))
        {
            return;
        }

        auto& refs = m_faceToWires[static_cast<std::size_t>(faceIndex)];

        for (const auto& ref : refs)
        {
            if (ref.wireIndex == wireIndex)
            {
                return;
            }
        }

        OrientedWireRef ref;
        ref.wireIndex = wireIndex;
        ref.orientation = orientation;
        ref.isOuter = isOuter;
        ref.isInner = isInner;

        refs.push_back(ref);
    }

    void GeometryGraph::addWireFaceRelation(int wireIndex, int faceIndex)
    {
        addRelation(m_wireToFaces, wireIndex, faceIndex);
    }

    void GeometryGraph::addWireEdgeRelation(int wireIndex, int edgeIndex, TopAbs_Orientation orientation)
    {
        if (!isValidIndex(
                wireIndex,
                static_cast<int>(m_wireToEdges.size())))
        {
            return;
        }

        if (!isValidIndex(
                edgeIndex,
                static_cast<int>(m_edgeToWires.size())))
        {
            return;
        }

        OrientedEdgeRef ref;
        ref.edgeIndex = edgeIndex;
        ref.orientation = orientation;

        // Wire内の順序・重複・向きをそのまま保持する。
        // シームEdgeは同一Wire内に複数回現れ得る。
        m_wireToEdges[
            static_cast<std::size_t>(wireIndex)]
            .push_back(ref);
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

    const std::vector<OrientedWireRef>&
    GeometryGraph::wireRefsOfFace(int faceIndex) const
    {
        if (!isValidIndex(
                faceIndex,
                static_cast<int>(m_faceToWires.size())))
        {
            return emptyWireRefList();
        }

        return m_faceToWires[
            static_cast<std::size_t>(faceIndex)];
    }

    const std::vector<int>&
    GeometryGraph::facesOfWire(int wireIndex) const
    {
        return listOrEmpty(
            m_wireToFaces,
            wireIndex);
    }

    const std::vector<OrientedEdgeRef>&
    GeometryGraph::edgeRefsOfWire(int wireIndex) const
    {
        if (!isValidIndex(
                wireIndex,
                static_cast<int>(m_wireToEdges.size())))
        {
            return emptyEdgeRefList();
        }

        return m_wireToEdges[
            static_cast<std::size_t>(wireIndex)];
    }

    const std::vector<int>&
    GeometryGraph::wiresOfEdge(int edgeIndex) const
    {
        return listOrEmpty(
            m_edgeToWires,
            edgeIndex);
    }

    const std::vector<int>&
    GeometryGraph::verticesOfEdge(int edgeIndex) const
    {
        return listOrEmpty(
            m_edgeToVertices,
            edgeIndex);
    }

    const std::vector<int>&
    GeometryGraph::edgesOfVertex(int vertexIndex) const
    {
        return listOrEmpty(
            m_vertexToEdges,
            vertexIndex);
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
}
