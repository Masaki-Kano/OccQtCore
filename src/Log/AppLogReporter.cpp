#include <BRepAdaptor_Surface.hxx>
#include <GeomAbs_SurfaceType.hxx>

#include "Log/AppLogReporter.h"
#include "Log/AppLogger.h"

#include "Geometry/GeometryModel.h"
#include "Geometry/TopologyQuery.h"

#include "Feature/HoleFeatureRecognizer.h"

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

        QString endTypeToString(OccQtCore::Feature::Hole::EndType type)
        {
            using OccQtCore::Feature::Hole::EndType;

            switch (type)
            {
            case EndType::Open:
                return "Open";
            case EndType::Bottom:
                return "Bottom";
            case EndType::Step:
                return "Step";
            case EndType::Unknown:
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
        const std::vector<Feature::HoleWallComponent>& wallComponents,
        const std::vector<Feature::HoleEndComponent>& endComponents) const
    {
        Q_UNUSED(model);

        if (m_logger == nullptr)
        {
            return;
        }

        m_logger->info("========== 穴フィーチャ認識レポート ==========");
        m_logger->info(QString("穴壁候補数: %1").arg(wallCandidates.size()));
        m_logger->info(QString("穴壁コンポーネント数: %1").arg(wallComponents.size()));
        m_logger->info(QString("穴端コンポーネント数: %1").arg(endComponents.size()));
        m_logger->info("============================================");
    }

    void AppLogReporter::logHoleWallCandidates(
        const GeometryModel& model,
        const std::vector<Feature::HoleWallCandidate>& candidates) const
    {
        Q_UNUSED(model);

        if (m_logger == nullptr)
        {
            return;
        }

        m_logger->info("========== 穴壁候補 ==========");
        m_logger->info(QString("候補数: %1").arg(candidates.size()));

        for (const auto& candidate : candidates)
        {
            m_logger->info(
                QString("WallCandidate[%1]: Face=%2, Center=(%3, %4, %5), Axis=(%6, %7, %8), Radius=%9")
                    .arg(candidate.index)
                    .arg(candidate.faceIndex)
                    .arg(candidate.center.X(), 0, 'f', 3)
                    .arg(candidate.center.Y(), 0, 'f', 3)
                    .arg(candidate.center.Z(), 0, 'f', 3)
                    .arg(candidate.axisDirection.X(), 0, 'f', 3)
                    .arg(candidate.axisDirection.Y(), 0, 'f', 3)
                    .arg(candidate.axisDirection.Z(), 0, 'f', 3)
                    .arg(candidate.radius, 0, 'f', 3));
        }
    }

    void AppLogReporter::logHoleWallComponents(
        const GeometryModel& model,
        const std::vector<Feature::HoleWallComponent>& components) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

        m_logger->info("========== 穴壁コンポーネント ==========");
        m_logger->info(QString("コンポーネント数: %1").arg(components.size()));

        for (const auto& component : components)
        {
            m_logger->info(
                QString("WallComponent[%1]: Faces=%2, Center=(%3, %4, %5), Axis=(%6, %7, %8), Radius=%9, Depth=%10")
                    .arg(component.index)
                    .arg(formatFaceIndexList(model, component.geometryRefs.faceIndices))
                    .arg(component.center.X(), 0, 'f', 3)
                    .arg(component.center.Y(), 0, 'f', 3)
                    .arg(component.center.Z(), 0, 'f', 3)
                    .arg(component.axisDirection.X(), 0, 'f', 3)
                    .arg(component.axisDirection.Y(), 0, 'f', 3)
                    .arg(component.axisDirection.Z(), 0, 'f', 3)
                    .arg(component.radius, 0, 'f', 3)
                    .arg(component.depth, 0, 'f', 3));
        }
    }

    void AppLogReporter::logHoleEndComponents(
        const GeometryModel& model,
        const std::vector<Feature::HoleEndComponent>& components) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

        m_logger->info("========== 穴端コンポーネント ==========");
        m_logger->info(QString("コンポーネント数: %1").arg(components.size()));

        for (const auto& component : components)
        {
            QString endTypeText = "Unknown";

            switch (component.endType)
            {
            case Feature::Hole::EndType::Open:
                endTypeText = "Open";
                break;

            case Feature::Hole::EndType::Bottom:
                endTypeText = "Bottom";
                break;

            case Feature::Hole::EndType::Step:
                endTypeText = "Step";
                break;

            default:
                break;
            }

            m_logger->info(
                QString("EndComponent[%1]: Type=%2, Wall=%3, Faces=%4, Edges=%5, Center=(%6, %7, %8), Axis=(%9, %10, %11), Radius=%12")
                    .arg(component.index)
                    .arg(endTypeText)
                    .arg(component.wallComponentIndex)
                    .arg(formatFaceIndexList(model, component.geometryRefs.faceIndices))
                    .arg(formatEdgeIndexList(model, component.geometryRefs.edgeIndices))
                    .arg(component.center.X(), 0, 'f', 3)
                    .arg(component.center.Y(), 0, 'f', 3)
                    .arg(component.center.Z(), 0, 'f', 3)
                    .arg(component.axisDirection.X(), 0, 'f', 3)
                    .arg(component.axisDirection.Y(), 0, 'f', 3)
                    .arg(component.axisDirection.Z(), 0, 'f', 3)
                    .arg(component.radius, 0, 'f', 3));
        }
    }

    void AppLogReporter::logHoleElements(
        const GeometryModel& model,
        const std::vector<Feature::HoleElement>& elements,
        const std::vector<Feature::HoleEndComponent>& endComponents) const
    {
        Q_UNUSED(model);

        if (m_logger == nullptr)
        {
            return;
        }

        m_logger->info("========== 穴要素 ==========");
        m_logger->info(QString("要素数: %1").arg(elements.size()));

        for (const auto& element : elements)
        {
            QString typeText = "Unknown";

            switch (element.type)
            {
            case Feature::Hole::Type::SimpleBlind:
                typeText = "SimpleBlind";
                break;

            case Feature::Hole::Type::SimpleThrough:
                typeText = "SimpleThrough";
                break;

            case Feature::Hole::Type::SteppedBlind:
                typeText = "SteppedBlind";
                break;

            case Feature::Hole::Type::SteppedThrough:
                typeText = "SteppedThrough";
                break;

            case Feature::Hole::Type::ConterBore:
                typeText = "CounterBore";
                break;

            case Feature::Hole::Type::CounterSink:
                typeText = "CounterSink";
                break;

            default:
                break;
            }

            QStringList endTexts;

            for (int endIndex : element.endComponentIndices)
            {
                QString endTypeText = "Unknown";

                if (endIndex >= 0 && endIndex < static_cast<int>(endComponents.size()))
                {
                    const auto& end = endComponents[endIndex];

                    switch (end.endType)
                    {
                    case Feature::Hole::EndType::Open:
                        endTypeText = "Open";
                        break;

                    case Feature::Hole::EndType::Bottom:
                        endTypeText = "Bottom";
                        break;

                    case Feature::Hole::EndType::Step:
                        endTypeText = "Step";
                        break;

                    default:
                        break;
                    }
                }

                endTexts.push_back(
                    QString("%1:%2").arg(endIndex).arg(endTypeText));
            }

            m_logger->info(
                QString("HoleElement[%1]: Type=%2, Wall=%3, Ends=[%4]")
                    .arg(element.index)
                    .arg(typeText)
                    .arg(element.wallComponentIndex)
                    .arg(endTexts.join(", ")));
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
