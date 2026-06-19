#include <QString>

#include "Log/GeometryLogReporter.h"
#include "Log/AppLogger.h"
#include "Log/LogFormatUtil.h"

#include "Geometry/GeometryModel.h"
#include "Geometry/TopologyQuery.h"

namespace
{
    namespace LF = OccQtCore::LogFormatUtil;
}

namespace OccQtCore
{
    GeometryLogReporter::GeometryLogReporter(AppLogger* logger)
        : m_logger(logger)
    {
    }

    void GeometryLogReporter::logGeometry(const GeometryLogReport& report) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

        m_logger->info("===== 形状解析ログ開始 =====");

        if (report.outputSummary)
        {
            logSummary(report.model);
        }

        if (report.outputTopologySummary)
        {
            logTopologySummary(report.model);
        }

        if (report.outputComplexGeometrySummary)
        {
            logComplexGeometrySummary(report.model);
        }

        if (report.outputDetailDiagnostics)
        {
            logDetailDiagnostics(report.model);
        }

        m_logger->info("===== 形状解析ログ終了 =====");
    }

    void GeometryLogReporter::logGeometryElement(const GeometryElementLogReport& report) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

        if (report.elementIndex < 0)
        {
            m_logger->warn("選択形状のインデックスが取得できないため、形状詳細ログを出力しません。");
            return;
        }

        switch (report.type)
        {
        case GeometryElementKind::Face:
            if (report.outputTree)
            {
                logFaceTreeDetails(report.model, report.elementIndex);
            }
            else
            {
                logFaceDetails(report.model, report.elementIndex);
            }
            break;

        case GeometryElementKind::Wire:
            if (report.outputTree)
            {
                logWireTreeDetails(report.model, report.elementIndex);
            }
            else
            {
                logWireDetails(report.model, report.elementIndex);
            }
            break;

        case GeometryElementKind::Edge:
            logEdgeDetails(report.model, report.elementIndex);
            break;

        case GeometryElementKind::Vertex:
            logVertexDetails(report.model, report.elementIndex);
            break;

        case GeometryElementKind::Unknown:
        case GeometryElementKind::Shell:
        case GeometryElementKind::Solid:
        case GeometryElementKind::Compound:
        default:
            break;
        }
    }

    void GeometryLogReporter::logSummary(const GeometryModel& model) const
    {
        m_logger->info("===== 形状概要 =====");

        m_logger->info(QString("要素数: Face=%1, Wire=%2, Edge=%4, Vertex=%5")
                        .arg(model.faceCount())
                        .arg(model.wireCount())
                        .arg(model.edgeCount())
                        .arg(model.vertexCount()));
    }

    void GeometryLogReporter::logTopologySummary(const GeometryModel& model) const
    {
        Q_UNUSED(model);

        m_logger->info("----- トポロジー概要 -----");

        // 次で実装する。
    }

    void GeometryLogReporter::logComplexGeometrySummary(const GeometryModel& model) const
    {
        Q_UNUSED(model);

        m_logger->info("----- 複雑形状サマリ -----");

        // 次で実装する。
    }

    void GeometryLogReporter::logDetailDiagnostics(const GeometryModel& model) const
    {
        m_logger->info("===== 形状詳細診断ログ開始 =====");

        m_logger->info("----- Face階層詳細 -----");
        for (int faceIndex = 0; faceIndex < model.faceCount(); ++faceIndex)
        {
            logFaceTreeDetails(model, faceIndex);
        }

        m_logger->info("===== 形状詳細診断ログ終了 =====");
    }

    void GeometryLogReporter::logFaceDetails(const GeometryModel& model, int faceIndex) const
    {
        const auto* faceData = model.faceAt(faceIndex);

        if (faceData == nullptr)
        {
            m_logger->warn(QString("Face[%1]: 詳細情報を取得できません。 ").arg(faceIndex));
            return;
        }

        const auto& graph = model.graph();
        const auto& wireIndices = graph.wiresOfFace(faceIndex);
        const auto& adjacentFaceIndices = TopologyQuery::adjacentFacesOfFace(model, faceIndex);

        m_logger->info(
            QString("Face[%1]: 種別=%2, 面積=%3, Wire数=%4, Wires=[%5], 隣接Face=[%6]")
                .arg(faceIndex)
                .arg(surfaceKindDisplayName(faceData->info.kind))
                .arg(faceData->info.area, 0, 'f', 3)
                .arg(wireIndices.size())
                .arg(LF::formatWireIndexList(model, wireIndices))
                .arg(LF::formatFaceIndexList(model, adjacentFaceIndices)));

        logFaceSurfaceDetails(faceData->info);
    }

    void GeometryLogReporter::logFaceSurfaceDetails(const FaceInfo& info) const
    {
        switch (info.kind)
        {
        case SurfaceKind::Plane:
            if (info.plane.has_value())
            {
                logPlaneDetails(info.plane.value());
            }
            break;

        case SurfaceKind::Cylinder:
            if (info.cylinder.has_value())
            {
                logCylinderDetails(info.cylinder.value());
            }
            break;

        case SurfaceKind::Cone:
            if (info.cone.has_value())
            {
                logConeDetails(info.cone.value());
            }
            break;

        case SurfaceKind::Sphere:
            if (info.sphere.has_value())
            {
                logSphereDetails(info.sphere.value());
            }
            break;

        case SurfaceKind::Torus:
            if (info.torus.has_value())
            {
                logTorusDetails(info.torus.value());
            }
            break;

        case SurfaceKind::Unknown:
        case SurfaceKind::BezierSurface:
        case SurfaceKind::BSplineSurface:
        case SurfaceKind::Other:
        default:
            break;
        }
    }

    void GeometryLogReporter::logPlaneDetails(const PlaneInfo& info) const
    {
        m_logger->info(QString(" Plane: Origin=%1, Normal=%2")
                        .arg(LF::formatPoint(info.origin))
                        .arg(LF::formatDirection(info.normal)));
    }

    void GeometryLogReporter::logCylinderDetails(const CylinderInfo& info) const
    {
        m_logger->info(QString(" Cylinder: Radius=%1, AxisOrigin=%2, AxisDir=%3")
                        .arg(info.radius, 0, 'f', 3)
                        .arg(LF::formatPoint(info.axis.Location()))
                        .arg(LF::formatDirection(info.axis.Direction())));
    }

    void GeometryLogReporter::logConeDetails(const ConeInfo& info) const
    {
        m_logger->info(QString(" Cone: SemiAngle=%1, RefRadius=%2, AxisOrigin=%3, AxisDir=%4")
                        .arg(info.semiAngle, 0, 'f', 6)
                        .arg(info.refRadius, 0, 'f', 3)
                        .arg(LF::formatPoint(info.axis.Location()))
                        .arg(LF::formatDirection(info.axis.Direction())));
    }

    void GeometryLogReporter::logSphereDetails(const SphereInfo& info) const
    {
        m_logger->info(
            QString("  Sphere: Center=%1, Radius=%2")
                .arg(LF::formatPoint(info.center))
                .arg(info.radius, 0, 'f', 3));
    }

    void GeometryLogReporter::logTorusDetails(const TorusInfo& info) const
    {
        m_logger->info(
            QString("  Torus: MajorRadius=%1, MinorRadius=%2, AxisOrigin=%3, AxisDir=%4")
                .arg(info.majorRadius, 0, 'f', 3)
                .arg(info.minorRadius, 0, 'f', 3)
                .arg(LF::formatPoint(info.axis.Location()))
                .arg(LF::formatDirection(info.axis.Direction())));
    }

    void GeometryLogReporter::logWireDetails(const GeometryModel& model, int wireIndex) const
    {
        const auto* wireData = model.wireAt(wireIndex);

        if (wireData == nullptr)
        {
            m_logger->warn(QString("Wire[%1]: 詳細情報を取得できません。").arg(wireIndex));
            return;
        }

        const auto& graph = model.graph();
        const auto& faceIndices = graph.facesOfWire(wireIndex);
        const auto& edgeIndices = graph.edgesOfWire(wireIndex);

        m_logger->info(
            QString("  Wire[%1]: Outer=%2, Inner=%3, Closed=%4, Faces=[%5], Edges=[%6]")
                .arg(wireIndex)
                .arg(wireData->info.isOuter ? "true" : "false")
                .arg(wireData->info.isInner ? "true" : "false")
                .arg(wireData->info.isClosed ? "true" : "false")
                .arg(LF::formatFaceIndexList(model, faceIndices))
                .arg(LF::formatEdgeIndexList(model, edgeIndices)));
    }

    void GeometryLogReporter::logEdgeDetails(const GeometryModel& model, int edgeIndex) const
    {
        const auto* edgeData = model.edgeAt(edgeIndex);

        if (edgeData == nullptr)
        {
            m_logger->warn(QString("Edge[%1]: 詳細情報を取得できません。").arg(edgeIndex));
        }

        const auto& graph = model.graph();
        const auto& faceIndices = TopologyQuery::facesOfEdge(model, edgeIndex);

        m_logger->info(
            QString("    Edge[%1]: 種別=%2, 長さ=%3, Faces=[%4], Wires=[%5], Vertices=[%6], Param=(%7, %8)")
                .arg(edgeIndex)
                .arg(curveKindDisplayName(edgeData->info.kind))
                .arg(edgeData->info.length, 0, 'f', 3)
                .arg(LF::formatFaceIndexList(model, faceIndices))
                .arg(LF::formatWireIndexList(model, graph.wiresOfEdge(edgeIndex)))
                .arg(LF::formatIndexList(graph.verticesOfEdge(edgeIndex)))
                .arg(edgeData->info.firstParameter, 0, 'f', 3)
                .arg(edgeData->info.lastParameter, 0, 'f', 3));

        logEdgeCurveDetails(edgeData->info);
    }

    void GeometryLogReporter::logEdgeCurveDetails(const EdgeInfo& info) const
    {
        switch (info.kind)
        {
        case CurveKind::Line:
            if (info.line.has_value())
            {
                logLineDetails(info.line.value());
            }
            break;

        case CurveKind::Circle:
            if (info.circle.has_value())
            {
                logCircleDetails(info.circle.value());
            }
            break;

        case CurveKind::Ellipse:
            if (info.ellipse.has_value())
            {
                logEllipseDetails(info.ellipse.value());
            }
            break;

        case CurveKind::Unknown:
        case CurveKind::Hyperbola:
        case CurveKind::Parabola:
        case CurveKind::BezierCurve:
        case CurveKind::BSplineCurve:
        case CurveKind::Other:
        default:
            break;
        }
    }

    void GeometryLogReporter::logLineDetails(
        const LineInfo& info) const
    {
        m_logger->info(
            QString("      Line: Origin=%1, Dir=%2")
                .arg(LF::formatPoint(info.origin))
                .arg(LF::formatDirection(info.direction)));
    }

    void GeometryLogReporter::logCircleDetails(
        const CircleInfo& info) const
    {
        m_logger->info(
            QString("      Circle: Radius=%1, Center=%2, AxisDir=%3")
                .arg(info.radius, 0, 'f', 3)
                .arg(LF::formatPoint(info.center))
                .arg(LF::formatDirection(info.axis.Direction())));
    }

    void GeometryLogReporter::logEllipseDetails(
        const EllipseInfo& info) const
    {
        m_logger->info(
            QString("      Ellipse: MajorR=%1, MinorR=%2, Center=%3, AxisDir=%4")
                .arg(info.majorRadius, 0, 'f', 3)
                .arg(info.minorRadius, 0, 'f', 3)
                .arg(LF::formatPoint(info.center))
                .arg(LF::formatDirection(info.axis.Direction())));
    }

    void GeometryLogReporter::logVertexDetails(
        const GeometryModel& model,
        int vertexIndex) const
    {
        const auto* vertexData = model.vertexAt(vertexIndex);

        if (vertexData == nullptr)
        {
            m_logger->warn(QString("Vertex[%1]: 詳細情報を取得できません。").arg(vertexIndex));
            return;
        }

        const auto& graph = model.graph();

        if (vertexData->info.hasPoint)
        {
            m_logger->info(
                QString("Vertex[%1]: Point=%2, Edges=[%3]")
                    .arg(vertexIndex)
                    .arg(LF::formatPoint(vertexData->info.point))
                    .arg(LF::formatIndexList(graph.edgesOfVertex(vertexIndex))));
        }
        else
        {
            m_logger->info(
                QString("Vertex[%1]: Point=なし, Edges=[%2]")
                    .arg(vertexIndex)
                    .arg(LF::formatIndexList(graph.edgesOfVertex(vertexIndex))));
        }
    }

    void GeometryLogReporter::logFaceTreeDetails(
        const GeometryModel& model,
        int faceIndex) const
    {
        logFaceDetails(model, faceIndex);

        const auto& graph = model.graph();
        const auto wireIndices = graph.wiresOfFace(faceIndex);

        for (int wireIndex : wireIndices)
        {
            logWireTreeDetails(model, wireIndex);
        }
    }

    void GeometryLogReporter::logWireTreeDetails(
        const GeometryModel& model,
        int wireIndex) const
    {
        logWireDetails(model, wireIndex);

        const auto& graph = model.graph();
        const auto edgeIndices = graph.edgesOfWire(wireIndex);

        for (int edgeIndex : edgeIndices)
        {
            logEdgeDetails(model, edgeIndex);
        }
    }
}
