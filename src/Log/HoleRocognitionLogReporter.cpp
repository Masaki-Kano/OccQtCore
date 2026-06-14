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

    QString HoleRecognitionLogReporter::formatHoleRecognition(const HoleRecognitionLogReport& report) const
    {
        QString text;
        QTextStream out(&text);

        out << "Hole Recognition Debug Log\n";
        out << "==========================\n\n";

        if (report.outputSummary)
        {
            appendSummary(text, report);
        }

        if (report.outputWalls)
        {
            appendWalls(text, report);
        }

        // Sections / Traces / Connections / Assemblies は後で追加。

        return text;
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

    void HoleRecognitionLogReporter::appendSummary(QString& text, const HoleRecognitionLogReport& report) const
    {
        const auto& result = report.result;

        QTextStream out(&text);

        out << "Summary\n";
        out << "-------\n";
        out << "Walls: " << static_cast<int>(result.walls.size()) << "\n";
        out << "Sections: " << static_cast<int>(result.sections.size()) << "\n";
        out << "Terminals: " << static_cast<int>(result.terminals.size()) << "\n";
        out << "Connections: " << static_cast<int>(result.connections.size()) << "\n";
        out << "Assemblies: " << static_cast<int>(result.assemblies.size()) << "\n";
        out << "\n";
    }

    void HoleRecognitionLogReporter::appendWalls(
        QString& text,
        const HoleRecognitionLogReport& report) const
    {
        const auto& result = report.result;

        QTextStream out(&text);

        out << "Walls\n";
        out << "-----\n";
        out << "Count: " << static_cast<int>(result.walls.size()) << "\n\n";

        for (int i = 0; i < static_cast<int>(result.walls.size()); ++i)
        {
            out << formatWall(result.walls[i], i);
            out << "\n";
        }
    }

    QString HoleRecognitionLogReporter::formatWall(
        const Feature::HoleWall& wall,
        int displayIndex) const
    {
        QString text;
        QTextStream out(&text);

        out << "Wall[" << displayIndex << "]\n";
        out << "  Index: " << wall.index << "\n";

        out << "  GeometryRefs:\n";
        out << "    Faces: " << formatIntList(wall.geometryRefs.faceIndices) << "\n";
        out << "    Wires: " << formatIntList(wall.geometryRefs.wireIndices) << "\n";
        out << "    Edges: " << formatIntList(wall.geometryRefs.edgeIndices) << "\n";
        out << "    Vertices: " << formatIntList(wall.geometryRefs.vertexIndices) << "\n";

        out << "  Geometry:\n";
        out << "    AxisPoint: "
            << LF::formatPoint(wall.axisPoint) << "\n";
        out << "    AxisDirection: "
            << LF::formatDirection(wall.axisDirection) << "\n";
        out << "    Radius: "
            << QString::number(wall.radius, 'f', 4) << "\n";
        out << "    AxialMin: "
            << QString::number(wall.axialMin, 'f', 4) << "\n";
        out << "    AxialMax: "
            << QString::number(wall.axialMax, 'f', 4) << "\n";

        return text;
    }

    QString HoleRecognitionLogReporter::formatIntList(
        const std::vector<int>& values) const
    {
        if (values.empty())
        {
            return "なし";
        }

        QStringList texts;

        for (const int value : values)
        {
            texts << QString::number(value);
        }

        return texts.join(", ");
    }

}
