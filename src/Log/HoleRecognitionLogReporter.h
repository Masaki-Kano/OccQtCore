#ifndef HOLERECOGNITIONLOGREPORTER_H
#define HOLERECOGNITIONLOGREPORTER_H

#include <QString>
#include <QTextStream>

#include "Feature/HoleRecognitionModel.h"
#include "Feature/HoleRecognitionWorkingData.h"

namespace OccQtCore
{
    class AppLogger;
    class GeometryModel;

    struct HoleRecognitionLogReport
    {
        const GeometryModel& model;
        const Feature::HoleRecognitionResult& result;

        const Feature::HoleRecognitionWorkingData* workingData = nullptr;

        bool outputSummary = true;
        bool outputGeometryGroup = true;
        bool outputPort = false;
        bool outputTraceStep = true;
        bool outputTraceSession = true;

        bool outputCylindricalWorkingGroup = true;
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
        void appendContextTraceSteps(QString& text, const HoleRecognitionLogReport& report) const;
        void appendContextTraceSessions(QString& text, const HoleRecognitionLogReport& report) const;
        void appendGroupRefList(QTextStream& out, const Feature::HoleRecognitionResult& result, const std::vector<int>& groupIndices) const;
        void appendCylindricalWorkingGroups(QString& text, const HoleRecognitionLogReport& report) const;


        QString formatIntList(const std::vector<int>& values) const;
        QString formatContextGeometryGroup(const Feature::HoleContextGeometryGroup& group, int displayIndex) const;
        QString formatContextTracePort(const Feature::HoleContextTracePort& port, int displayIndex) const;
        QString formatContextTraceStep(const Feature::HoleContextTraceStep& step, int displayIndex) const;
        QString formatGroupRef(const Feature::HoleRecognitionResult& result, int groupIndex) const;

        const Feature::HoleContextGeometryGroup* findGroupByIndex(const std::vector<Feature::HoleContextGeometryGroup>& groups, int groupIndex) const;
        const Feature::HoleContextTraceStep* findStepByIndex(const std::vector<Feature::HoleContextTraceStep>& steps, int stepIndex) const;

        QString toString(Feature::HoleContextGeometryGroupKind kind) const;
        QString toString(Feature::HoleContextTracePortKind kind) const;
        QString toString(Feature::HoleContextTraceStepKind kind) const;
        QString toString(Feature::CylindricalWallPromotionRejectReason reason) const;

    private:
        AppLogger* m_logger = nullptr;
    };
}

#endif // HOLERECOGNITIONLOGREPORTER_H
