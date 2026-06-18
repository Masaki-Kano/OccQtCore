#include "Feature/HoleContextTraversalPolicy.h"

#include "Feature/HoleContextQuery.h"

namespace OccQtCore::Feature
{
    namespace
    {
        namespace HoleContextQuery = OccQtCore::Feature::HoleContextQuery;
        constexpr int MaxContextTraceDepth = 8;
    }

    HoleContextTraversalDecision HoleContextTraversalPolicy::decide(const Context& context) const
    {
        const auto commonDecision = decideCommonStop(context);

        if (commonDecision.kind == HoleContextTraversalDecisionKind::Stop)
        {
            return commonDecision;
        }

        return {HoleContextTraversalDecisionKind::Continue, "common rules passed"};
    }

    HoleContextTraversalDecision HoleContextTraversalPolicy::decideCommonStop(const Context& context) const
    {
        if (context.observedGroupIndex < 0 &&
            !context.observedGroupIsNewCandidate)
        {
            return {
                HoleContextTraversalDecisionKind::Stop,
                "invalid observed group index"
            };
        }

        if (!context.observedGroupIsNewCandidate &&
            context.sourceGroupIndex == context.observedGroupIndex)
        {
            return {HoleContextTraversalDecisionKind::Stop, "self loop"};
        }

        if (context.groups == nullptr)
        {
            return {HoleContextTraversalDecisionKind::Stop, "null group list"};
        }

        if (HoleContextQuery::findGroupByIndex(*context.groups, context.sourceGroupIndex) == nullptr)
        {
            return {
                HoleContextTraversalDecisionKind::Stop,
                "source group index not found in registry"
            };
        }

        if (!context.observedGroupIsNewCandidate)
        {
            if (HoleContextQuery::findGroupByIndex(*context.groups, context.observedGroupIndex) == nullptr)
            {
                return {
                    HoleContextTraversalDecisionKind::Stop,
                    "observed group index not found in registry"
                };
            }
        }

        if (!context.observedGroupIsNewCandidate)
        {
            if (context.visitedGroupIndices != nullptr &&
                context.visitedGroupIndices->find(context.observedGroupIndex) !=
                    context.visitedGroupIndices->end())
            {
                return {
                    HoleContextTraversalDecisionKind::Stop,
                    "observed group already visited"
                };
            }
        }

        if (!context.observedGroupIsNewCandidate)
        {
            if (context.pendingGroupIndexSet != nullptr &&
                context.pendingGroupIndexSet->find(context.observedGroupIndex) !=
                    context.pendingGroupIndexSet->end())
            {
                return {
                    HoleContextTraversalDecisionKind::Stop,
                    "observed group already pending"
                };
            }
        }

        if (context.nextDepth >= MaxContextTraceDepth)
        {
            return {
                HoleContextTraversalDecisionKind::Stop,
                "max depth reached"
            };
        }

        if (!context.observedGroupIsNewCandidate)
        {
            const auto edge =
                std::make_pair(
                    context.sourceGroupIndex,
                    context.observedGroupIndex);

            if (context.visitedEdges != nullptr &&
                context.visitedEdges->find(edge) !=
                    context.visitedEdges->end())
            {
                return {
                    HoleContextTraversalDecisionKind::Stop,
                    "edge already visited"
                };
            }
        }

        return {
            HoleContextTraversalDecisionKind::Continue,
            "common rules passed"
        };
    }
}
