#include <QString>

#include "Log/HoleRecognitionLogReporter.h"
#include "Log/AppLogger.h"
#include "Log/LogFormatUtil.h"

namespace
{
    namespace LF = OccQtCore::LogFormatUtil;
}

namespace OccQtCore
{
    HoleRecognitionLogReporter::HoleRecognitionLogReporter(AppLogger* logger)
        : m_logger(logger)
    {
    }

    void HoleRecognitionLogReporter::logHoleRecognition(const HoleRecognitionLogReport& report) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

        if (report.outputSummary)
        {
            logSummary(report);
        }

        if (report.outputWallCandidates)
        {
            logWallCandidates(report);
        }

        if (report.outputEndCandidates)
        {
            logEndCandidates(report);
        }

        if (report.outputSegmentCandidates)
        {
            logSegmentCandidates(report);
        }

        if (report.outputHoleCandidates)
        {
            logHoleCandidates(report);
        }
    }

    void HoleRecognitionLogReporter::logSummary(const HoleRecognitionLogReport& report) const
    {
        const auto& result = report.result;

        m_logger->info("========== 穴フィーチャ認識レポート ==========");
        m_logger->info(QString("穴壁候補数: %1").arg(result.wallCandidates.size()));
        m_logger->info(QString("穴端候補数: %1").arg(result.endCandidates.size()));
        m_logger->info(QString("穴セグメント候補数: %1").arg(result.segmentCandidates.size()));
        m_logger->info(QString("穴候補数: %1").arg(result.holeCandidates.size()));
        m_logger->info("=====================================");
    }

    void HoleRecognitionLogReporter::logWallCandidates(const HoleRecognitionLogReport& report) const
    {
        const auto& model = report.model;
        const auto& result = report.result;

        m_logger->info("========== 穴壁候補 ==========");
        m_logger->info(QString("候補数: %1").arg(result.wallCandidates.size()));

        for (const auto& candidate: result.wallCandidates)
        {
            const QString message =
                QString("WallCandidate[%1]: Faces=%2, Center=%3, Axis=%4, Radius=%5, Depth=%6")
                    .arg(candidate.index)
                    .arg(LF::formatFaceIndexList(model, candidate.geometryRefs.faceIndices))
                    .arg(LF::formatPoint(candidate.center))
                    .arg(LF::formatDirection(candidate.axisDirection))
                    .arg(candidate.radius, 0, 'f', 3)
                    .arg(candidate.depth, 0, 'f', 3);

            m_logger->info(message);
        }
    }

    void HoleRecognitionLogReporter::logEndCandidates(const HoleRecognitionLogReport& report) const
    {
        const auto& model = report.model;
        const auto& result = report.result;

        m_logger->info("========== 穴端候補 ==========");
        m_logger->info(QString("候補数: %1").arg(result.endCandidates.size()));

        for (const auto& candidate : result.endCandidates)
        {
            const QString message =
                QString("EndCandidate[%1]: Type=%2, Wall=%3, Faces=%4, Edges=%5")
                    .arg(candidate.index)
                    .arg(Feature::holeEndCandidateTypeDisplayName(candidate.type))
                    .arg(candidate.wallCandidateIndex)
                    .arg(LF::formatFaceIndexList(model, candidate.geometryRefs.faceIndices))
                    .arg(LF::formatEdgeIndexList(model, candidate.geometryRefs.edgeIndices));

            m_logger->info(message);
        }
    }

    void HoleRecognitionLogReporter::logSegmentCandidates(const HoleRecognitionLogReport& report) const
    {
        const auto& model = report.model;
        const auto& result = report.result;

        m_logger->info("========== 穴セグメント候補 ==========");
        m_logger->info(QString("候補数: %1").arg(result.segmentCandidates.size()));

        for (const auto& segment: result.segmentCandidates)
        {
            QString message;

            message += QString("SegmentCandidate[%1]: ").arg(segment.index);

            message += QString("Wall=%1").arg(segment.wallCandidateIndex);

            if (segment.wallCandidateIndex >= 0 &&
                segment.wallCandidateIndex < static_cast<int>(result.wallCandidates.size()))
            {
                const auto& wall = result.wallCandidates[segment.wallCandidateIndex];

                message += QString(", WallFaces=%1").arg(LF::formatFaceIndexList(model, wall.geometryRefs.faceIndices));

                message += QString(", Center=%1").arg(LF::formatPoint(wall.center));

                message += QString(", Axis=%1").arg(LF::formatDirection(wall.axisDirection));

                message += QString(", Radius=%1").arg(wall.radius);
            }

            message += QString(", EndIndices=%1").arg(LF::formatIndexList(segment.endCandidateIndices));

            m_logger->info(message);

            for (int endCandidateIndex: segment.endCandidateIndices)
            {
                if (endCandidateIndex < 0 ||
                    endCandidateIndex >= static_cast<int>(result.endCandidates.size()))
                {
                    m_logger->info(QString(" End[%1]: <invalid>").arg(endCandidateIndex));
                    continue;
                }

                const auto& end = result.endCandidates[endCandidateIndex];

                QString endMessage;

                endMessage += QString(" End[%1]: Type=%2")
                                    .arg(end.index)
                                    .arg(Feature::holeEndCandidateTypeDisplayName(end.type));

                endMessage += QString(", Faces=%1").arg(LF::formatFaceIndexList(model, end.geometryRefs.faceIndices));

                endMessage += QString(", Edges=%1").arg(LF::formatEdgeIndexList(model, end.geometryRefs.edgeIndices));

                m_logger->info(endMessage);
            }
        }
    }

    void HoleRecognitionLogReporter::logHoleCandidates(const HoleRecognitionLogReport& report) const
    {
        Q_UNUSED(report);

        m_logger->info("========= 穴候補 =========");

        // 後で
    }
}
