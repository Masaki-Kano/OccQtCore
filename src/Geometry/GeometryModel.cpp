#include "Geometry/GeometryModel.h"
#include "Geometry/GeometryAnalyzer.h"

#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>

namespace OccQtCore
{
    void GeometryModel::clear()
    {
        m_faces.clear();
        m_edges.clear();
        m_vertices.clear();

        m_graph.clear();
    }

    void GeometryModel::build(const TopoDS_Shape& rootShape)
    {
        clear();

        if (rootShape.IsNull())
        {
            return;
        }

        TopTools_IndexedMapOfShape faceMap;
        TopTools_IndexedMapOfShape edgeMap;
        TopTools_IndexedMapOfShape vertexMap;

        TopExp::MapShapes(rootShape, TopAbs_FACE, faceMap);
        TopExp::MapShapes(rootShape, TopAbs_EDGE, edgeMap);
        TopExp::MapShapes(rootShape, TopAbs_VERTEX, vertexMap);

        buildFaces(faceMap);
        buildEdges(edgeMap);
        buildVertices(vertexMap);

        buildGraph(faceMap, edgeMap, vertexMap);
    }

    bool GeometryModel::isEmpty() const
    {
        return m_faces.empty() && m_edges.empty() && m_vertices.empty();
    }

    int GeometryModel::faceCount() const
    {
        return static_cast<int>(m_faces.size());
    }

    int GeometryModel::edgeCount() const
    {
        return static_cast<int>(m_edges.size());
    }

    int GeometryModel::vertexCount() const
    {
        return static_cast<int>(m_vertices.size());
    }

    const std::vector<FaceData>& GeometryModel::faces() const
    {
        return m_faces;
    }

    const std::vector<EdgeData>& GeometryModel::edges() const
    {
        return m_edges;
    }

    const std::vector<VertexData>& GeometryModel::vertices() const
    {
        return m_vertices;
    }

    const FaceData* GeometryModel::faceAt(int index) const
    {
        if (index < 0 || index >= faceCount())
        {
            return nullptr;
        }

        return &m_faces[static_cast<std::size_t>(index)];
    }

    const EdgeData* GeometryModel::edgeAt(int index) const
    {
        if (index < 0 || index >= edgeCount())
        {
            return nullptr;
        }

        return &m_edges[static_cast<std::size_t>(index)];
    }

    const VertexData* GeometryModel::vertexAt(int index) const
    {
        if (index < 0 || index >= vertexCount())
        {
            return nullptr;
        }

        return &m_vertices[static_cast<std::size_t>(index)];
    }

    int GeometryModel::findFaceIndex(const TopoDS_Shape& shape) const
    {
        if (shape.IsNull())
        {
            return -1;
        }

        for (const auto& face : m_faces)
        {
            if (face.shape.IsSame(shape))
            {
                return face.index;
            }
        }

        return -1;
    }

    int GeometryModel::findEdgeIndex(const TopoDS_Shape& shape) const
    {
        if (shape.IsNull())
        {
            return -1;
        }

        for (const auto& edge : m_edges)
        {
            if (edge.shape.IsSame(shape))
            {
                return edge.index;
            }
        }

        return -1;
    }

    int GeometryModel::findVertexIndex(const TopoDS_Shape& shape) const
    {
        if (shape.IsNull())
        {
            return -1;
        }

        for (const auto& vertex : m_vertices)
        {
            if (vertex.shape.IsSame(shape))
            {
                return vertex.index;
            }
        }

        return -1;
    }

    int GeometryModel::findElementIndex(const TopoDS_Shape& shape, PickedShapeType type) const
    {
        switch (type)
        {
        case PickedShapeType::Face:
            return findFaceIndex(shape);

        case PickedShapeType::Edge:
            return findEdgeIndex(shape);

        case PickedShapeType::Vertex:
            return findVertexIndex(shape);

        default:
            return -1;
        }
    }

    const GeometryGraph& GeometryModel::graph() const
    {
        return m_graph;
    }

    GeometryGraph& GeometryModel::graph()
    {
        return m_graph;
    }

