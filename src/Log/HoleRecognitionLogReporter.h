#ifndef HOLERECOGNITIONLOGREPORTER_H
#define HOLERECOGNITIONLOGREPORTER_H

#include "Feature/HoleRecognitionTypes.h"

namespace OccQtCore
{
    class AppLogger;
    class GeometryModel;

    struct HoleRecognitionLogReport
    {
        const GeometryModel& model;
        const Feature::HoleRecognitionResult& result;

        bool outputSummary = true;
        bool outputWallCandidates = true;
        bool outputEndCandidates = true;
        bool outputSegmentCandidates = true;
        bool outputHoleCandidates = true;
    };

    class HoleRecognitionLogReporter
    {
    public:
        explicit HoleRecognitionLogReporter(AppLogger* logger);

        void logHoleRecognition(const HoleRecognitionLogReport& report) const;

    private:
        void logSummary(const HoleRecognitionLogReport& report) const;

        void logWallCandidates(const HoleRecognitionLogReport& report) const;

        void logEndCandidates(const HoleRecognitionLogReport& report) const;

        void logSegmentCandidates(const HoleRecognitionLogReport& report) const;

        void logHoleCandidates(const HoleRecognitionLogReport& report) const;

    private:
        AppLogger* m_logger = nullptr;
    };
}

#endif // HOLERECOGNITIONLOGREPORTER_H
