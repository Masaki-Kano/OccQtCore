#ifndef GEOMETRYMODEL_H
#define GEOMETRYMODEL_H

#include <vector>

#include <TopoDS_Shape.hxx>
#include <TopTools_IndexedMapOfShape.hxx>

#include "Core/PickResult.h"
#include "Geometry/GeometryTypes.h"
#include "Geometry/GeometryGraph.h"

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
        int wireCount() const;
        int edgeCount() const;
        int vertexCount() const;

        const std::vector<FaceData>& faces() const;
        const std::vector<WireData>& wires() const;
        const std::vector<EdgeData>& edges() const;
        const std::vector<VertexData>& vertices() const;

        const FaceData* faceAt(int index) const;
        const WireData* wireAt(int index) const;
        const EdgeData* edgeAt(int index) const;
        const VertexData* vertexAt(int index) const;

        int findFaceIndex(const TopoDS_Shape& shape) const;
        int findWireIndex(const TopoDS_Shape& shape) const;
        int findEdgeIndex(const TopoDS_Shape& shape) const;
        int findVertexIndex(const TopoDS_Shape& shape) const;

        int findElementIndex(const TopoDS_Shape& shape, PickedShapeType type) const;

        const GeometryGraph& graph() const;
        GeometryGraph& graph();

    private:
        void buildFaces(const TopTools_IndexedMapOfShape& faceMap);
        void buildWires(const TopTools_IndexedMapOfShape& wireMap);
        void buildEdges(const TopTools_IndexedMapOfShape& edgeMap);
        void buildVertices(const TopTools_IndexedMapOfShape& vertexMap);

        void buildGraph(
            const TopTools_IndexedMapOfShape& faceMap,
            const TopTools_IndexedMapOfShape& wireMap,
            const TopTools_IndexedMapOfShape& edgeMap,
            const TopTools_IndexedMapOfShape& vertexMap);

    private:
        std::vector<FaceData> m_faces;
        std::vector<WireData> m_wires;
        std::vector<EdgeData> m_edges;
        std::vector<VertexData> m_vertices;

        GeometryGraph m_graph;
    };
}

#endif // GEOMETRYMODEL_H
