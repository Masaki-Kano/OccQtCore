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
        const GeometryGraph& graph,
        int faceIndex) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

        const auto& edgeIndices = graph.edgesOfFace(faceIndex);

        m_logger->info(
            QString("面[%1]: 接続エッジ数=%2")
                .arg(faceIndex)
                .arg(edgeIndices.size()));

        for (int edgeIndex : edgeIndices)
        {
            const auto& connectedFaceIndices = graph.facesOfEdge(edgeIndex);

            std::vector<int> neighborFaceIndices;

            for (int connectedFaceIndex : connectedFaceIndices)
            {
                if (connectedFaceIndex == faceIndex)
                {
                    continue;
                }

                neighborFaceIndices.push_back(connectedFaceIndex);
            }

            m_logger->info(
                QString("  エッジ[%1]: 接続面=[%2], 隣接面=[%3]")
                    .arg(edgeIndex)
                    .arg(formatIndexList(connectedFaceIndices))
                    .arg(formatIndexList(neighborFaceIndices)));
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

        const auto& faceIndices = graph.facesOfEdge(edgeIndex);
        const auto& vertexIndices = graph.verticesOfEdge(edgeIndex);

        m_logger->info(
            QString("エッジ[%1]: 接続面=[%2], 接続頂点=[%3]")
                .arg(edgeIndex)
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
            QString("形状モデル構築: 面=%1, エッジ=%2, 頂点=%3")
                .arg(model.faceCount())
                .arg(model.edgeCount())
                .arg(model.vertexCount()));

        m_logger->info(
            QString("接続グラフ構築: 面=%1, エッジ=%2, 頂点=%3")
                .arg(model.graph().faceCount())
                .arg(model.graph().edgeCount())
                .arg(model.graph().vertexCount()));

        int zeroEdgeFaceCount = 0;
        int nonTwoVertexEdgeCount = 0;

        for (int edgeIndex = 0; edgeIndex < model.edgeCount(); ++edgeIndex)
        {
            if (model.graph().facesOfEdge(edgeIndex).empty())
            {
                ++zeroEdgeFaceCount;
            }

            if (model.graph().verticesOfEdge(edgeIndex).size() != 2)
            {
                ++nonTwoVertexEdgeCount;
            }
        }

        m_logger->info(
            QString("接続グラフ確認: 接続面なしエッジ=%1, 接続頂点数が2以外のエッジ=%2")
                .arg(zeroEdgeFaceCount)
                .arg(nonTwoVertexEdgeCount));
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
