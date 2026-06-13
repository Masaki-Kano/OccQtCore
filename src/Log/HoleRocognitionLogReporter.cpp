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

        if (report.outputWalls)
        {
            logWalls(report);
        }

        // Sections / Traces / Connections / Assemblies は後で追加。
    }

    void HoleRecognitionLogReporter::logSummary(const HoleRecognitionLogReport& report) const
    {
        const auto& result = report.result;

        m_logger->info("========== 穴フィーチャ認識レポート ==========");
        m_logger->info(QString("穴壁数: %1").arg(result.walls.size()));
        m_logger->info(QString("穴セクション数: %1").arg(result.sections.size()));
        m_logger->info(QString("穴終端解釈数: %1").arg(result.terminals.size()));
        m_logger->info(QString("穴接続解釈数: %1").arg(result.connections.size()));
        m_logger->info(QString("穴アセンブリ数: %1").arg(result.assemblies.size()));
        m_logger->info("=====================================");
    }

    void HoleRecognitionLogReporter::logWalls(const HoleRecognitionLogReport& report) const
    {
        const auto& model = report.model;
        const auto& result = report.result;

        m_logger->info("========== 穴壁 ==========");
        m_logger->info(QString("穴壁数: %1").arg(result.walls.size()));

        for (const auto& wall : result.walls)
        {
            const QString message =
                QString("Wall[%1]: Faces=%2, AxisPoint=%3, Axis=%4, Radius=%5, AxialRange=[%6, %7]")
                    .arg(wall.index)
                    .arg(LF::formatFaceIndexList(model, wall.geometryRefs.faceIndices))
                    .arg(LF::formatPoint(wall.axisPoint))
                    .arg(LF::formatDirection(wall.axisDirection))
                    .arg(wall.radius, 0, 'f', 3)
                    .arg(wall.axialMin, 0, 'f', 3)
                    .arg(wall.axialMax, 0, 'f', 3);

            m_logger->info(message);
        }
    }
}
