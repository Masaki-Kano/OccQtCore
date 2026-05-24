#ifndef SHAPEINDEX_H
#define SHAPEINDEX_H

#include <vector>

#include <TopoDS_Shape.hxx>

namespace OccQtCore
{
    class ShapeIndex
    {
    public:
        void clear();
        void build(const TopoDS_Shape& rootShape);

        int faceCount() const;
        int edgeCount() const;
        int vertexCount() const;

        int findFaceIndex(const TopoDS_Shape& shape) const;
        int findEdgeIndex(const TopoDS_Shape& shape) const;
        int findVertexIndex(const TopoDS_Shape& shape) const;

        const TopoDS_Shape& faceAt(int index) const;
        const TopoDS_Shape& edgeAt(int index) const;
        const TopoDS_Shape& vertexAt(int index) const;

    private:
        int findShapeIndex(
            const std::vector<TopoDS_Shape>& shapes,
            const TopoDS_Shape& target) const;

    private:
        std::vector<TopoDS_Shape> m_faces;
        std::vector<TopoDS_Shape> m_edges;
        std::vector<TopoDS_Shape> m_vertices;
    };
}

#endif // SHAPEINDEX_H
