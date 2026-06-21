#ifndef GEOMETRYTYPES_H
#define GEOMETRYTYPES_H

#include <optional>

#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Vertex.hxx>

#include <TopAbs_Orientation.hxx>

#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>

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

        // 同一支持円錐判定をしやすくなるため保持
        gp_Pnt apex;
        bool hasApex = false;

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
        // 中心、法線、面内方向を保持
        gp_Ax2 position;
        double radius = 0.0;
    };

    struct EllipseInfo
    {
        // 長軸方向まで保持
        gp_Ax2 position;

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

        // 支持曲面の周期情報
        bool isUPeriodic = false;
        bool isVPeriodic = false;

        double uPeriod = 0.0;
        double vPeriod = 0.0;

        std::optional<PlaneInfo> plane;
        std::optional<CylinderInfo> cylinder;
        std::optional<ConeInfo> cone;
        std::optional<SphereInfo> sphere;
        std::optional<TorusInfo> torus;
    };

    struct WireInfo
    {
        bool isClosed = false;
        int edgeCount = 0;
    };

    struct EdgeInfo
    {
        CurveKind kind = CurveKind::Unknown;

        double length = 0.0;

        double firstParameter = 0.0;
        double lastParameter = 0.0;

        bool isClosed = false;
        bool isDegenerated = false;

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

        // TopoDS_Faceとしての向き
        // 支持曲面の法線方向と組み合わせて内向き・外向きを導出する
        TopAbs_Orientation orientation = TopAbs_FORWARD;

        FaceInfo info;
    };

    struct WireData
    {
        int index = -1;
        TopoDS_Wire shape;

        TopAbs_Orientation orientation = TopAbs_FORWARD;

        WireInfo info;
    };

    struct EdgeData
    {
        int index = -1;
        TopoDS_Edge shape;

        // Edge単体としての向き
        // Wire内での向きはOrientedEdgeRef側で保持する
        TopAbs_Orientation orientation = TopAbs_FORWARD;

        EdgeInfo info;
    };

    struct VertexData
    {
        int index = -1;
        TopoDS_Vertex shape;

        TopAbs_Orientation orientation = TopAbs_FORWARD;

        VertexInfo info;
    };

    // Wire内におけるEdgeの参照。
    // Edgeの登録順だけでなく、そのWire内での向きも保持する
    struct OrientedEdgeRef
    {
        int edgeIndex = -1;
        TopAbs_Orientation orientation = TopAbs_FORWARD;
    };

    // Face内におけるWireの参照。
    // Outer / Inner判定を将来的にFace-Wire関係側へ移せるようにする
    struct OrientedWireRef
    {
        int wireIndex = -1;
        TopAbs_Orientation orientation = TopAbs_FORWARD;

        bool isOuter = false;
        bool isInner = false;
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

    inline const char* orientationDisplayName(TopAbs_Orientation orientation)
    {
        switch (orientation)
        {
        case TopAbs_FORWARD:
            return "FORWARD";

        case TopAbs_REVERSED:
            return "REVERSED";

        case TopAbs_INTERNAL:
            return "INTERNAL";

        case TopAbs_EXTERNAL:
            return "EXTERNAL";

        default:
            return "UNKNOWN";
        }
    }
}

#endif // GEOMETRYTYPES_H
