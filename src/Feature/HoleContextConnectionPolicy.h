#ifndef HOLECONTEXTCONNECTIONPOLICY_H
#define HOLECONTEXTCONNECTIONPOLICY_H

#include "Feature/HoleRecognitionModel.h"
#include "Feature/HoleContextTraversalPolicy.h"

#include <vector>

namespace OccQtCore::Feature
{
    class HoleContextConnectionPolicy
    {
    public:
        struct Context
        {
            const HoleContextGeometryGroup* previousGroup = nullptr;
            const HoleContextGeometryGroup* sourceGroup = nullptr;
            const HoleContextGeometryGroup* observedGroup = nullptr;
        };

    public:
        HoleContextTraversalDecision decide(
            const Context& context) const;

    private:
        const HoleContextGeometryGroup* findGroupByIndex(
            const std::vector<HoleContextGeometryGroup>& groups,
            int groupIndex) const;

        bool isSameAxisLine(
            const HoleContextGeometryGroup& lhs,
            const HoleContextGeometryGroup& rhs) const;
    };
}

#endif // HOLECONTEXTCONNECTIONPOLICY_H
