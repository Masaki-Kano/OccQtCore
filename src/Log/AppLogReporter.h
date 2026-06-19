#ifndef APPLOGREPORTER_H
#define APPLOGREPORTER_H

#include <QString>

#include "Interaction/SelectionTypes.h"
#include "Log/HoleRecognitionLogReporter.h"
#include "Log/GeometryLogReporter.h"

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
        void logSelection(const CurrentSelection& selectionInfo) const;
        void logStepLoaded(const QString& filePath) const;
        void logStepLoadFailed(const QString& errorMessage) const;

        void logActionStarted(const QString& actionName) const;
        void logActionFinished(const QString& actionName) const;
        void logActionFailed(const QString& actionName, const QString& reason) const;

        // ============================================================
        // Geometry logs
        // 読み込んだ形状データそのもののログ
        // ============================================================
        void logGeometry(const GeometryLogReport& report) const;
        void logGeometryElement(const GeometryElementLogReport& report) const;

        // ============================================================
        // Feature logs
        // 加工フィーチャとして意味づけした情報のログ
        // ============================================================
        void logHoleRecognition(const HoleRecognitionLogReport& report) const;


    private:
        AppLogger* m_logger = nullptr;

        GeometryLogReporter m_geometryReporter;
        HoleRecognitionLogReporter m_holeRecognitionReporter;
    };
}

#endif // APPLOGREPORTER_H
