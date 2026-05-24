#include "ShapeIndex.h"

#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>

namespace OccQtCore
{
    void ShapeIndex::clear()
    {
        m_faces.clear();
        m_edges.clear();
        m_vertices.clear();
    }

    void ShapeIndex::build(const TopoDS_Shape& rootShape)
    {
        clear();

        if (rootShape.IsNull())
        {
            return;
        }

        for (TopExp_Explorer explorer(rootShape, TopAbs_FACE);
             explorer.More();
             explorer.Next())
        {
            m_faces.push_back(explorer.Current());
        }

        for (TopExp_Explorer explorer(rootShape, TopAbs_EDGE);
             explorer.More();
             explorer.Next())
        {
            m_edges.push_back(explorer.Current());
        }

        for (TopExp_Explorer explorer(rootShape, TopAbs_VERTEX);
             explorer.More();
             explorer.Next())
        {
            m_vertices.push_back(explorer.Current());
        }
    }

    int ShapeIndex::faceCount() const
    {
        return static_cast<int>(m_faces.size());
    }

    int ShapeIndex::edgeCount() const
    {
        return static_cast<int>(m_edges.size());
    }

    int ShapeIndex::vertexCount() const
    {
        return static_cast<int>(m_vertices.size());
    }

    int ShapeIndex::findFaceIndex(const TopoDS_Shape& shape) const
    {
        return findShapeIndex(m_faces, shape);
    }

    int ShapeIndex::findEdgeIndex(const TopoDS_Shape& shape) const
    {
        return findShapeIndex(m_edges, shape);
    }

    int ShapeIndex::findVertexIndex(const TopoDS_Shape& shape) const
    {
        return findShapeIndex(m_vertices, shape);
    }

    const TopoDS_Shape& ShapeIndex::faceAt(int index) const
    {
        return m_faces.at(static_cast<size_t>(index));
    }

    const TopoDS_Shape& ShapeIndex::edgeAt(int index) const
    {
        return m_edges.at(static_cast<size_t>(index));
    }

    const TopoDS_Shape& ShapeIndex::vertexAt(int index) const
    {
        return m_vertices.at(static_cast<size_t>(index));
    }

    int ShapeIndex::findShapeIndex(
        const std::vector<TopoDS_Shape>& shapes,
        const TopoDS_Shape& target) const
    {
        if (target.IsNull())
        {
            return -1;
        }

        for (int i = 0; i < static_cast<int>(shapes.size()); ++i)
        {
            if (shapes[i].IsSame(target))
            {
                return i;
            }
        }

        return -1;
    }


}
