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
            m_logger->warn("選択形状のインデックスが取得できないため、接続関係ログを出力しません。");
        }
    }

    void AppLogReporter::logFaceGraph(
        const GeometryModel& model,
        int faceIndex) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

        const auto& graph = model.graph();
        const auto& wireIndices = graph.wiresOfFace(faceIndex);

        m_logger->info(
            QString("面[%1]: 接続ワイヤー数=%2")
                .arg(faceIndex)
                .arg(wireIndices.size()));

        for (int wireIndex : wireIndices)
        {
            const auto* wireData = model.wireAt(wireIndex);
            const auto& edgeIndices = graph.edgesOfWire(wireIndex);
            const auto& faceIndices = graph.facesOfWire(wireIndex);

            const bool isOuter = wireData != nullptr && wireData->info.isOuter;
            const bool isInner = wireData != nullptr && wireData->info.isInner;
            const bool isClosed = wireData != nullptr && wireData->info.isClosed;

            m_logger->info(
                QString("  ワイヤー[%1]: Outer=%2, Inner=%3, Closed=%4, 接続面=[%5], 接続エッジ=[%6]")
                    .arg(wireIndex)
                    .arg(isOuter ? "true" : "false")
                    .arg(isInner ? "true" : "false")
                    .arg(isClosed ? "true" : "false")
                    .arg(formatIndexList(faceIndices))
                    .arg(formatIndexList(edgeIndices)));
        }
    }

    void AppLogReporter::logEdgeGraph(
        const GeometryGraph& graph,
        int edgeIndex) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

        const auto& wireIndices = graph.wiresOfEdge(edgeIndex);
        const auto& vertexIndices = graph.verticesOfEdge(edgeIndex);

        std::vector<int> faceIndices;

        for (int wireIndex : wireIndices)
        {
            const auto& connectedFaceIndices = graph.facesOfWire(wireIndex);

            for (int faceIndex : connectedFaceIndices)
            {
                if (std::find(faceIndices.begin(), faceIndices.end(), faceIndex) == faceIndices.end())
                {
                    faceIndices.push_back(faceIndex);
                }
            }
        }

        m_logger->info(
            QString("エッジ[%1]: 接続ワイヤー=[%2], 接続面=[%3], 接続頂点=[%4]")
                .arg(edgeIndex)
                .arg(formatIndexList(wireIndices))
                .arg(formatIndexList(faceIndices))
                .arg(formatIndexList(vertexIndices)));
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

    void AppLogReporter::logGeometryModelDiagnostics(
        const GeometryModel& model) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

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
