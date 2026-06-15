#ifndef HOLERECOGNITIONLOGREPORTER_H
#define HOLERECOGNITIONLOGREPORTER_H

#include <QString>

#include "Feature/HoleRecognitionModel.h"

namespace OccQtCore
{
    class AppLogger;
    class GeometryModel;

    struct HoleRecognitionLogReport
    {
        const GeometryModel& model;
        const Feature::HoleRecognitionResult& result;

        bool outputSummary = true;
        bool outputGeometryGroup = true;
        bool outputPort = true;
    };

    class HoleRecognitionLogReporter
    {
    public:
        explicit HoleRecognitionLogReporter(AppLogger* logger);

        // ログパネル出力用
        void logHoleRecognition(const HoleRecognitionLogReport& report) const;

        // ファイル出力用
        QString formatHoleRecognition(const HoleRecognitionLogReport& report) const;

    private:
        // Apploggerへ流す用
        void logSummary(const HoleRecognitionLogReport& report) const;

        // QStringへ組み立てる用
        void appendSummary(QString& text, const HoleRecognitionLogReport& report) const;
        void appendContextGeometryGroups(QString& text, const HoleRecognitionLogReport& report) const;
        void appendContextTracePorts(QString& text, const HoleRecognitionLogReport& report) const;

        QString formatIntList(const std::vector<int>& values) const;
        QString formatContextGeometryGroup(const Feature::HoleContextGeometryGroup& group, int displayIndex) const;
        QString formatContextTracePort(const Feature::HoleContextTracePort& port, int displayIndex) const;

        QString toString(Feature::HoleContextGeometryGroupKind kind) const;
        QString toString(Feature::HoleContextTracePortKind kind) const;

    private:
        AppLogger* m_logger = nullptr;
    };
}

#endif // HOLERECOGNITIONLOGREPORTER_H
