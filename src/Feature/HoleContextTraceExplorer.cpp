#include "Feature/HoleContextTraceExplorer.h"

#include "Feature/HoleContextGeometryGrouper.h"
#include "Feature/HoleContextGeometryGroupRegistry.h"
#include "Feature/HoleContextTracePortBuilder.h"
#include "Feature/HoleContextTraceStepBuilder.h"
#include "Feature/HoleContextTraversalPolicy.h"
#include "Feature/HoleContextConnectionPolicy.h"
#include "Feature/HoleContextSeedWallFinder.h"

#include <set>
#include <algorithm>
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

        const HoleContextGeometryGroup* findGroupByIndex(
            const std::vector<HoleContextGeometryGroup>& groups,
            int groupIndex)
        {
            const auto it =
                std::find_if(
                    groups.begin(),
                    groups.end(),
                    [groupIndex](const HoleContextGeometryGroup& group)
                    {
                        return group.index == groupIndex;
                    });

            if (it == groups.end())
            {
                return nullptr;
            }

            return &(*it);
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
        HoleContextSeedWallFinder seedWallFinder(
            m_model,
            m_workingData);

        std::set<int> exploredWallGroupIndices;

        while (true)
        {
            auto seedWall =
                seedWallFinder.findNext(
                    groupRegistry,
                    exploredWallGroupIndices);

            if (!seedWall.has_value())
            {
                break;
            }

            const int seedGroupIndex =
                groupRegistry.registerOrMerge(std::move(*seedWall));

            if (seedGroupIndex < 0)
            {
                continue;
            }

            if (exploredWallGroupIndices.find(seedGroupIndex) !=
                exploredWallGroupIndices.end())
            {
                continue;
            }

            TraceSession session;
            session.index =
                static_cast<int>(result.contextTraceSessions.size());
            session.seedGroupIndex = seedGroupIndex;

            exploreDepthFirst(
                groupRegistry,
                contextGrouper,
                tracePortBuilder,
                traversalPolicy,
                connectionPolicy,
                result,
                session,
                seedGroupIndex,
                0);

            HoleContextTraceSession outputSession;
            outputSession.index = session.index;
            outputSession.seedGroupIndex = session.seedGroupIndex;
            outputSession.reachedGroupIndices = session.reachedGroupIndices;
            outputSession.traceStepIndices = session.traceStepIndices;

            for (const int reachedGroupIndex : session.reachedGroupIndices)
            {
                const auto* reachedGroup =
                    findGroupByIndex(
                        groupRegistry.groups(),
                        reachedGroupIndex);

                if (reachedGroup == nullptr)
                {
                    continue;
                }

                if (reachedGroup->kind ==
                    HoleContextGeometryGroupKind::WallCandidate)
                {
                    outputSession.reachedWallGroupIndices.push_back(
                        reachedGroup->index);
                }
            }

            outputSession.note = "DFS trace session.";

            result.contextTraceSessions.push_back(
                std::move(outputSession));

            for (const int reachedGroupIndex : session.reachedGroupIndices)
            {
                const auto* reachedGroup =
                    findGroupByIndex(
                        groupRegistry.groups(),
                        reachedGroupIndex);

                if (reachedGroup == nullptr)
                {
                    continue;
                }

                if (reachedGroup->kind == HoleContextGeometryGroupKind::WallCandidate)
                {
                    exploredWallGroupIndices.insert(reachedGroup->index);
                }
            }
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
        TraceSession& session,
        int sourceGroupIndex,
        int depth) const
    {
        if (sourceGroupIndex < 0)
        {
            return;
        }

        if (session.visitedGroupIndices.find(sourceGroupIndex) !=
            session.visitedGroupIndices.end())
        {
            return;
        }

        session.visitedGroupIndices.insert(sourceGroupIndex);

        if (std::find(
                session.reachedGroupIndices.begin(),
                session.reachedGroupIndices.end(),
                sourceGroupIndex) == session.reachedGroupIndices.end())
        {
            session.reachedGroupIndices.push_back(sourceGroupIndex);
        }

        auto groupPorts =
            tracePortBuilder.buildForGroup(
                groupRegistry.groups(),
                sourceGroupIndex,
                static_cast<int>(result.contextTracePorts.size()));

        if (groupPorts.empty())
        {
            return;
        }

        result.contextTracePorts.insert(
            result.contextTracePorts.end(),
            groupPorts.begin(),
            groupPorts.end());

        HoleContextTraceStepBuilder traceStepBuilder(
            m_model,
            groupRegistry.groups(),
            groupPorts);

        auto groupSteps =
            traceStepBuilder.build();

        for (auto& step : groupSteps)
        {
            step.index =
                static_cast<int>(result.contextTraceSteps.size());

            const int stepIndex = step.index;

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
                    findGroupByIndex(
                        groupRegistry.groups(),
                        sourceGroupIndex);

                if (sourceGroup == nullptr)
                {
                    continue;
                }

                const HoleContextGeometryGroup* previousGroup = nullptr;

                const auto parentIt =
                    session.parentGroupIndexByGroupIndex.find(sourceGroupIndex);

                if (parentIt != session.parentGroupIndexByGroupIndex.end())
                {
                    previousGroup =
                        findGroupByIndex(
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

                HoleContextTraversalPolicy::Context context;
                context.sourceGroupIndex = sourceGroupIndex;
                context.observedGroupIndex = existingObservedGroupIndex;
                context.observedGroupIsNewCandidate = isNewObservedGroup;
                context.stepIndex = stepIndex;
                context.nextDepth = depth + 1;
                context.groups = &groupRegistry.groups();
                context.visitedGroupIndices = &session.visitedGroupIndices;
                context.pendingGroupIndexSet = nullptr;
                context.visitedEdges = &session.visitedEdges;

                const auto traversalDecision =
                    traversalPolicy.decide(context);

                if (traversalDecision.kind ==
                    HoleContextTraversalDecisionKind::Stop)
                {
                    continue;
                }

                observedGroup.note +=
                    " SourceTraceStepIndex=" + std::to_string(step.index) +
                    ", SourceGroupIndex=" + std::to_string(step.sourceGroupIndex) +
                    ", SourcePortIndex=" + std::to_string(step.sourcePortIndex) + ".";

                const int observedGroupIndex =
                    isNewObservedGroup
                        ? groupRegistry.registerOrMerge(std::move(observedGroup))
                        : existingObservedGroupIndex;

                if (observedGroupIndex < 0)
                {
                    continue;
                }

                if (std::find(
                        step.observedGroupIndices.begin(),
                        step.observedGroupIndices.end(),
                        observedGroupIndex) == step.observedGroupIndices.end())
                {
                    step.observedGroupIndices.push_back(observedGroupIndex);
                }

                session.visitedEdges.insert(
                    std::make_pair(
                        sourceGroupIndex,
                        observedGroupIndex));

                if (session.parentGroupIndexByGroupIndex.find(observedGroupIndex) ==
                    session.parentGroupIndexByGroupIndex.end())
                {
                    session.parentGroupIndexByGroupIndex[observedGroupIndex] =
                        sourceGroupIndex;
                }

                exploreDepthFirst(
                    groupRegistry,
                    contextGrouper,
                    tracePortBuilder,
                    traversalPolicy,
                    connectionPolicy,
                    result,
                    session,
                    observedGroupIndex,
                    depth + 1);
            }

            result.contextTraceSteps.push_back(step);
            session.traceStepIndices.push_back(stepIndex);
        }
    }
}
