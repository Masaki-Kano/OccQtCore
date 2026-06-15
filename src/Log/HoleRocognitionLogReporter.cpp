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

        if (report.outputWallBoundaries)
        {
            appendWallBoundaries(text, report);
        }

        if (report.outputGeometryTraces)
        {
            appendGeometryTraces(text, report);
        }

        return text;
    }

    void HoleRecognitionLogReporter::logSummary(const HoleRecognitionLogReport& report) const
    {
        const auto& result = report.result;

        m_logger->info("========== 穴フィーチャ認識レポート ==========");
        m_logger->info(QString("穴壁数: %1").arg(result.walls.size()));
        m_logger->info(QString("穴壁境界数: %1").arg(result.wallBoundaries.size()));
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
        out << "WallBoundaries: " << static_cast<int>(result.wallBoundaries.size()) << "\n";
        out << "GeometryTraces: " << static_cast<int>(result.geometryTraces.size()) << "\n";
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

    void HoleRecognitionLogReporter::appendWallBoundaries(
        QString& text,
        const HoleRecognitionLogReport& report) const
    {
        const auto& result = report.result;

        QTextStream out(&text);

        out << "Wall Boundaries\n";
        out << "---------------\n";
        out << "Count: " << static_cast<int>(result.wallBoundaries.size()) << "\n\n";

        for (int i = 0; i < static_cast<int>(result.wallBoundaries.size()); ++i)
        {
            out << formatWallBoundary(result.wallBoundaries[i], i);
            out << "\n";
        }
    }

    void HoleRecognitionLogReporter::appendGeometryTraces(
        QString& text,
        const HoleRecognitionLogReport& report) const
    {
        const auto& result = report.result;

        QTextStream out(&text);

        out << "Geometry Traces\n";
        out << "---------------\n";
        out << "Count: " << static_cast<int>(result.geometryTraces.size()) << "\n\n";

        for (int i = 0; i < static_cast<int>(result.geometryTraces.size()); ++i)
        {
            out << formatGeometryTrace(result.geometryTraces[i], i);
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

    QString HoleRecognitionLogReporter::formatWallBoundary(
        const Feature::HoleWallBoundary& boundary,
        int displayIndex) const
    {
        QString text;
        QTextStream out(&text);

        out << "Boundary[" << displayIndex << "]\n";
        out << "  Index: " << boundary.index << "\n";
        out << "  WallIndex: " << boundary.wallIndex << "\n";
        out << "  Kind: " << toString(boundary.kind) << "\n";
        out << "  TraceStatus: " << toString(boundary.traceStatus) << "\n";

        out << "  GeometryRefs:\n";
        out << "    Faces: " << formatIntList(boundary.geometryRefs.faceIndices) << "\n";
        out << "    Wires: " << formatIntList(boundary.geometryRefs.wireIndices) << "\n";
        out << "    Edges: " << formatIntList(boundary.geometryRefs.edgeIndices) << "\n";
        out << "    Vertices: " << formatIntList(boundary.geometryRefs.vertexIndices) << "\n";

        out << "  AdjacentGeometryRefs:\n";
        out << "    Faces: " << formatIntList(boundary.adjacentGeometryRefs.faceIndices) << "\n";
        out << "    Wires: " << formatIntList(boundary.adjacentGeometryRefs.wireIndices) << "\n";
        out << "    Edges: " << formatIntList(boundary.adjacentGeometryRefs.edgeIndices) << "\n";
        out << "    Vertices: " << formatIntList(boundary.adjacentGeometryRefs.vertexIndices) << "\n";

        out << "  AxialRange: ["
            << QString::number(boundary.axialMin, 'f', 4) << ", "
            << QString::number(boundary.axialMax, 'f', 4) << "]\n";

        out << "  AxialPosition: "
            << QString::number(boundary.axialPosition, 'f', 4) << "\n";

        out << "  CircumferentialCoverage: "
            << QString::number(boundary.circumferentialCoverage, 'f', 4) << "\n";

        if (!boundary.note.empty())
        {
            out << "  Note: " << QString::fromStdString(boundary.note) << "\n";
        }

        return text;
    }

    QString HoleRecognitionLogReporter::formatGeometryTrace(
        const Feature::GeometryTrace& trace,
        int treeIndex) const
    {
        QString text;
        QTextStream out(&text);

        out << "Trace[" << treeIndex << "]\n";
        out << "  TraceIndex: " << trace.index << "\n";
        out << "  SourceBoundaryIndex: " << trace.sourceBoundaryIndex << "\n";
        out << "  EndReason: " << toString(trace.endReason) << "\n";

        if (!trace.note.empty())
        {
            out << "  Note: " << QString::fromStdString(trace.note) << "\n";
        }

        out << "  Nodes\n";
        out << "  -----\n";
        out << "  Count: " << static_cast<int>(trace.nodes.size()) << "\n";

        for (int i = 0; i < static_cast<int>(trace.nodes.size()); ++i)
        {
            out << formatGeometryTraceNode(trace.nodes[i], i);
        }

        return text;
    }

    QString HoleRecognitionLogReporter::formatGeometryTraceNode(
        const Feature::GeometryTraceNode& node,
        int treeIndex) const
    {
        QString text;
        QTextStream out(&text);

        out << "  Node[" << treeIndex << "]\n";
        out << "    NodeIndex: " << node.index << "\n";
        out << "    Depth: " << node.depth << "\n";
        out << "    ParentNodeIndex: " << node.parentNodeIndex << "\n";

        out << "    GeometryRefs:\n";
        out << "      Faces: " << formatIntList(node.geometryRefs.faceIndices) << "\n";
        out << "      Wires: " << formatIntList(node.geometryRefs.wireIndices) << "\n";
        out << "      Edges: " << formatIntList(node.geometryRefs.edgeIndices) << "\n";
        out << "      Vertices: " << formatIntList(node.geometryRefs.vertexIndices) << "\n";

        out << "    Geometry:\n";
        out << "      AxialMin: " << QString::number(node.axialMin, 'f', 4) << "\n";
        out << "      AxialMax: " << QString::number(node.axialMax, 'f', 4) << "\n";

        if (!node.note.empty())
        {
            out << "    Note: " << QString::fromStdString(node.note) << "\n";
        }

        return text;
    }

    QString HoleRecognitionLogReporter::toString(
        Feature::HoleWallBoundaryKind kind) const
    {
        switch (kind)
        {
        case Feature::HoleWallBoundaryKind::Unknown:
            return "Unknown";
        case Feature::HoleWallBoundaryKind::AxialEnd:
            return "AxialEnd";
        case Feature::HoleWallBoundaryKind::LateralConnection:
            return "LateralConnection";
        case Feature::HoleWallBoundaryKind::InternalWallSplit:
            return "InternalWallSplit";
        case Feature::HoleWallBoundaryKind::Broken:
            return "Broken";
        case Feature::HoleWallBoundaryKind::Ambiguous:
            return "Ambiguous";
        }

        return "Unknown";
    }

    QString HoleRecognitionLogReporter::toString(
        Feature::HoleWallBoundaryTraceStatus status) const
    {
        switch (status)
        {
        case Feature::HoleWallBoundaryTraceStatus::Unknown:
            return "Unknown";
        case Feature::HoleWallBoundaryTraceStatus::Traceable:
            return "Traceable";
        case Feature::HoleWallBoundaryTraceStatus::Ignored:
            return "Ignored";
        case Feature::HoleWallBoundaryTraceStatus::NotTraceable:
            return "NotTraceable";
        }

        return "Unknown";
    }

    QString HoleRecognitionLogReporter::toString(OccQtCore::Feature::GeometryTraceEndReason reason) const
    {
        using Reason = OccQtCore::Feature::GeometryTraceEndReason;

        switch (reason)
        {
        case Reason::Unknown:
            return "Unknown";
        case Reason::ReachedHoleWall:
            return "ReachedHoleWall";
        case Reason::NoHoleWallCandidate:
            return "NoHoleWallCandidate";
        case Reason::OutOfHoleContext:
            return "OutOfHoleContext";
        case Reason::Ambiguous:
            return "Ambiguous";
        case Reason::LoopDetected:
            return "LoopDetected";
        case Reason::MaxDepthReached:
            return "MaxDepthReached";
        }

        return "Unknown";
    }

}
