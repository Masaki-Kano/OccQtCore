#include <BRepAdaptor_Surface.hxx>
#include <GeomAbs_SurfaceType.hxx>

#include "Log/AppLogReporter.h"
#include "Log/AppLogger.h"

#include "Geometry/GeometryModel.h"
#include "Geometry/TopologyQuery.h"

#include "Feature/FeatureTypes.h"

namespace OccQtCore
{
    namespace
    {
        QString pickedShapeTypeToJapanese(PickedShapeType type)
        {
            switch (type)
            {
            case OccQtCore::PickedShapeType::Vertex:
                return "頂点";

            case OccQtCore::PickedShapeType::Edge:
                return "エッジ";

            case OccQtCore::PickedShapeType::Face:
                return "面";

            case OccQtCore::PickedShapeType::Solid:
                return "ソリッド";

            case OccQtCore::PickedShapeType::Unknown:
            default:
                return "不明";
            }
        }

        QString surfaceTypeToString(GeomAbs_SurfaceType type)
        {
            switch (type)
            {
            case GeomAbs_Plane:
                return "Plane";
            case GeomAbs_Cylinder:
                return "Cylinder";
            case GeomAbs_Cone:
                return "Cone";
            case GeomAbs_Sphere:
                return "Sphere";
            case GeomAbs_Torus:
                return "Torus";
            case GeomAbs_BezierSurface:
                return "Bezier";
            case GeomAbs_BSplineSurface:
                return "BSpline";
            case GeomAbs_SurfaceOfRevolution:
                return "Revolution";
            case GeomAbs_SurfaceOfExtrusion:
                return "Extrusion";
            case GeomAbs_OffsetSurface:
                return "Offset";
            case GeomAbs_OtherSurface:
            default:
                return "Other";
            }
        }

        QString holeEndCandidateTypeToString(
            OccQtCore::Feature::HoleEndCandidateType type)
        {
            using OccQtCore::Feature::HoleEndCandidateType;

            switch (type)
            {
            case HoleEndCandidateType::Open:
                return "Open";

            case HoleEndCandidateType::Bottom:
                return "Bottom";

            case HoleEndCandidateType::WallConnection:
                return "WallConnection";

            case HoleEndCandidateType::Unknown:
            default:
                return "Unknown";
            }
        }
    }

    AppLogReporter::AppLogReporter(AppLogger* logger)
        : m_logger(logger)
    {
    }

    void AppLogReporter::logSelection(const SelectionInfo& selectionInfo) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

        if (!selectionInfo.isValid)
        {
            m_logger->info("選択: なし");
            return;
        }

        m_logger->info(
            QString("選択: %1, インデックス=%2, 表示オブジェクトID=%3")
                .arg(pickedShapeTypeToJapanese(selectionInfo.type))
                .arg(selectionInfo.elementIndex)
                .arg(selectionInfo.sourceDisplayObjectId));

