#include "Feature/HoleFeatureRecognizer.h"
#include "Feature/HoleContextGeometryGrouper.h"
#include "Feature/HoleContextTracePortBuilder.h"
#include "Feature/HoleContextTraceStepBuilder.h"

#include "Geometry/GeometryModel.h"
#include "Geometry/TopologyQuery.h"

#include <qDebug>

namespace OccQtCore::Feature
{
    HoleRecognitionResult HoleFeatureRecognizer::recognize(
        const GeometryModel& model) const
    {
        (void)model;

        // TODO:
        // V2 の HoleAssembly / HoleFeature 生成までできたら、
        // Hole::Data へ変換する。
        return {};
    }

    HoleRecognitionResult HoleFeatureRecognizer::recognizeCandidates(
        const GeometryModel& model) const
    {
        HoleRecognitionResult result;

        HoleContextGeometryGrouper contextGrouper(model);
        result.contextGeometryGroups = contextGrouper.group();

        HoleContextTracePortBuilder tracePortBuilder(model);
        result.contextTracePorts =
            tracePortBuilder.build(result.contextGeometryGroups);

        HoleContextTraceStepBuilder traceStepBuilder(
            model,
            result.contextGeometryGroups,
            result.contextTracePorts);

        result.contextTraceSteps = traceStepBuilder.build();

        for (const auto& step : result.contextTraceSteps)
        {
            if (step.outsideGeometryRefs.faceIndices.empty())
            {
                continue;
            }

            if (!step.adjacentExistingGroupIndices.empty())
            {
                continue;
            }

            if (step.sourceGroupIndex < 0 ||
                step.sourceGroupIndex >= static_cast<int>(result.contextGeometryGroups.size()))
            {
                continue;
            }

            const auto& parentGroup =
                result.contextGeometryGroups[step.sourceGroupIndex];

            auto tracedGroups =
                contextGrouper.groupFromGeometryRefs(
                    step.outsideGeometryRefs,
                    parentGroup);

            for (auto& tracedGroup : tracedGroups)
            {
                if (tracedGroup.geometryRefs.faceIndices.empty())
                {
                    continue;
                }

                tracedGroup.index =
                    static_cast<int>(result.contextGeometryGroups.size());

                tracedGroup.note +=
                    " SourceTraceStepIndex=" + std::to_string(step.index) +
                    ", SourceGroupIndex=" + std::to_string(step.sourceGroupIndex) +
                    ", SourcePortIndex=" + std::to_string(step.sourcePortIndex) + ".";

                result.contextGeometryGroups.push_back(tracedGroup);
            }
        }

        int planarCount = 0;

        for (const auto& group : result.contextGeometryGroups)
        {
            if (group.kind == HoleContextGeometryGroupKind::Planar)
            {
                ++planarCount;
            }
        }

        return result;
    }
}
