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
    }

    QString HoleRecognitionLogReporter::formatHoleRecognition(const HoleRecognitionLogReport& report) const
    {
        QString text;
        QTextStream out(&text);

        out << "Hole Context DFS Debug Log\n";
        out << "==========================\n\n";

        if (report.outputSummary)
        {
            appendSummary(text, report);
        }

        if (report.outputTraceSession)
        {
            appendContextTraceSessions(text, report);
        }

        if (report.outputGeometryGroup)
        {
            appendContextGeometryGroups(text, report);
        }

        if (report.outputTraceStep)
        {
            appendContextTraceSteps(text, report);
        }

        // Portは必要な時だけ
        if (report.outputPort)
        {
            appendContextTracePorts(text, report);
        }

        return text;
    }

    void HoleRecognitionLogReporter::logSummary(const HoleRecognitionLogReport& report) const
    {
        const auto& result = report.result;

        m_logger->info("========== 穴フィーチャ認識レポート ==========");
        m_logger->info(QString("ContextGeometryGroups: %1")
                           .arg(result.contextGeometryGroups.size()));
        m_logger->info(QString("ContextTraceRuns: %1")
                           .arg(result.contextTrace.runs.size()));
        m_logger->info(QString("ContextTraceSteps: %1")
                           .arg(result.contextTrace.steps.size()));
        m_logger->info(QString("ContextTracePorts: %1")
                           .arg(result.contextTrace.ports.size()));
        m_logger->info("=====================================");
    }

    void HoleRecognitionLogReporter::appendSummary(QString& text, const HoleRecognitionLogReport& report) const
    {
        const auto& result = report.result;

        QTextStream out(&text);

        out << "Summary\n";
        out << "-------\n";
        out << "ContextGeometryGroups: "
            << static_cast<int>(result.contextGeometryGroups.size()) << "\n";
        out << "ContextTraceRuns: "
            << static_cast<int>(result.contextTrace.runs.size()) << "\n";
        out << "ContextTraceSteps: "
            << static_cast<int>(result.contextTrace.steps.size()) << "\n";
        out << "ContextTracePorts: "
            << static_cast<int>(result.contextTrace.ports.size()) << "\n";
        out << "\n";
    }

    void HoleRecognitionLogReporter::appendContextGeometryGroups(
        QString& text,
        const HoleRecognitionLogReport& report) const
    {
        const auto& result = report.result;

        QTextStream out(&text);

        out << "Context Geometry Groups\n";
        out << "-----------------------\n";
        out << "Count: "
            << static_cast<int>(result.contextGeometryGroups.size()) << "\n\n";

        for (int i = 0; i < static_cast<int>(result.contextGeometryGroups.size()); ++i)
        {
            out << formatContextGeometryGroup(result.contextGeometryGroups[i], i);
            out << "\n";
        }
    }

    void HoleRecognitionLogReporter::appendContextTracePorts(
        QString& text,
        const HoleRecognitionLogReport& report) const
    {
        const auto& result = report.result;

        QTextStream out(&text);

        out << "Context Trace Ports\n";
        out << "-------------------\n";
        out << "Count: "
            << static_cast<int>(result.contextTrace.ports.size()) << "\n\n";

        for (int i = 0; i < static_cast<int>(result.contextTrace.ports.size()); ++i)
        {
            out << formatContextTracePort(result.contextTrace.ports[i], i);
            out << "\n";
        }
    }

    void HoleRecognitionLogReporter::appendContextTraceSteps(
        QString& text,
        const HoleRecognitionLogReport& report) const
    {
        const auto& result = report.result;

        QTextStream out(&text);

        out << "Context Trace Steps\n";
        out << "-------------------\n";
        out << "Count: "
            << static_cast<int>(result.contextTrace.steps.size()) << "\n\n";

        for (int i = 0; i < static_cast<int>(result.contextTrace.steps.size()); ++i)
        {
            out << formatContextTraceStep(result.contextTrace.steps[i], i);
            out << "\n";
        }
    }

    void HoleRecognitionLogReporter::appendContextTraceSessions(
        QString& text,
        const HoleRecognitionLogReport& report) const
    {
        const auto& result = report.result;

        QTextStream out(&text);

        out << "Context Trace Runs\n";
        out << "------------------\n";
        out << "Count: "
            << static_cast<int>(result.contextTrace.runs.size()) << "\n\n";

        for (const auto& run : result.contextTrace.runs)
        {
            out << "Run[" << run.index << "]\n";
            out << "  StartGroup: "
                << formatGroupRef(result, run.startGroupIndex) << "\n";

            out << "  ReachedGroups: ";
            appendGroupRefList(out, result, run.reachedGroupIndices);
            out << "\n";

            out << "  ReachedWalls: ";
            appendGroupRefList(out, result, run.reachedWallGroupIndices);
            out << "\n";

            out << "  Ports: "
                << formatIntList(run.tracePortIndices) << "\n";

            out << "  Steps:\n";

            for (const int stepIndex : run.traceStepIndices)
            {
                const auto* step =
                    findStepByIndex(result.contextTrace.steps, stepIndex);

                if (step == nullptr)
                {
                    out << "    Step[" << stepIndex << "] not found\n";
                    continue;
                }

                out << "    Step[" << step->index << "] "
                    << formatGroupRef(result, step->sourceGroupIndex)
                    << " -> ";

                if (step->observedGroupIndices.empty())
                {
                    out << "(none)";
                }
                else
                {
                    appendGroupRefList(out, result, step->observedGroupIndices);
                }

                out << "\n";
            }

            out << "  Completed: "
                << (run.completed ? "true" : "false") << "\n";

            if (!run.note.empty())
            {
                out << "  Note: " << QString::fromStdString(run.note) << "\n";
            }

            out << "\n";
        }
    }

    void HoleRecognitionLogReporter::appendGroupRefList(
        QTextStream& out,
        const Feature::HoleRecognitionResult& result,
        const std::vector<int>& groupIndices) const
    {
        if (groupIndices.empty())
        {
            out << "なし";
            return;
        }

        for (int i = 0; i < static_cast<int>(groupIndices.size()); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }

            out << formatGroupRef(result, groupIndices[i]);
        }
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

    QString HoleRecognitionLogReporter::formatContextGeometryGroup(
        const Feature::HoleContextGeometryGroup& group,
        int displayIndex) const
    {
        QString text;
        QTextStream out(&text);

        out << "Group[" << displayIndex << "]\n";
        out << "  Index: " << group.index << "\n";
        out << "  Kind: " << toString(group.kind) << "\n";

        out << "  GeometryRefs:\n";
        out << "    Faces: " << formatIntList(group.geometryRefs.faceIndices) << "\n";
        out << "    Wires: " << formatIntList(group.geometryRefs.wireIndices) << "\n";
        out << "    Edges: " << formatIntList(group.geometryRefs.edgeIndices) << "\n";
        out << "    Vertices: " << formatIntList(group.geometryRefs.vertexIndices) << "\n";

        out << "  Geometry:\n";
        out << "    HasAxis: " << (group.hasReferenceDirection ? "true" : "false") << "\n";

        if (group.hasReferenceDirection)
        {
            out << "    AxisPoint: "
                << LF::formatPoint(group.referencePoint) << "\n";
            out << "    AxisDirection: "
                << LF::formatDirection(group.referenceDirection) << "\n";
            out << "    Radius: "
                << QString::number(group.radius, 'f', 4) << "\n";
            out << "    AxialMin: "
                << QString::number(group.parameterMin, 'f', 4) << "\n";
            out << "    AxialMax: "
                << QString::number(group.parameterMax, 'f', 4) << "\n";
            out << "    AxialPosition: "
                << QString::number(group.parameterPosition, 'f', 4) << "\n";
        }

        if (!group.note.empty())
        {
            out << "  Note: " << QString::fromStdString(group.note) << "\n";
        }

        return text;
    }

    QString HoleRecognitionLogReporter::formatContextTracePort(
        const Feature::HoleContextTracePort& port,
        int displayIndex) const
    {
        QString text;
        QTextStream out(&text);

        out << "Port[" << displayIndex << "]\n";
        out << "  Index: " << port.index << "\n";
        out << "  SourceGroupIndex: " << port.sourceGroupIndex << "\n";
        out << "  Kind: " << toString(port.kind) << "\n";

        out << "  GeometryRefs:\n";
        out << "    Faces: " << formatIntList(port.geometryRefs.faceIndices) << "\n";
        out << "    Wires: " << formatIntList(port.geometryRefs.wireIndices) << "\n";
        out << "    Edges: " << formatIntList(port.geometryRefs.edgeIndices) << "\n";
        out << "    Vertices: " << formatIntList(port.geometryRefs.vertexIndices) << "\n";

        out << "  AxialRange: ["
            << QString::number(port.axialMin, 'f', 4) << ", "
            << QString::number(port.axialMax, 'f', 4) << "]\n";

        out << "  AxialPosition: "
            << QString::number(port.axialPosition, 'f', 4) << "\n";

        out << "  CircumferentialCoverage: "
            << QString::number(port.circumferentialCoverage, 'f', 4) << "\n";

        if (!port.note.empty())
        {
            out << "  Note: " << QString::fromStdString(port.note) << "\n";
        }

        return text;
    }

    QString HoleRecognitionLogReporter::formatContextTraceStep(
        const Feature::HoleContextTraceStep& step,
        int displayIndex) const
    {
        QString text;
        QTextStream out(&text);

        out << "Step[" << displayIndex << "]\n";
        out << "  Index: " << step.index << "\n";
        out << "  SourceGroupIndex: " << step.sourceGroupIndex << "\n";
        out << "  SourcePortIndex: " << step.sourcePortIndex << "\n";
        out << "  Kind: " << toString(step.kind) << "\n";

        out << "  PortGeometryRefs:\n";
        out << "    Faces: " << formatIntList(step.portGeometryRefs.faceIndices) << "\n";
        out << "    Wires: " << formatIntList(step.portGeometryRefs.wireIndices) << "\n";
        out << "    Edges: " << formatIntList(step.portGeometryRefs.edgeIndices) << "\n";
        out << "    Vertices: " << formatIntList(step.portGeometryRefs.vertexIndices) << "\n";

        out << "  OutsideGeometryRefs:\n";
        out << "    Faces: " << formatIntList(step.outsideGeometryRefs.faceIndices) << "\n";
        out << "    Wires: " << formatIntList(step.outsideGeometryRefs.wireIndices) << "\n";
        out << "    Edges: " << formatIntList(step.outsideGeometryRefs.edgeIndices) << "\n";
        out << "    Vertices: " << formatIntList(step.outsideGeometryRefs.vertexIndices) << "\n";

        out << "  ObservedGroupIndices: "
            << formatIntList(step.observedGroupIndices) << "\n";

        if (!step.note.empty())
        {
            out << "  Note: " << QString::fromStdString(step.note) << "\n";
        }

        return text;
    }

    QString HoleRecognitionLogReporter::formatGroupRef(
        const Feature::HoleRecognitionResult& result,
        int groupIndex) const
    {
        const auto* group =
            findGroupByIndex(result.contextGeometryGroups, groupIndex);

        if (group == nullptr)
        {
            return QString("Group[%1](not found)").arg(groupIndex);
        }

        return QString("Group[%1](%2)")
            .arg(group->index)
            .arg(toString(group->kind));
    }

    const Feature::HoleContextGeometryGroup* HoleRecognitionLogReporter::findGroupByIndex(
        const std::vector<Feature::HoleContextGeometryGroup>& groups,
        int groupIndex) const
    {
        for (const auto& group : groups)
        {
            if (group.index == groupIndex)
            {
                return &group;
            }
        }

        return nullptr;
    }

    const Feature::HoleContextTraceStep* HoleRecognitionLogReporter::findStepByIndex(
        const std::vector<Feature::HoleContextTraceStep>& steps,
        int stepIndex) const
    {
        for (const auto& step : steps)
        {
            if (step.index == stepIndex)
            {
                return &step;
            }
        }

        return nullptr;
    }

    QString HoleRecognitionLogReporter::toString(
        Feature::HoleContextGeometryGroupKind kind) const
    {
        switch (kind)
        {
        case Feature::HoleContextGeometryGroupKind::Unknown:
            return "Unknown";
        case Feature::HoleContextGeometryGroupKind::WallCandidate:
            return "WallCandidate";
        case Feature::HoleContextGeometryGroupKind::BoundaryCandidate:
            return "BoundaryCandidate";
        case Feature::HoleContextGeometryGroupKind::TransitionCandidate:
            return "TransitionCandidate";
        case Feature::HoleContextGeometryGroupKind::Ambiguous:
            return "Ambiguous";
        }

        return "Unknown";
    }

    QString HoleRecognitionLogReporter::toString(
        Feature::HoleContextTracePortKind kind) const
    {
        switch (kind)
        {
        case Feature::HoleContextTracePortKind::Unknown:
            return "Unknown";
        case Feature::HoleContextTracePortKind::ExternalTransition:
            return "ExternalTransition";
        case Feature::HoleContextTracePortKind::InternalLoop:
            return "InternalLoop";
        case Feature::HoleContextTracePortKind::Ambiguous:
            return "Ambiguous";
        }

        return "Unknown";
    }

    QString HoleRecognitionLogReporter::toString(
        Feature::HoleContextTraceStepKind kind) const
    {
        switch (kind)
        {
        case Feature::HoleContextTraceStepKind::Unknown:
            return "Unknown";
        case Feature::HoleContextTraceStepKind::NoOutsideFace:
            return "NoOutsideFace";
        case Feature::HoleContextTraceStepKind::OutsideFace:
            return "OutsideFace";
        case Feature::HoleContextTraceStepKind::ReachedExistingGroup:
            return "ReachedExistingGroup";
        case Feature::HoleContextTraceStepKind::Ambiguous:
            return "Ambiguous";
        }

        return "Unknown";
    }

    QString HoleRecognitionLogReporter::toString(
        Feature::CylindricalWallPromotionRejectReason reason) const
    {
        using Reason = Feature::CylindricalWallPromotionRejectReason;

        switch (reason)
        {
        case Reason::None:
            return "None";

        case Reason::NotWallCandidateKind:
            return "NotWallCandidateKind";

        case Reason::EmptyFaces:
            return "EmptyFaces";

        case Reason::MissingReference:
            return "MissingReference";

        case Reason::InvalidRadius:
            return "InvalidRadius";

        case Reason::InsufficientCircumferentialCoverage:
            return "InsufficientCircumferentialCoverage";

        case Reason::MissingTopologicalCircumferentialLoop:
            return "MissingTopologicalCircumferentialLoop";

        case Reason::NoInnerCylindricalFace:
            return "NoInnerCylindricalFace";
        }

        return "Unknown";
    }

}
