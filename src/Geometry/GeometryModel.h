#ifndef GEOMETRYMODEL_H
#define GEOMETRYMODEL_H

#include <vector>

#include <TopoDS_Shape.hxx>

#include "Core/PickResult.h"
#include "Geometry/GeometryTypes.h"

namespace OccQtCore
{
    class GeometryModel
    {
    public:
        GeometryModel() = default;

        void clear();
        void build(const TopoDS_Shape& rootShape);

        bool isEmpty() const;

        int faceCount() const;
        int edgeCount() const;
        int vertexCount() const;

        const std::vector<FaceData>& faces() const;
        const std::vector<EdgeData>& edges() const;
        const std::vector<VertexData>& vertices() const;

        const FaceData* faceAt(int index) const;
        const EdgeData* edgeAt(int index) const;
        const VertexData* vertexAt(int index) const;

        int findFaceIndex(const TopoDS_Shape& shape) const;
        int findEdgeIndex(const TopoDS_Shape& shape) const;
        int findVertexIndex(const TopoDS_Shape& shape) const;

        int findElementIndex(const TopoDS_Shape& shape, PickedShapeType type) const;

    private:
        std::vector<FaceData> m_faces;
        std::vector<EdgeData> m_edges;
        std::vector<VertexData> m_vertices;
    };
}

#endif // GEOMETRYMODEL_H
