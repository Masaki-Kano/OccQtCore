#ifndef APPLOGREPORTER_H
#define APPLOGREPORTER_H

#include <vector>

#include <QString>

#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

#include "Core/SelectionInfo.h"
#include "Log/HoleRecognitionLogReporter.h"
#include "Feature/HoleRecognitionTypes.h"

namespace OccQtCore
{
    class AppLogger;
    class GeometryModel;

    class AppLogReporter
    {
    public:
        explicit AppLogReporter(AppLogger* logger);

        // ============================================================
        // Operation logs
        // 操作・イベントログ
        // ============================================================
        void logSelection(const SelectionInfo& selectionInfo) const;
        void logStepLoaded(const QString& filePath) const;
        void logStepLoadFailed(const QString& errorMessage) const;

        void logActionStarted(const QString& actionName) const;
        void logActionFinished(const QString& actionName) const;
        void logActionFailed(const QString& actionName, const QString& reason) const;

        // ============================================================
        // Geometry logs
        // 読み込んだ形状データそのもののログ
        // ============================================================
        void logGeometryAnalysisReport(const GeometryModel& model) const;
        void logGeometryDetailDiagnostics(const GeometryModel& model) const;

        void logGeometrySummary(const GeometryModel& model) const;
        void logGeometryTopologySummary(const GeometryModel& model) const;
        void logComplexGeometrySummary(const GeometryModel& model) const;
        void logCircleGroupSummary(const GeometryModel& model) const;

        void logSelectionGeometryDetails(
            const GeometryModel& model,
            const SelectionInfo& selectionInfo) const;

        // 単体詳細：ピック向け
        void logFaceDetails(const GeometryModel& model, int faceIndex) const;
        void logWireDetails(const GeometryModel& model, int wireIndex) const;
        void logEdgeDetails(const GeometryModel& model, int edgeIndex) const;
        void logVertexDetails(const GeometryModel& model, int vertexIndex) const;

        // 階層詳細：診断向け
        void logFaceTreeDetails(const GeometryModel& model, int faceIndex) const;
        void logWireTreeDetails(const GeometryModel& model, int wireIndex) const;

        // ============================================================
        // Feature logs
        // 加工フィーチャとして意味づけした情報のログ
        // ============================================================
        void logHoleRecognition(const HoleRecognitionLogReport& report) const;


    private:
        AppLogger* m_logger = nullptr;

        HoleRecognitionLogReporter m_holeRecognitionReporter;
    };
}

#endif // APPLOGREPORTER_H
