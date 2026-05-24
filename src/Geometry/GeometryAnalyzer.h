#ifndef GEOMETRYANALYZER_H
#define GEOMETRYANALYZER_H

#include <optional>

#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>

#include "Geometry/GeometryTypes.h"

namespace OccQtCore::GeometryAnalyzer
{
    FaceInfo analyzeFace(const TopoDS_Face& face);
    EdgeInfo analyzeEdge(const TopoDS_Edge& edge);
    VertexInfo analyzeVertex(const TopoDS_Vertex& vertex);

    SurfaceKind detectSurfaceKind(const TopoDS_Face& face);
    CurveKind detectCurveKind(const TopoDS_Edge& edge);

    std::optional<PlaneInfo> extractPlaneInfo(const TopoDS_Face& face);
    std::optional<CylinderInfo> extractCylinderInfo(const TopoDS_Face& face);
    std::optional<ConeInfo> extractConeInfo(const TopoDS_Face& face);
    std::optional<SphereInfo> extractSphereInfo(const TopoDS_Face& face);
    std::optional<TorusInfo> extractTorusInfo(const TopoDS_Face& face);

    std::optional<LineInfo> extractLineInfo(const TopoDS_Edge& edge);
    std::optional<CircleInfo> extractCircleInfo(const TopoDS_Edge& edge);
    std::optional<EllipseInfo> extractEllipseInfo(const TopoDS_Edge& edge);
}


#endif // GEOMETRYANALYZER_H