    void GeometryModel::buildFaces(const TopTools_IndexedMapOfShape& faceMap)
    {
        m_faces.reserve(static_cast<std::size_t>(faceMap.Extent()));

        for (int mapIndex = 1; mapIndex <= faceMap.Extent(); ++mapIndex)
        {
            const int index = mapIndex - 1;
            const TopoDS_Face face = TopoDS::Face(faceMap.FindKey(mapIndex));

            FaceData data;
            data.index = index;
            data.shape = face;

            data.info = GeometryAnalyzer::analyzeFace(face);

            m_faces.push_back(data);
        }
    }

    void GeometryModel::buildEdges(const TopTools_IndexedMapOfShape& edgeMap)
    {
        m_edges.reserve(static_cast<std::size_t>(edgeMap.Extent()));

        for (int mapIndex = 1; mapIndex <= edgeMap.Extent(); ++mapIndex)
        {
            const int index = mapIndex - 1;
            const TopoDS_Edge edge = TopoDS::Edge(edgeMap.FindKey(mapIndex));

            EdgeData data;
            data.index = index;
            data.shape = edge;

            data.info = GeometryAnalyzer::analyzeEdge(edge);

            m_edges.push_back(data);
        }
    }

    void GeometryModel::buildVertices(const TopTools_IndexedMapOfShape& vertexMap)
    {
        m_vertices.reserve(static_cast<std::size_t>(vertexMap.Extent()));

        for (int mapIndex = 1; mapIndex <= vertexMap.Extent(); ++mapIndex)
        {
            const int index = mapIndex - 1;
            const TopoDS_Vertex vertex = TopoDS::Vertex(vertexMap.FindKey(mapIndex));

            VertexData data;
            data.index = index;
            data.shape = vertex;

            data.info = GeometryAnalyzer::analyzeVertex(vertex);

            m_vertices.push_back(data);
        }
    }

    void GeometryModel::buildGraph(
        const TopTools_IndexedMapOfShape& faceMap,
        const TopTools_IndexedMapOfShape& edgeMap,
        const TopTools_IndexedMapOfShape& vertexMap)
    {
        m_graph.clear();
        m_graph.resize(faceCount(), edgeCount(), vertexCount());

        for (int faceMapIndex = 1; faceMapIndex <= faceMap.Extent(); ++faceMapIndex)
        {
            const int faceIndex = faceMapIndex - 1;
            const TopoDS_Face face = TopoDS::Face(faceMap.FindKey(faceMapIndex));

            for (TopExp_Explorer edgeExp(face, TopAbs_EDGE);
                 edgeExp.More();
                 edgeExp.Next())
            {
                const TopoDS_Edge edge = TopoDS::Edge(edgeExp.Current());
                const int edgeMapIndex = edgeMap.FindIndex(edge);

                if (edgeMapIndex <= 0)
                {
                    continue;
                }

                const int edgeIndex = edgeMapIndex - 1;

                m_graph.addFaceEdgeRelation(faceIndex, edgeIndex);
                m_graph.addEdgeFaceRelation(edgeIndex, faceIndex);
            }
        }

        for (int edgeMapIndex = 1; edgeMapIndex <= edgeMap.Extent(); ++edgeMapIndex)
        {
            const int edgeIndex = edgeMapIndex - 1;
            const TopoDS_Edge edge = TopoDS::Edge(edgeMap.FindKey(edgeMapIndex));

            for (TopExp_Explorer vertexExp(edge, TopAbs_VERTEX);
                 vertexExp.More();
                 vertexExp.Next())
            {
                const TopoDS_Vertex vertex = TopoDS::Vertex(vertexExp.Current());
                const int vertexMapIndex = vertexMap.FindIndex(vertex);

                if (vertexMapIndex <= 0)
                {
                    continue;
                }

                const int vertexIndex = vertexMapIndex - 1;

                m_graph.addEdgeVertexRelation(edgeIndex, vertexIndex);
                m_graph.addVertexEdgeRelation(vertexIndex, edgeIndex);
            }
        }
    }
}
