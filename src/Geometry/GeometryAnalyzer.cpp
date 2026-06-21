#include "Geometry/GeometryAnalyzer.h"

#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepGProp.hxx>
#include <BRepTools.hxx>
#include <GProp_GProps.hxx>

#include <GeomAbs_CurveType.hxx>
#include <GeomAbs_SurfaceType.hxx>

namespace OccQtCore::GeometryAnalyzer
{
    FaceInfo analyzeFace(const TopoDS_Face& face)
    {
        FaceInfo info;

        if (face.IsNull())
        {
            return info;
        }

        BRepAdaptor_Surface surface(face);

        info.kind = detectSurfaceKind(face);

        // 面積
        GProp_GProps props;
        BRepGProp::SurfaceProperties(face, props);
        info.area = props.Mass();

        // トリム後のUV範囲
        BRepTools::UVBounds(face, info.uMin, info.uMax, info.vMin, info.vMax);

        // 支持曲面の周期情報
        info.isUPeriodic = surface.IsUPeriodic();
        info.isVPeriodic = surface.IsVPeriodic();

        if (info.isUPeriodic)
        {
            info.uPeriod = surface.UPeriod();
        }

        if (info.isVPeriodic)
        {
            info.vPeriod = surface.VPeriod();
        }

        // 種別情報
        switch (info.kind)
        {
        case SurfaceKind::Plane:
            info.plane = extractPlaneInfo(face);
            break;

        case SurfaceKind::Cylinder:
            info.cylinder = extractCylinderInfo(face);
            break;

        case SurfaceKind::Cone:
            info.cone = extractConeInfo(face);
            break;

        case SurfaceKind::Sphere:
            info.sphere = extractSphereInfo(face);
            break;

        case SurfaceKind::Torus:
            info.torus = extractTorusInfo(face);
            break;

        default:
            break;
        }

        return info;
    }

    EdgeInfo analyzeEdge(const TopoDS_Edge& edge)
    {
        EdgeInfo info;

        if (edge.IsNull())
        {
            return info;
        }

        info.kind = detectCurveKind(edge);

        BRepAdaptor_Curve curve(edge);

        info.firstParameter = curve.FirstParameter();
        info.lastParameter = curve.LastParameter();

        // TopoDSとしての閉性
        info.isClosed = edge.Closed();

        // 円錐頂点や球面極などの縮退Edge
        info.isDegenerated = BRep_Tool::Degenerated(edge);

        GProp_GProps props;
        BRepGProp::LinearProperties(edge, props);
        info.length = props.Mass();

        switch (info.kind)
        {
        case CurveKind::Line:
            info.line = extractLineInfo(edge);
            break;

        case CurveKind::Circle:
            info.circle = extractCircleInfo(edge);
            break;

        case CurveKind::Ellipse:
            info.ellipse = extractEllipseInfo(edge);
            break;

        default:
            break;
        }

        return info;
    }

    VertexInfo analyzeVertex(const TopoDS_Vertex& vertex)
    {
        VertexInfo info;

        if (vertex.IsNull())
        {
            return info;
        }

        info.point = BRep_Tool::Pnt(vertex);
        info.hasPoint = true;

        return info;
    }

    SurfaceKind detectSurfaceKind(const TopoDS_Face& face)
    {
        if (face.IsNull())
        {
            return SurfaceKind::Unknown;
        }

        BRepAdaptor_Surface surface(face);

        switch (surface.GetType())
        {
        case GeomAbs_Plane:
            return SurfaceKind::Plane;

        case GeomAbs_Cylinder:
            return SurfaceKind::Cylinder;

        case GeomAbs_Cone:
            return SurfaceKind::Cone;

        case GeomAbs_Sphere:
            return SurfaceKind::Sphere;

        case GeomAbs_Torus:
            return SurfaceKind::Torus;

        case GeomAbs_BezierSurface:
            return SurfaceKind::BezierSurface;

        case GeomAbs_BSplineSurface:
            return SurfaceKind::BSplineSurface;

        default:
            return SurfaceKind::Other;
        }
    }

    CurveKind detectCurveKind(const TopoDS_Edge& edge)
    {
        if (edge.IsNull())
        {
            return CurveKind::Unknown;
        }

        BRepAdaptor_Curve curve(edge);

        switch (curve.GetType())
        {
        case GeomAbs_Line:
            return CurveKind::Line;

        case GeomAbs_Circle:
            return CurveKind::Circle;

        case GeomAbs_Ellipse:
            return CurveKind::Ellipse;

        case GeomAbs_Hyperbola:
            return CurveKind::Hyperbola;

        case GeomAbs_Parabola:
            return CurveKind::Parabola;

        case GeomAbs_BezierCurve:
            return CurveKind::BezierCurve;

        case GeomAbs_BSplineCurve:
            return CurveKind::BSplineCurve;

        default:
            return CurveKind::Other;
        }
    }

