#ifndef HOLERECOGNITIONLOGREPORTER_H
#define HOLERECOGNITIONLOGREPORTER_H

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
        bool outputWalls = true;

        bool outputSections = false;
        bool outputTraces = false;
        bool outputConnections = false;
        bool outputAssemblies = false;
    };

    class HoleRecognitionLogReporter
    {
    public:
        explicit HoleRecognitionLogReporter(AppLogger* logger);

        void logHoleRecognition(const HoleRecognitionLogReport& report) const;

    private:
        void logSummary(const HoleRecognitionLogReport& report) const;
        void logWalls(const HoleRecognitionLogReport& report) const;

        AppLogger* m_logger = nullptr;
    };
}

#endif // HOLERECOGNITIONLOGREPORTER_H
