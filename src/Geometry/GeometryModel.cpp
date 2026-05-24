#include "Geometry/GeometryModel.h"
#include "Geometry/GeometryAnalyzer.h"

#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>

namespace OccQtCore
{
    void GeometryModel::clear()
    {
        m_faces.clear();
        m_edges.clear();
        m_vertices.clear();
    }

    void GeometryModel::build(const TopoDS_Shape& rootShape)
    {
        clear();

        if (rootShape.IsNull())
        {
            return;
        }

        int faceIndex = 0;
        for (TopExp_Explorer explorer(rootShape, TopAbs_FACE); explorer.More(); explorer.Next())
        {
            FaceData data;
            data.index = faceIndex++;
            data.shape = TopoDS::Face(explorer.Current());
            data.info = GeometryAnalyzer::analyzeFace(data.shape);

            m_faces.push_back(data);
        }

        int edgeIndex = 0;
        for (TopExp_Explorer explorer(rootShape, TopAbs_EDGE); explorer.More(); explorer.Next())
        {
            EdgeData data;
            data.index = edgeIndex++;
            data.shape = TopoDS::Edge(explorer.Current());
            data.info = GeometryAnalyzer::analyzeEdge(data.shape);

            m_edges.push_back(data);
        }

        int vertexIndex = 0;
        for (TopExp_Explorer explorer(rootShape, TopAbs_VERTEX); explorer.More(); explorer.Next())
        {
            VertexData data;
            data.index = vertexIndex++;
            data.shape = TopoDS::Vertex(explorer.Current());
            data.info = GeometryAnalyzer::analyzeVertex(data.shape);

            m_vertices.push_back(data);
        }
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
}