    std::optional<PlaneInfo> extractPlaneInfo(const TopoDS_Face& face)
    {
        if (face.IsNull())
        {
            return std::nullopt;
        }

        BRepAdaptor_Surface surface(face);

        if (surface.GetType() != GeomAbs_Plane)
        {
            return std::nullopt;
        }

        const gp_Pln plane = surface.Plane();

        PlaneInfo info;
        info.origin = plane.Location();
        info.normal = plane.Axis().Direction();

        return info;
    }

    std::optional<CylinderInfo> extractCylinderInfo(const TopoDS_Face& face)
    {
        if (face.IsNull())
        {
            return std::nullopt;
        }

        BRepAdaptor_Surface surface(face);

        if (surface.GetType() != GeomAbs_Cylinder)
        {
            return std::nullopt;
        }

        const gp_Cylinder cylinder = surface.Cylinder();

        CylinderInfo info;
        info.axis = cylinder.Axis();
        info.radius = cylinder.Radius();

        return info;
    }

    std::optional<ConeInfo> extractConeInfo(const TopoDS_Face& face)
    {
        if (face.IsNull())
        {
            return std::nullopt;
        }

        BRepAdaptor_Surface surface(face);

        if (surface.GetType() != GeomAbs_Cone)
        {
            return std::nullopt;
        }

        const gp_Cone cone = surface.Cone();

        ConeInfo info;
        info.axis = cone.Axis();
        info.apex = cone.Apex();
        info.hasApex = true;
        info.semiAngle = cone.SemiAngle();
        info.refRadius = cone.RefRadius();

        return info;
    }

    std::optional<SphereInfo> extractSphereInfo(const TopoDS_Face& face)
    {
        if (face.IsNull())
        {
            return std::nullopt;
        }

        BRepAdaptor_Surface surface(face);

        if (surface.GetType() != GeomAbs_Sphere)
        {
            return std::nullopt;
        }

        const gp_Sphere sphere = surface.Sphere();

        SphereInfo info;
        info.center = sphere.Location();
        info.radius = sphere.Radius();

        return info;
    }

    std::optional<TorusInfo> extractTorusInfo(const TopoDS_Face& face)
    {
        if (face.IsNull())
        {
            return std::nullopt;
        }

        BRepAdaptor_Surface surface(face);

        if (surface.GetType() != GeomAbs_Torus)
        {
            return std::nullopt;
        }

        const gp_Torus torus = surface.Torus();

        TorusInfo info;
        info.axis = torus.Axis();
        info.majorRadius = torus.MajorRadius();
        info.minorRadius = torus.MinorRadius();

        return info;
    }

    std::optional<LineInfo> extractLineInfo(const TopoDS_Edge& edge)
    {
        if (edge.IsNull())
        {
            return std::nullopt;
        }

        BRepAdaptor_Curve curve(edge);

        if (curve.GetType() != GeomAbs_Line)
        {
            return std::nullopt;
        }

        const gp_Lin line = curve.Line();

        LineInfo info;
        info.origin = line.Location();
        info.direction = line.Direction();

        return info;
    }

    std::optional<CircleInfo> extractCircleInfo(const TopoDS_Edge& edge)
    {
        if (edge.IsNull())
        {
            return std::nullopt;
        }

        BRepAdaptor_Curve curve(edge);

        if (curve.GetType() != GeomAbs_Circle)
        {
            return std::nullopt;
        }

        const gp_Circ circle = curve.Circle();

        CircleInfo info;
        info.position = circle.Position();
        info.radius = circle.Radius();

        return info;
    }

    std::optional<EllipseInfo> extractEllipseInfo(const TopoDS_Edge& edge)
    {
        if (edge.IsNull())
        {
            return std::nullopt;
        }

        BRepAdaptor_Curve curve(edge);

        if (curve.GetType() != GeomAbs_Ellipse)
        {
            return std::nullopt;
        }

        const gp_Elips ellipse = curve.Ellipse();

        EllipseInfo info;
        info.position = ellipse.Position();
        info.majorRadius = ellipse.MajorRadius();
        info.minorRadius = ellipse.MinorRadius();

        return info;
    }
}
