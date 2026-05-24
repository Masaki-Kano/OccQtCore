#ifndef GEOMETRYTYPES_H
#define GEOMETRYTYPES_H

#include <optional>

#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>

#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <gp_Ax1.hxx>

namespace OccQtCore
{
    enum class SurfaceKind
    {
        Unknown,
        Plane,
        Cylinder,
        Cone,
        Sphere,
        Torus,
        BezierSurface,
        BSplineSurface,
        Other
    };

    enum class CurveKind
    {
        Unknown,
        Line,
        Circle,
        Ellipse,
        Hyperbola,
        Parabola,
        BezierCurve,
        BSplineCurve,
        Other
    };

    struct PlaneInfo
    {
        gp_Pnt origin;
        gp_Dir normal;
    };

    struct CylinderInfo
    {
        gp_Ax1 axis;
        double radius = 0.0;
    };

    struct ConeInfo
    {
        gp_Ax1 axis;
        double semiAngle = 0.0;
        double refRadius = 0.0;
    };

    struct SphereInfo
    {
        gp_Pnt center;
        double radius = 0.0;
    };

    struct TorusInfo
    {
        gp_Ax1 axis;
        double majorRadius = 0.0;
        double minorRadius = 0.0;
    };

    struct LineInfo
    {
        gp_Pnt origin;
        gp_Dir direction;
    };

    struct CircleInfo
    {
        gp_Pnt center;
        gp_Ax1 axis;
        double radius = 0.0;
    };

    struct EllipseInfo
    {
        gp_Pnt center;
        gp_Ax1 axis;
        double majorRadius = 0.0;
        double minorRadius = 0.0;
    };

    struct FaceInfo
    {
        SurfaceKind kind = SurfaceKind::Unknown;

        double area = 0.0;

        double uMin = 0.0;
        double uMax = 0.0;
        double vMin = 0.0;
        double vMax = 0.0;

        std::optional<PlaneInfo> plane;
        std::optional<CylinderInfo> cylinder;
        std::optional<ConeInfo> cone;
        std::optional<SphereInfo> sphere;
        std::optional<TorusInfo> torus;
    };

    struct EdgeInfo
    {
        CurveKind kind = CurveKind::Unknown;

        double length = 0.0;

        double firstParameter = 0.0;
        double lastParameter = 0.0;

        std::optional<LineInfo> line;
        std::optional<CircleInfo> circle;
        std::optional<EllipseInfo> ellipse;
    };

    struct VertexInfo
    {
        gp_Pnt point;
        bool hasPoint = false;
    };

    struct FaceData
    {
        int index = -1;
        TopoDS_Face shape;
        FaceInfo info;
    };

    struct EdgeData
    {
        int index = -1;
        TopoDS_Edge shape;
        EdgeInfo info;
    };

    struct VertexData
    {
        int index = -1;
        TopoDS_Vertex shape;
        VertexInfo info;
    };

    inline const char* surfaceKindDisplayName(SurfaceKind kind)
    {
        switch (kind)
        {
        case SurfaceKind::Unknown:
            return "不明";
        case SurfaceKind::Plane:
            return "平面";
        case SurfaceKind::Cylinder:
            return "円筒面";
        case SurfaceKind::Cone:
            return "円錐面";
        case SurfaceKind::Sphere:
            return "球面";
        case SurfaceKind::Torus:
            return "トーラス面";
        case SurfaceKind::BezierSurface:
            return "ベジェ曲面";
        case SurfaceKind::BSplineSurface:
            return "Bスプライン曲面";
        case SurfaceKind::Other:
            return "その他";
        default:
            return "不明";
        }
    }

    inline const char* curveKindDisplayName(CurveKind kind)
    {
        switch (kind)
        {
        case CurveKind::Unknown:
            return "不明";
        case CurveKind::Line:
            return "直線";
        case CurveKind::Circle:
            return "円";
        case CurveKind::Ellipse:
            return "楕円";
        case CurveKind::Hyperbola:
            return "双曲線";
        case CurveKind::Parabola:
            return "放物線";
        case CurveKind::BezierCurve:
            return "ベジェ曲線";
        case CurveKind::BSplineCurve:
            return "Bスプライン曲線";
        case CurveKind::Other:
            return "その他";
        default:
            return "不明";
        }
    }
}

#endif // GEOMETRYTYPES_H
