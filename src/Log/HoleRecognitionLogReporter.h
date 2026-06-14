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
        bool outputWalls = true;
        bool outputWallBoundaries = true;
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
        void logWalls(const HoleRecognitionLogReport& report) const;

        // QStringへ組み立てる用
        void appendSummary(QString& text, const HoleRecognitionLogReport& report) const;
        void appendWalls(QString& text, const HoleRecognitionLogReport& report) const;
        void appendWallBoundaries(QString& text, const HoleRecognitionLogReport& report) const;

        QString formatWall(const OccQtCore::Feature::HoleWall& wall, int displayIndex) const;
        QString formatIntList(const std::vector<int>& values) const;
        QString formatWallBoundary(const Feature::HoleWallBoundary& boundary, int displayIndex) const;

        QString toString(Feature::HoleWallBoundaryKind kind) const;
        QString toString(Feature::HoleWallBoundaryTraceStatus status) const;

    private:
        AppLogger* m_logger = nullptr;
    };
}

#endif // HOLERECOGNITIONLOGREPORTER_H
