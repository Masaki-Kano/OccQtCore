#include "Feature/HoleContextTraceExplorer.h"

#include "Feature/HoleContextGeometryGrouper.h"
#include "Feature/HoleContextGeometryGroupRegistry.h"
#include "Feature/HoleContextTracePortBuilder.h"
#include "Feature/HoleContextTraceStepBuilder.h"
#include "Feature/HoleContextTraversalPolicy.h"
#include "Feature/HoleContextConnectionPolicy.h"
#include "Feature/HoleContextQuery.h"
#include "Core/CollectionUtil.h"

#include <set>
#include <string>
#include <utility>

namespace OccQtCore::Feature
{
    namespace
    {

        bool isEmptyGeometryRefs(const GeometryRefs& refs)
        {
            return refs.faceIndices.empty()
                && refs.wireIndices.empty()
                && refs.edgeIndices.empty()
                && refs.vertexIndices.empty();
        }
    }

    HoleContextTraceExplorer::HoleContextTraceExplorer(
        const GeometryModel& model,
        HoleRecognitionWorkingData* workingData)
        : m_model(model)
        , m_workingData(workingData)
    {
    }

    HoleRecognitionResult HoleContextTraceExplorer::explore() const
    {
        HoleRecognitionResult result;

        HoleContextGeometryGrouper contextGrouper(m_model);
        HoleContextGeometryGroupRegistry groupRegistry(m_model);

        HoleContextTracePortBuilder tracePortBuilder(m_model);
        HoleContextTraversalPolicy traversalPolicy;
        HoleContextConnectionPolicy connectionPolicy;

        const auto contextGroups =
            contextGrouper.group();

        for (const auto& group : contextGroups)
        {
            groupRegistry.registerOrMerge(
                std::move(group));
        }

        result.contextGeometryGroups =
            groupRegistry.groups();

        std::set<int> coveredWallGroupIndices;

        for (const auto& group : result.contextGeometryGroups)
        {
            if (group.kind != HoleContextGeometryGroupKind::WallCandidate)
            {
                continue;
            }

            if (coveredWallGroupIndices.find(group.index) !=
                coveredWallGroupIndices.end())
            {
                continue;
            }

            TraceWorkState workState;
            workState.index =
                static_cast<int>(result.contextTrace.runs.size());
            workState.startGroupIndex = group.index;

            exploreDepthFirst(
                groupRegistry,
                contextGrouper,
                tracePortBuilder,
                traversalPolicy,
                connectionPolicy,
                result,
                workState,
                group.index,
                0);

            HoleContextTraceRun run;
            run.index = workState.index;
            run.startGroupIndex = workState.startGroupIndex;
            run.reachedGroupIndices = workState.reachedGroupIndices;
            run.traceStepIndices = workState.traceStepIndices;
            run.tracePortIndices = workState.tracePortIndices;
            run.completed = true;
            run.note = "DFS trace run.";

            for (const int reachedGroupIndex : workState.reachedGroupIndices)
            {
                const auto* reachedGroup =
                    HoleContextQuery::findGroupByIndex(
                        groupRegistry.groups(),
                        reachedGroupIndex);

                if (reachedGroup == nullptr)
                {
                    continue;
                }

                if (reachedGroup->kind !=
                    HoleContextGeometryGroupKind::WallCandidate)
                {
                    continue;
                }

                CollectionUtil::addUnique(
                    run.reachedWallGroupIndices,
                    reachedGroup->index);

                coveredWallGroupIndices.insert(
                    reachedGroup->index);
            }

            result.contextTrace.runs.push_back(
                std::move(run));
        }

        result.contextGeometryGroups =
            groupRegistry.groups();

        return result;
    }

    std::vector<HoleContextGeometryGroup>
    HoleContextTraceExplorer::buildObservedGroups(
        HoleContextGeometryGrouper& contextGrouper,
        const HoleContextTraceStep& step) const
    {
        if (isEmptyGeometryRefs(step.outsideGeometryRefs))
        {
            return {};
        }

        return contextGrouper.group(
            step.outsideGeometryRefs);
    }

