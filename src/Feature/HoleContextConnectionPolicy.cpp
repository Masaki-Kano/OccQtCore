#include "Feature/HoleContextConnectionPolicy.h"

namespace OccQtCore::Feature
{
    namespace
    {
        constexpr double AxisParallelTolerance = 1.0e-6;
        constexpr double AxisDistanceTolerance = 1.0e-3;
    }

        HoleContextTraversalDecision HoleContextConnectionPolicy::decide(
            const Context& context) const
        {
            using Kind = HoleContextGeometryGroupKind;

            if (context.sourceGroup == nullptr ||
                context.observedGroup == nullptr)
            {
                return {
                    HoleContextTraversalDecisionKind::Stop,
                    "connection policy requires source and observed groups"
                };
            }

            const auto& sourceGroup =
                *context.sourceGroup;

            const auto& observedGroup =
                *context.observedGroup;

            if (sourceGroup.kind == Kind::WallCandidate &&
                observedGroup.kind == Kind::BoundaryCandidate)
            {
                return {
                    HoleContextTraversalDecisionKind::Continue,
                    "wall to boundary connection"
                };
            }

            if (sourceGroup.kind == Kind::BoundaryCandidate &&
                observedGroup.kind == Kind::WallCandidate)
            {
                if (context.previousGroup == nullptr)
                {
                    return {
                        HoleContextTraversalDecisionKind::Stop,
                        "boundary to wall requires previous group"
                    };
                }

                const auto& previousGroup =
                    *context.previousGroup;

                if (previousGroup.kind != Kind::WallCandidate)
                {
                    return {
                        HoleContextTraversalDecisionKind::Stop,
                        "boundary to wall requires previous wall group"
                    };
                }

                if (!isSameAxisLine(previousGroup, observedGroup))
                {
                    return {
                        HoleContextTraversalDecisionKind::Stop,
                        "boundary to wall rejected because previous wall and observed wall are not coaxial"
                    };
                }

                return {
                    HoleContextTraversalDecisionKind::Continue,
                    "boundary to coaxial wall connection"
                };
            }

            if (sourceGroup.kind == Kind::BoundaryCandidate &&
                observedGroup.kind == Kind::BoundaryCandidate)
            {
                return {
                    HoleContextTraversalDecisionKind::Stop,
                    "boundary to boundary connection is not a context continuation"
                };
            }

            if (sourceGroup.kind == Kind::WallCandidate &&
                observedGroup.kind == Kind::WallCandidate)
            {
                return {
                    HoleContextTraversalDecisionKind::Continue,
                    "wall to wall connection"
                };
            }

            return {
                HoleContextTraversalDecisionKind::Stop,
                "unsupported context connection"
            };
        }

    const HoleContextGeometryGroup* HoleContextConnectionPolicy::findGroupByIndex(
        const std::vector<HoleContextGeometryGroup>& groups,
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

    bool HoleContextConnectionPolicy::isSameAxisLine(
        const HoleContextGeometryGroup& lhs,
        const HoleContextGeometryGroup& rhs) const
    {
        if (!lhs.hasReferenceDirection ||
            !rhs.hasReferenceDirection)
        {
            return false;
        }

        const gp_Dir& lhsDirection = lhs.referenceDirection;
        const gp_Dir& rhsDirection = rhs.referenceDirection;

        const double dot =
            std::abs(lhsDirection.Dot(rhsDirection));

        if (std::abs(1.0 - dot) > AxisParallelTolerance)
        {
            return false;
        }

        const gp_Vec lhsToRhs(
            lhs.referencePoint,
            rhs.referencePoint);

        const gp_Vec lhsAxisVector(lhsDirection);

        const double distance =
            lhsToRhs.Crossed(lhsAxisVector).Magnitude();

        return distance <= AxisDistanceTolerance;
    }
}
