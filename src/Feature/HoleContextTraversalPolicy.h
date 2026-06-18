#ifndef HOLECONTEXTTRAVERSALPOLICY_H
#define HOLECONTEXTTRAVERSALPOLICY_H

#include "Feature/HoleRecognitionModel.h"

#include <set>
#include <string>
#include <utility>
#include <vector>

namespace OccQtCore::Feature
{
    enum class HoleContextTraversalDecisionKind
    {
        Continue,
        Stop
    };

    struct HoleContextTraversalDecision
    {
        HoleContextTraversalDecisionKind kind = HoleContextTraversalDecisionKind::Stop;
        std::string reason;
    };

    class HoleContextTraversalPolicy
    {
    public:
        struct Context
        {
            int sourceGroupIndex = -1;
            int observedGroupIndex = -1;

            bool observedGroupIsNewCandidate = false;

            int stepIndex = -1;
            int nextDepth = 0;

            const std::vector<HoleContextGeometryGroup>* groups = nullptr;
            const std::set<int>* visitedGroupIndices = nullptr;
            const std::set<int>* pendingGroupIndexSet = nullptr;
            const std::set<std::pair<int, int>>* visitedEdges = nullptr;
        };

    public:
        HoleContextTraversalDecision decide(const Context& context) const;

    private:
        HoleContextTraversalDecision decideCommonStop(const Context& context) const;

        const HoleContextGeometryGroup* findGroupByIndex(const std::vector<HoleContextGeometryGroup>& groups, int groupIndex) const;

        bool isEmptyGeometryRefs(const GeometryRefs& refs) const;
    };
}

#endif // HOLECONTEXTTRAVERSALPOLICY_H
