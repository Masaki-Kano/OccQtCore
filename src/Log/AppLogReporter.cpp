#include "Log/AppLogReporter.h"
#include "Log/AppLogger.h"

#include "Geometry/GeometryModel.h"

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

    void AppLogReporter::logStepLoadFailed(const QString& errorMessage) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

        m_logger->error(QString("STEPファイルの読み込みに失敗しました: %1")
                            .arg(errorMessage));
    }

    void AppLogReporter::logStepLoaded(const QString& filePath) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

        m_logger->info(QString("STEPファイルを読み込みました: %1").arg(filePath));
    }

    void AppLogReporter::logGeometryModelDiagnostics(const GeometryModel& model) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

        const auto graph = model.graph();

        m_logger->info(
            QString("形状モデル構築: 面=%1, ワイヤー=%2, エッジ=%3, 頂点=%4")
                .arg(model.faceCount())
                .arg(model.wireCount())
                .arg(model.edgeCount())
                .arg(model.vertexCount()));

        m_logger->info(
            QString("接続グラフ構築: 面=%1, ワイヤー=%2, エッジ=%3, 頂点=%4")
                .arg(model.graph().faceCount())
                .arg(model.graph().wireCount())
                .arg(model.graph().edgeCount())
                .arg(model.graph().vertexCount()));

        int zeroWireFaceCount = 0;
        int zeroFaceWireCount = 0;
        int zeroEdgeWireCount = 0;
        int zeroWireEdgeCount = 0;

        for (int faceIndex = 0; faceIndex < model.faceCount(); ++faceIndex)
        {
            if (graph.wiresOfFace(faceIndex).empty())
            {
                ++zeroWireFaceCount;
            }
        }

        for (int wireIndex = 0; wireIndex < model.wireCount(); ++wireIndex)
        {
            if (graph.facesOfWire(wireIndex).empty())
            {
                ++zeroFaceWireCount;
            }

            if (graph.edgesOfWire(wireIndex).empty())
            {
                ++zeroEdgeWireCount;
            }
        }

        m_logger->info(
            QString("接続グラフ確認: 接続ワイヤーなし面=%1, 接続面なしワイヤー=%2, 接続エッジなしワイヤー=%3, 接続ワイヤーなしエッジ=%4, 接続頂点数が2以外のエッジ=%5")
                .arg(zeroWireFaceCount)
                .arg(zeroFaceWireCount)
                .arg(zeroEdgeWireCount)
                .arg(zeroWireEdgeCount));
    }

    void AppLogReporter::logPickedFaceDetails(const GeometryModel& model, int faceIndex) const
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
        const auto& wireIndices = graph.wiresOfFace(faceIndex);

        m_logger->info(
            QString("Face[%1]: 種別=%2, 面積=%3, Wire数=%4, Wires=[%5]")
                .arg(faceIndex)
                .arg(surfaceKindDisplayName(faceData->info.kind))
                .arg(faceData->info.area, 0, 'f', 3)
                .arg(wireIndices.size())
                .arg(formatIndexList(wireIndices)));

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

        for (int wireIndex : wireIndices)
        {
            const auto* wireData = model.wireAt(wireIndex);
            const auto& faceIndices = graph.facesOfWire(wireIndex);
            const auto& edgeIndices = graph.edgesOfWire(wireIndex);

            const bool isOuter = wireData != nullptr && wireData->info.isOuter;
            const bool isInner = wireData != nullptr && wireData->info.isInner;
            const bool isClosed = wireData != nullptr && wireData->info.isClosed;

            m_logger->info(
                QString("  Wire[%1]: Outer=%2, Inner=%3, Closed=%4, Faces=[%5], Edges=[%6]")
                    .arg(wireIndex)
                    .arg(isOuter ? "true" : "false")
                    .arg(isInner ? "true" : "false")
                    .arg(isClosed ? "true" : "false")
                    .arg(formatIndexList(faceIndices))
                    .arg(formatIndexList(edgeIndices)));

            for (int edgeIndex : edgeIndices)
            {
                const auto* edgeData = model.edgeAt(edgeIndex);

                if (edgeData == nullptr)
                {
                    continue;
                }

                m_logger->info(
                    QString("    Edge[%1]: 種別=%2, 長さ=%3, Vertices=[%4]")
                        .arg(edgeIndex)
                        .arg(curveKindDisplayName(edgeData->info.kind))
                        .arg(edgeData->info.length, 0, 'f', 3)
                        .arg(formatIndexList(graph.verticesOfEdge(edgeIndex))));

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
            }
        }

        const auto adjacentFaceIndices = graph.adjacentFacesOfFace(faceIndex);

        m_logger->info(
            QString("Face[%1]: 隣接Face=[%2]")
                .arg(faceIndex)
                .arg(formatIndexList(adjacentFaceIndices)));
    }

    void AppLogReporter::logPickedEdgeDetails(const GeometryModel& model, int edgeIndex) const
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

        m_logger->info(
            QString("Edge[%1]: 種別=%2, 長さ=%3, Wires=[%4], Vertices=[%5], Param=(%6, %7)")
                .arg(edgeIndex)
                .arg(curveKindDisplayName(edgeData->info.kind))
                .arg(edgeData->info.length, 0, 'f', 3)
                .arg(formatIndexList(graph.wiresOfEdge(edgeIndex)))
                .arg(formatIndexList(graph.verticesOfEdge(edgeIndex)))
                .arg(edgeData->info.firstParameter, 0, 'f', 3)
                .arg(edgeData->info.lastParameter, 0, 'f', 3));

        if (edgeData->info.line.has_value())
        {
            const auto& line = edgeData->info.line.value();

            m_logger->info(
                QString("  Line: Origin=(%1, %2, %3), Dir=(%4, %5, %6)")
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
                QString("  Circle: Radius=%1, Center=(%2, %3, %4), AxisDir=(%5, %6, %7)")
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
                QString("  Ellipse: MajorR=%1, MinorR=%2, Center=(%3, %4, %5), AxisDir=(%6, %7, %8)")
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

    void AppLogReporter::logPickedVertexDetails(const GeometryModel& model, int vertexIndex) const
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

    void AppLogReporter::logAllGeometryDetails(const GeometryModel& model) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

        m_logger->info("===== 全ジオメトリ詳細ログ開始 =====");

        m_logger->info("----- Face詳細 -----");
        for (int faceIndex = 0; faceIndex < model.faceCount(); ++faceIndex)
        {
            logPickedFaceDetails(model, faceIndex);
        }

        m_logger->info("----- Edge詳細 -----");
        for (int edgeIndex = 0; edgeIndex < model.edgeCount(); ++edgeIndex)
        {
            logPickedEdgeDetails(model, edgeIndex);
        }

        m_logger->info("----- Vertex詳細 -----");
        for (int vertexIndex = 0; vertexIndex < model.vertexCount(); ++vertexIndex)
        {
            logPickedVertexDetails(model, vertexIndex);
        }

        m_logger->info("===== 全ジオメトリ詳細ログ終了 =====");
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
}