    void HoleContextTraceExplorer::exploreDepthFirst(
        HoleContextGeometryGroupRegistry& groupRegistry,
        HoleContextGeometryGrouper& contextGrouper,
        HoleContextTracePortBuilder& tracePortBuilder,
        HoleContextTraversalPolicy& traversalPolicy,
        HoleContextConnectionPolicy& connectionPolicy,
        HoleRecognitionResult& result,
        TraceWorkState& workState,
        int sourceGroupIndex,
        int depth) const
    {
        if (sourceGroupIndex < 0)
        {
            return;
        }

        if (workState.visitedGroupIndices.find(sourceGroupIndex) !=
            workState.visitedGroupIndices.end())
        {
            return;
        }

        workState.visitedGroupIndices.insert(sourceGroupIndex);

        CollectionUtil::addUnique(
            workState.reachedGroupIndices,
            sourceGroupIndex);

        auto groupPorts =
            tracePortBuilder.buildForGroup(
                groupRegistry.groups(),
                sourceGroupIndex,
                static_cast<int>(result.contextTrace.ports.size()));

        if (groupPorts.empty())
        {
            return;
        }

        for (const auto& port : groupPorts)
        {
            result.contextTrace.ports.push_back(port);

            CollectionUtil::addUnique(
                workState.tracePortIndices,
                port.index);
        }

        HoleContextTraceStepBuilder traceStepBuilder(
            m_model,
            groupRegistry.groups(),
            groupPorts);

        auto groupSteps =
            traceStepBuilder.build();

        for (auto& step : groupSteps)
        {
            step.index =
                static_cast<int>(result.contextTrace.steps.size());

            const int stepIndex = step.index;

            std::vector<int> nextGroupIndices;

            auto observedGroups =
                buildObservedGroups(
                    contextGrouper,
                    step);

            for (auto& observedGroup : observedGroups)
            {
                if (observedGroup.geometryRefs.faceIndices.empty())
                {
                    continue;
                }

                const auto* sourceGroup =
                    HoleContextQuery::findGroupByIndex(
                        groupRegistry.groups(),
                        sourceGroupIndex);

                if (sourceGroup == nullptr)
                {
                    continue;
                }

                const HoleContextGeometryGroup* previousGroup = nullptr;

                const auto parentIt =
                    workState.parentGroupIndexByGroupIndex.find(
                        sourceGroupIndex);

                if (parentIt !=
                    workState.parentGroupIndexByGroupIndex.end())
                {
                    previousGroup =
                        HoleContextQuery::findGroupByIndex(
                            groupRegistry.groups(),
                            parentIt->second);
                }

                HoleContextConnectionPolicy::Context connectionContext;
                connectionContext.previousGroup = previousGroup;
                connectionContext.sourceGroup = sourceGroup;
                connectionContext.observedGroup = &observedGroup;

                const auto connectionDecision =
                    connectionPolicy.decide(connectionContext);

                if (connectionDecision.kind ==
                    HoleContextTraversalDecisionKind::Stop)
                {
                    continue;
                }

                const int existingObservedGroupIndex =
                    groupRegistry.findEquivalentGroupIndex(observedGroup);

                const bool isNewObservedGroup =
                    existingObservedGroupIndex < 0;

                HoleContextTraversalPolicy::Context traversalContext;
                traversalContext.sourceGroupIndex = sourceGroupIndex;
                traversalContext.observedGroupIndex = existingObservedGroupIndex;
                traversalContext.observedGroupIsNewCandidate = isNewObservedGroup;
                traversalContext.stepIndex = stepIndex;
                traversalContext.nextDepth = depth + 1;
                traversalContext.groups = &groupRegistry.groups();
                traversalContext.visitedGroupIndices =
                    &workState.visitedGroupIndices;
                traversalContext.pendingGroupIndexSet = nullptr;
                traversalContext.visitedEdges =
                    &workState.visitedEdges;

                const auto traversalDecision =
                    traversalPolicy.decide(traversalContext);

                if (traversalDecision.kind ==
                    HoleContextTraversalDecisionKind::Stop)
                {
                    continue;
                }

                observedGroup.note +=
                    " SourceTraceStepIndex=" + std::to_string(step.index) +
                    ", SourceGroupIndex=" + std::to_string(step.sourceGroupIndex) +
                    ", SourcePortIndex=" + std::to_string(step.sourcePortIndex) +
                    ".";

                const int observedGroupIndex =
                    isNewObservedGroup
                        ? groupRegistry.registerOrMerge(std::move(observedGroup))
                        : existingObservedGroupIndex;

                if (observedGroupIndex < 0)
                {
                    continue;
                }

                CollectionUtil::addUnique(
                    step.observedGroupIndices,
                    observedGroupIndex);

                workState.visitedEdges.insert(
                    std::make_pair(
                        sourceGroupIndex,
                        observedGroupIndex));

                if (workState.parentGroupIndexByGroupIndex.find(
                        observedGroupIndex) ==
                    workState.parentGroupIndexByGroupIndex.end())
                {
                    workState.parentGroupIndexByGroupIndex[observedGroupIndex] =
                        sourceGroupIndex;
                }

                CollectionUtil::addUnique(
                    nextGroupIndices,
                    observedGroupIndex);
            }

            result.contextTrace.steps.push_back(step);

            CollectionUtil::addUnique(
                workState.traceStepIndices,
                stepIndex);

            for (const int nextGroupIndex : nextGroupIndices)
            {
                exploreDepthFirst(
                    groupRegistry,
                    contextGrouper,
                    tracePortBuilder,
                    traversalPolicy,
                    connectionPolicy,
                    result,
                    workState,
                    nextGroupIndex,
                    depth + 1);
            }
        }
    }
}
