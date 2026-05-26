#ifndef APPLOGREPORTER_H
#define APPLOGREPORTER_H

#include <vector>

#include <QString>

#include "Core/SelectionInfo.h"

namespace OccQtCore
{
    class AppLogger;
    class GeometryModel;

    class AppLogReporter
    {
    public:
        explicit AppLogReporter(AppLogger* logger);

        // ピックログ
        void logSelection(const SelectionInfo& selectionInfo) const;

        // ファイル読み込みログ
        void logStepLoadFailed(const QString& errorMessage) const;
        void logStepLoaded(const QString& filePath) const;

        // ジオメトリ関連ログ
        void logGeometryModelDiagnostics(const GeometryModel& model) const;
        void logPickedFaceDetails(const GeometryModel& model, int faceIndex) const;
        void logPickedEdgeDetails(const GeometryModel& model, int edgeIndex) const;
        void logPickedVertexDetails(const GeometryModel& model, int vertexIndex) const;
        void logAllGeometryDetails(const GeometryModel& model) const;

        // 穴フィーチャ認識関連ログ(一時デバック用あとでいい感じにする)
        void logHoleEndCandidates(const OccQtCore::GeometryModel& model) const;

    private:
        QString formatIndexList(const std::vector<int>& indices) const;

    private:
        AppLogger* m_logger = nullptr;
    };
}

#endif // APPLOGREPORTER_H