        if (selectionInfo.elementIndex < 0)
        {
            m_logger->warn("選択形状のインデックスが取得できないため、ジオメトリ詳細ログを出力しません。");
        }
    }

    void AppLogReporter::logStepLoaded(const QString& filePath) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

        m_logger->info(QString("STEPファイルを読み込みました: %1").arg(filePath));
    }

    void AppLogReporter::logStepLoadFailed(const QString& errorMessage) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

        m_logger->error(QString("STEPファイルの読み込みに失敗しました: %1")
                            .arg(errorMessage));
    }

    void AppLogReporter::logActionStarted(const QString& actionName) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

        m_logger->info(QString("操作開始: %1").arg(actionName));
    }

    void AppLogReporter::logActionFinished(const QString& actionName) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

        m_logger->info(QString("操作完了: %1").arg(actionName));
    }

    void AppLogReporter::logActionFailed(
        const QString& actionName,
        const QString& reason) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

        m_logger->error(
            QString("操作失敗: %1, 理由=%2")
            .arg(actionName)
                .arg(reason));
    }

    void AppLogReporter::logGeometryAnalysisReport(const GeometryModel& model) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

        m_logger->info("===== 形状解析ログ開始 =====");

        logGeometrySummary(model);
        logGeometryTopologySummary(model);
        logComplexGeometrySummary(model);
        logCircleGroupSummary(model);

        m_logger->info("===== 形状解析ログ終了 =====");
    }

    void AppLogReporter::logGeometryDetailDiagnostics(const GeometryModel& model) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

        m_logger->info("===== 形状詳細診断ログ開始 =====");

        m_logger->info("----- Face階層詳細 -----");
        for (int faceIndex = 0; faceIndex < model.faceCount(); ++faceIndex)
        {
            logFaceTreeDetails(model, faceIndex);
        }

        m_logger->info("===== 形状詳細診断ログ終了 =====");
    }

    void AppLogReporter::logGeometrySummary(const GeometryModel& model) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

        m_logger->info("----- 形状概要 -----");

        m_logger->info(
            QString("要素数: Face=%1, Wire=%2, Edge=%3, Vertex=%4")
                .arg(model.faceCount())
                .arg(model.wireCount())
                .arg(model.edgeCount())
                .arg(model.vertexCount()));
    }

    void AppLogReporter::logGeometryTopologySummary(const GeometryModel& model) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

        m_logger->info("----- トポロジー概要 -----");

        // 次で実装する。
    }

    void AppLogReporter::logComplexGeometrySummary(const GeometryModel& model) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

        m_logger->info("----- 複雑形状サマリ -----");

        // 次で実装する。
    }

    void AppLogReporter::logCircleGroupSummary(const GeometryModel& model) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

        m_logger->info("----- CircleGroupサマリ -----");

        // 次で実装する。
    }

    void AppLogReporter::logSelectionGeometryDetails(
        const GeometryModel& model,
        const SelectionInfo& selectionInfo) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

        if (!selectionInfo.isValid)
        {
            return;
        }

        if (selectionInfo.elementIndex < 0)
        {
            m_logger->warn("選択形状のインデックスが取得できないため、形状詳細ログを出力しません。");
            return;
        }

        switch (selectionInfo.type)
        {
        case PickedShapeType::Face:
            logFaceDetails(model, selectionInfo.elementIndex);
            break;

        case PickedShapeType::Edge:
            logEdgeDetails(model, selectionInfo.elementIndex);
            break;

        case PickedShapeType::Vertex:
            logVertexDetails(model, selectionInfo.elementIndex);
            break;

        case PickedShapeType::Solid:
        case PickedShapeType::Unknown:
        default:
            break;
        }
    }

    void AppLogReporter::logFaceDetails(const GeometryModel& model, int faceIndex) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

        const auto* faceData = model.faceAt(faceIndex);

        if (faceData == nullptr)
        {
            m_logger->warn(QString("Face[%1]: 詳細情報を取得できません。").arg(faceIndex));
            return;
        }

        const auto& graph = model.graph();
        const auto wireIndices = graph.wiresOfFace(faceIndex);
        const auto adjacentFaceIndices =
            TopologyQuery::adjacentFacesOfFace(model, faceIndex);

        m_logger->info(
            QString("Face[%1]: 種別=%2, 面積=%3, Wire数=%4, Wires=[%5], 隣接Face=[%6]")
                .arg(faceIndex)
                .arg(surfaceKindDisplayName(faceData->info.kind))
                .arg(faceData->info.area, 0, 'f', 3)
                .arg(wireIndices.size())
                .arg(formatWireIndexList(model, wireIndices))
                .arg(formatFaceIndexList(model, adjacentFaceIndices)));

        if (faceData->info.plane.has_value())
        {
            const auto& plane = faceData->info.plane.value();

            m_logger->info(
                QString("  Plane: Origin=(%1, %2, %3), Normal=(%4, %5, %6)")
                    .arg(plane.origin.X(), 0, 'f', 3)
                    .arg(plane.origin.Y(), 0, 'f', 3)
                    .arg(plane.origin.Z(), 0, 'f', 3)
                    .arg(plane.normal.X(), 0, 'f', 3)
                    .arg(plane.normal.Y(), 0, 'f', 3)
                    .arg(plane.normal.Z(), 0, 'f', 3));
        }

        if (faceData->info.cylinder.has_value())
        {
            const auto& cylinder = faceData->info.cylinder.value();

            m_logger->info(
                QString("  Cylinder: Radius=%1, AxisOrigin=(%2, %3, %4), AxisDir=(%5, %6, %7)")
                    .arg(cylinder.radius, 0, 'f', 3)
                    .arg(cylinder.axis.Location().X(), 0, 'f', 3)
                    .arg(cylinder.axis.Location().Y(), 0, 'f', 3)
                    .arg(cylinder.axis.Location().Z(), 0, 'f', 3)
                    .arg(cylinder.axis.Direction().X(), 0, 'f', 3)
                    .arg(cylinder.axis.Direction().Y(), 0, 'f', 3)
                    .arg(cylinder.axis.Direction().Z(), 0, 'f', 3));
        }
    }

    void AppLogReporter::logWireDetails(const GeometryModel& model, int wireIndex) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

        const auto* wireData = model.wireAt(wireIndex);

        if (wireData == nullptr)
        {
            m_logger->warn(QString("Wire[%1]: 詳細情報を取得できません。").arg(wireIndex));
            return;
        }

        const auto& graph = model.graph();
        const auto faceIndices = graph.facesOfWire(wireIndex);
        const auto edgeIndices = graph.edgesOfWire(wireIndex);

        m_logger->info(
            QString("  Wire[%1]: Outer=%2, Inner=%3, Closed=%4, Faces=[%5], Edges=[%6]")
                .arg(wireIndex)
                .arg(wireData->info.isOuter ? "true" : "false")
                .arg(wireData->info.isInner ? "true" : "false")
                .arg(wireData->info.isClosed ? "true" : "false")
                .arg(formatFaceIndexList(model, faceIndices))
                .arg(formatEdgeIndexList(model, edgeIndices)));
    }

    void AppLogReporter::logEdgeDetails(const GeometryModel& model, int edgeIndex) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

        const auto* edgeData = model.edgeAt(edgeIndex);

        if (edgeData == nullptr)
        {
            m_logger->warn(QString("Edge[%1]: 詳細情報を取得できません。").arg(edgeIndex));
            return;
        }

        const auto& graph = model.graph();
        const auto faceIndices =
            TopologyQuery::facesOfEdge(model, edgeIndex);

        m_logger->info(
            QString("    Edge[%1]: 種別=%2, 長さ=%3, Faces=[%4], Wires=[%5], Vertices=[%6], Param=(%7, %8)")
                .arg(edgeIndex)
                .arg(curveKindDisplayName(edgeData->info.kind))
                .arg(edgeData->info.length, 0, 'f', 3)
                .arg(formatFaceIndexList(model, faceIndices))
                .arg(formatWireIndexList(model, graph.wiresOfEdge(edgeIndex)))
                .arg(formatIndexList(graph.verticesOfEdge(edgeIndex)))
                .arg(edgeData->info.firstParameter, 0, 'f', 3)
                .arg(edgeData->info.lastParameter, 0, 'f', 3));

        if (edgeData->info.line.has_value())
        {
            const auto& line = edgeData->info.line.value();

            m_logger->info(
                QString("      Line: Origin=(%1, %2, %3), Dir=(%4, %5, %6)")
                    .arg(line.origin.X(), 0, 'f', 3)
                    .arg(line.origin.Y(), 0, 'f', 3)
                    .arg(line.origin.Z(), 0, 'f', 3)
                    .arg(line.direction.X(), 0, 'f', 3)
                    .arg(line.direction.Y(), 0, 'f', 3)
                    .arg(line.direction.Z(), 0, 'f', 3));
        }

        if (edgeData->info.circle.has_value())
        {
            const auto& circle = edgeData->info.circle.value();

            m_logger->info(
                QString("      Circle: Radius=%1, Center=(%2, %3, %4), AxisDir=(%5, %6, %7)")
                    .arg(circle.radius, 0, 'f', 3)
                    .arg(circle.center.X(), 0, 'f', 3)
                    .arg(circle.center.Y(), 0, 'f', 3)
                    .arg(circle.center.Z(), 0, 'f', 3)
                    .arg(circle.axis.Direction().X(), 0, 'f', 3)
                    .arg(circle.axis.Direction().Y(), 0, 'f', 3)
                    .arg(circle.axis.Direction().Z(), 0, 'f', 3));
        }

        if (edgeData->info.ellipse.has_value())
        {
            const auto& ellipse = edgeData->info.ellipse.value();

            m_logger->info(
                QString("      Ellipse: MajorR=%1, MinorR=%2, Center=(%3, %4, %5), AxisDir=(%6, %7, %8)")
                    .arg(ellipse.majorRadius, 0, 'f', 3)
                    .arg(ellipse.minorRadius, 0, 'f', 3)
                    .arg(ellipse.center.X(), 0, 'f', 3)
                    .arg(ellipse.center.Y(), 0, 'f', 3)
                    .arg(ellipse.center.Z(), 0, 'f', 3)
                    .arg(ellipse.axis.Direction().X(), 0, 'f', 3)
                    .arg(ellipse.axis.Direction().Y(), 0, 'f', 3)
                    .arg(ellipse.axis.Direction().Z(), 0, 'f', 3));
        }
    }

    void AppLogReporter::logVertexDetails(const GeometryModel& model, int vertexIndex) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

        const auto* vertexData = model.vertexAt(vertexIndex);

        if (vertexData == nullptr)
        {
            m_logger->warn(QString("Vertex[%1]: 詳細情報を取得できません。").arg(vertexIndex));
            return;
        }

        const auto& graph = model.graph();

        if (vertexData->info.hasPoint)
        {
            const auto& p = vertexData->info.point;

            m_logger->info(
                QString("Vertex[%1]: Point=(%2, %3, %4), Edges=[%5]")
                    .arg(vertexIndex)
                    .arg(p.X(), 0, 'f', 3)
                    .arg(p.Y(), 0, 'f', 3)
                    .arg(p.Z(), 0, 'f', 3)
                    .arg(formatIndexList(graph.edgesOfVertex(vertexIndex))));
        }
        else
        {
            m_logger->info(
                QString("Vertex[%1]: Point=なし, Edges=[%2]")
                    .arg(vertexIndex)
                    .arg(formatIndexList(graph.edgesOfVertex(vertexIndex))));
        }
    }

    void AppLogReporter::logFaceTreeDetails(const GeometryModel& model, int faceIndex) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

        logFaceDetails(model, faceIndex);

        const auto& graph = model.graph();
        const auto wireIndices = graph.wiresOfFace(faceIndex);

        for (int wireIndex : wireIndices)
        {
            logWireTreeDetails(model, wireIndex);
        }
    }

    void AppLogReporter::logWireTreeDetails(const GeometryModel& model, int wireIndex) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

        logWireDetails(model, wireIndex);

        const auto& graph = model.graph();
        const auto edgeIndices = graph.edgesOfWire(wireIndex);

        for (int edgeIndex : edgeIndices)
        {
            logEdgeDetails(model, edgeIndex);
        }
    }

    void AppLogReporter::logHoleRecognitionReport(
        const GeometryModel& model,
        const std::vector<Feature::HoleWallCandidate>& wallCandidates,
        const std::vector<Feature::HoleEndCandidate>& endCandidates,
        const std::vector<Feature::HoleSegmentCandidate>& segmentCandidates) const
    {
        Q_UNUSED(model);

        if (m_logger == nullptr)
        {
            return;
        }

        m_logger->info("========== 穴フィーチャ認識レポート ==========");
        m_logger->info(QString("穴壁候補数: %1").arg(wallCandidates.size()));
        m_logger->info(QString("穴端候補数: %1").arg(endCandidates.size()));
        m_logger->info(QString("穴セグメント候補数: %1").arg(segmentCandidates.size()));
        m_logger->info("============================================");
    }

    void AppLogReporter::logHoleWallCandidates(
        const GeometryModel& model,
        const std::vector<Feature::HoleWallCandidate>& candidates) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

        m_logger->info("========== 穴壁候補 ==========");
        m_logger->info(QString("候補数: %1").arg(candidates.size()));

        for (const auto& candidate : candidates)
        {
            m_logger->info(
                QString("WallCandidate[%1]: Faces=%2, Center=(%3, %4, %5), Axis=(%6, %7, %8), Radius=%9, Depth=%10")
                    .arg(candidate.index)
                    .arg(formatFaceIndexList(model, candidate.geometryRefs.faceIndices))
                    .arg(candidate.center.X(), 0, 'f', 3)
                    .arg(candidate.center.Y(), 0, 'f', 3)
                    .arg(candidate.center.Z(), 0, 'f', 3)
                    .arg(candidate.axisDirection.X(), 0, 'f', 3)
                    .arg(candidate.axisDirection.Y(), 0, 'f', 3)
                    .arg(candidate.axisDirection.Z(), 0, 'f', 3)
                    .arg(candidate.radius, 0, 'f', 3)
                    .arg(candidate.depth, 0, 'f', 3));
        }
    }

    void AppLogReporter::logHoleEndCandidates(
        const GeometryModel& model,
        const std::vector<Feature::HoleEndCandidate>& candidates) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

        m_logger->info("========== 穴端候補 ==========");
        m_logger->info(QString("候補数: %1").arg(candidates.size()));

        for (const auto& candidate : candidates)
        {
            m_logger->info(
                QString("EndCandidate[%1]: Type=%2, Wall=%3, Faces=%4, Edges=%5, Center=(%6, %7, %8), Axis=(%9, %10, %11), Radius=%12")
                    .arg(candidate.index)
                    .arg(holeEndCandidateTypeToString(candidate.type))
                    .arg(candidate.wallCandidateIndex)
                    .arg(formatFaceIndexList(model, candidate.geometryRefs.faceIndices))
                    .arg(formatEdgeIndexList(model, candidate.geometryRefs.edgeIndices))
                    .arg(candidate.center.X(), 0, 'f', 3)
                    .arg(candidate.center.Y(), 0, 'f', 3)
                    .arg(candidate.center.Z(), 0, 'f', 3)
                    .arg(candidate.axisDirection.X(), 0, 'f', 3)
                    .arg(candidate.axisDirection.Y(), 0, 'f', 3)
                    .arg(candidate.axisDirection.Z(), 0, 'f', 3)
                    .arg(candidate.radius, 0, 'f', 3));
        }
    }

    void AppLogReporter::logHoleSegmentCandidates(
        const GeometryModel& model,
        const std::vector<Feature::HoleSegmentCandidate>& segments,
        const std::vector<Feature::HoleWallCandidate>& wallCandidates,
        const std::vector<Feature::HoleEndCandidate>& endCandidates) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

        m_logger->info("========== 穴セグメント候補 ==========");
        m_logger->info(QString("候補数: %1").arg(segments.size()));

        for (const auto& segment : segments)
        {
            QString message;

            message += QString("SegmentCandidate[%1]: ")
                           .arg(segment.index);

            message += QString("Wall=%1")
                           .arg(segment.wallCandidateIndex);

            if (segment.wallCandidateIndex >= 0 &&
                segment.wallCandidateIndex < static_cast<int>(wallCandidates.size()))
            {
                const auto& wall = wallCandidates[segment.wallCandidateIndex];

                message += QString(", WallFaces=%1")
                               .arg(formatFaceIndexList(
                                   model,
                                   wall.geometryRefs.faceIndices));

                message += QString(", Center=(%1, %2, %3)")
                               .arg(wall.center.X(), 0, 'f', 3)
                               .arg(wall.center.Y(), 0, 'f', 3)
                               .arg(wall.center.Z(), 0, 'f', 3);

                message += QString(", Axis=(%1, %2, %3)")
                               .arg(wall.axisDirection.X(), 0, 'f', 3)
                               .arg(wall.axisDirection.Y(), 0, 'f', 3)
                               .arg(wall.axisDirection.Z(), 0, 'f', 3);

                message += QString(", Radius=%1")
                               .arg(wall.radius, 0, 'f', 3);
            }

            message += QString(", EndIndices=%1")
                           .arg(formatIndexList(segment.endCandidateIndices));

            m_logger->info(message);

            for (int endCandidateIndex : segment.endCandidateIndices)
            {
                if (endCandidateIndex < 0 ||
                    endCandidateIndex >= static_cast<int>(endCandidates.size()))
                {
                    m_logger->info(
                        QString("  End[%1]: <invalid>")
                            .arg(endCandidateIndex));
                    continue;
                }

                const auto& end = endCandidates[endCandidateIndex];

                QString endMessage;

                endMessage += QString("  End[%1]: Type=%2")
                                  .arg(end.index)
                                  .arg(holeEndCandidateTypeToString(end.type));

                endMessage += ", Axial=";

                if (end.hasAxialPosition)
                {
                    endMessage += QString::number(
                        end.axialPosition,
                        'f',
                        4);
                }
                else
                {
                    endMessage += "N/A";
                }

                endMessage += QString(", Faces=%1")
                                  .arg(formatFaceIndexList(
                                      model,
                                      end.geometryRefs.faceIndices));

                endMessage += QString(", Edges=%1")
                                  .arg(formatEdgeIndexList(
                                      model,
                                      end.geometryRefs.edgeIndices));

                m_logger->info(endMessage);
            }
        }
    }

    QString AppLogReporter::formatIndexList(
        const std::vector<int>& indices) const
    {
        QString text;

        for (int index : indices)
        {
            if (!text.isEmpty())
            {
                text += ", ";
            }

            text += QString::number(index);
        }

        return text;
    }

    QString AppLogReporter::formatFaceIndexList(
        const GeometryModel& model,
        const std::vector<int>& faceIndices) const
    {
        QString text;

        for (int faceIndex : faceIndices)
        {
            if (!text.isEmpty())
            {
                text += ", ";
            }

            QString kindText = "Invalid";

            const auto* faceData = model.faceAt(faceIndex);
            if (faceData != nullptr)
            {
                kindText = surfaceKindDisplayName(faceData->info.kind);
            }

            text += QString("%1:%2")
                        .arg(faceIndex)
                        .arg(kindText);
        }

        return text;
    }

    QString AppLogReporter::formatEdgeIndexList(
        const GeometryModel& model,
        const std::vector<int>& edgeIndices) const
    {
        QString text;

        for (int edgeIndex : edgeIndices)
        {
            if (!text.isEmpty())
            {
                text += ", ";
            }

            QString kindText = "Invalid";

            const auto* edgeData = model.edgeAt(edgeIndex);
            if (edgeData != nullptr)
            {
                kindText = curveKindDisplayName(edgeData->info.kind);
            }

            text += QString("%1:%2")
                        .arg(edgeIndex)
                        .arg(kindText);
        }

        return text;
    }

    QString AppLogReporter::formatWireIndexList(
        const GeometryModel& model,
        const std::vector<int>& wireIndices) const
    {
        QString text;

        for (int wireIndex : wireIndices)
        {
            if (!text.isEmpty())
            {
                text += ", ";
            }

            QString kindText = "Invalid";

            const auto* wireData = model.wireAt(wireIndex);
            if (wireData != nullptr)
            {
                if (wireData->info.isOuter)
                {
                    kindText = "Outer";
                }
                else if (wireData->info.isInner)
                {
                    kindText = "Inner";
                }
                else
                {
                    kindText = "Unknown";
                }

                if (wireData->info.isClosed)
                {
                    kindText += "/Closed";
                }
                else
                {
                    kindText += "/Open";
                }
            }

            text += QString("%1:%2")
                        .arg(wireIndex)
                        .arg(kindText);
        }

        return text;
    }

    QString AppLogReporter::formatFaceIndex(
        const GeometryModel& model,
        int faceIndex) const
    {
        const auto* faceData = model.faceAt(faceIndex);

        if (faceData == nullptr)
        {
            return QString("%1:Invalid").arg(faceIndex);
        }

        return QString("%1:%2")
            .arg(faceIndex)
            .arg(surfaceKindDisplayName(faceData->info.kind));
    }

    QString AppLogReporter::formatWireIndex(
        const GeometryModel& model,
        int wireIndex) const
    {
        const auto* wireData = model.wireAt(wireIndex);

        if (wireData == nullptr)
        {
            return QString("%1:Invalid").arg(wireIndex);
        }

        QString typeText;

        if (wireData->info.isOuter)
        {
            typeText = "Outer";
        }
        else if (wireData->info.isInner)
        {
            typeText = "Inner";
        }
        else
        {
            typeText = "Unknown";
        }

        typeText += wireData->info.isClosed ? "/Closed" : "/Open";

        return QString("%1:%2")
            .arg(wireIndex)
            .arg(typeText);
    }
}
