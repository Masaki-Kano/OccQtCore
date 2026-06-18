#include "Feature/HoleContextQuery.h"

#include "Core/CollectionUtil.h"

#include <algorithm>

namespace OccQtCore::Feature::HoleContextQuery
{
    // ------------------------------------------------------------
    // Index lookup
    // ------------------------------------------------------------

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

    const HoleContextTraceRun* findRunByIndex(
        const std::vector<HoleContextTraceRun>& runs,
        int runIndex)
    {
        const auto it =
            std::find_if(
                runs.begin(),
                runs.end(),
                [runIndex](const HoleContextTraceRun& run)
                {
                    return run.index == runIndex;
                });

        if (it == runs.end())
        {
            return nullptr;
        }

        return &(*it);
    }

    const HoleContextTraceStep* findStepByIndex(
        const std::vector<HoleContextTraceStep>& steps,
        int stepIndex)
    {
        const auto it =
            std::find_if(
                steps.begin(),
                steps.end(),
                [stepIndex](const HoleContextTraceStep& step)
                {
                    return step.index == stepIndex;
                });

        if (it == steps.end())
        {
            return nullptr;
        }

        return &(*it);
    }

    const HoleContextTracePort* findPortByIndex(
        const std::vector<HoleContextTracePort>& ports,
        int portIndex)
    {
        const auto it =
            std::find_if(
                ports.begin(),
                ports.end(),
                [portIndex](const HoleContextTracePort& port)
                {
                    return port.index == portIndex;
                });

        if (it == ports.end())
        {
            return nullptr;
        }

        return &(*it);
    }

    const HoleContextGeometryGroup* findGroupByIndex(
        const HoleRecognitionResult& result,
        int groupIndex)
    {
        return findGroupByIndex(
            result.contextGeometryGroups,
            groupIndex);
    }

    const HoleContextTraceRun* findRunByIndex(
        const HoleRecognitionResult& result,
        int runIndex)
    {
        return findRunByIndex(
            result.contextTrace.runs,
            runIndex);
    }

    const HoleContextTraceStep* findStepByIndex(
        const HoleRecognitionResult& result,
        int stepIndex)
    {
        return findStepByIndex(
            result.contextTrace.steps,
            stepIndex);
    }

    const HoleContextTracePort* findPortByIndex(
        const HoleRecognitionResult& result,
        int portIndex)
    {
        return findPortByIndex(
            result.contextTrace.ports,
            portIndex);
    }

    // ------------------------------------------------------------
    // Geometry reference query
    // ------------------------------------------------------------

    bool groupContainsFace(
        const HoleContextGeometryGroup& group,
        int faceIndex)
    {
        return CollectionUtil::contains(
            group.geometryRefs.faceIndices,
            faceIndex);
    }

    bool portReferencesFace(
        const HoleContextTracePort& port,
        int faceIndex)
    {
        return CollectionUtil::contains(
            port.geometryRefs.faceIndices,
            faceIndex);
    }

    bool portReferencesEdge(
        const HoleContextTracePort& port,
        int edgeIndex)
    {
        return CollectionUtil::contains(
            port.geometryRefs.edgeIndices,
            edgeIndex);
    }

    bool stepOutsideReferencesFace(
        const HoleContextTraceStep& step,
        int faceIndex)
    {
        return CollectionUtil::contains(
            step.outsideGeometryRefs.faceIndices,
            faceIndex);
    }

    bool stepPortReferencesFace(
        const HoleContextTraceStep& step,
        int faceIndex)
    {
        return CollectionUtil::contains(
            step.portGeometryRefs.faceIndices,
            faceIndex);
    }

    bool stepReferencesFace(
        const HoleContextTraceStep& step,
        int faceIndex)
    {
        return stepOutsideReferencesFace(step, faceIndex)
        || stepPortReferencesFace(step, faceIndex);
    }

    // ------------------------------------------------------------
    // Reverse lookup
    // ------------------------------------------------------------

    int findRunIndexByStepIndex(
        const HoleRecognitionResult& result,
        int stepIndex)
    {
        for (const auto& run : result.contextTrace.runs)
        {
            if (CollectionUtil::contains(
                    run.traceStepIndices,
                    stepIndex))
            {
                return run.index;
            }
        }

        return -1;
    }

    int findRunIndexByPortIndex(
        const HoleRecognitionResult& result,
        int portIndex)
    {
        for (const auto& run : result.contextTrace.runs)
        {
            if (CollectionUtil::contains(
                    run.tracePortIndices,
                    portIndex))
            {
                return run.index;
            }
        }

        return -1;
    }

    std::vector<int> findGroupIndicesContainingFace(
        const HoleRecognitionResult& result,
        int faceIndex)
    {
        std::vector<int> groupIndices;

        for (const auto& group : result.contextGeometryGroups)
        {
            if (!groupContainsFace(group, faceIndex))
            {
                continue;
            }

            CollectionUtil::addUnique(
                groupIndices,
                group.index);
        }

        return groupIndices;
    }

    std::vector<int> findStepIndicesReferencingFace(
        const HoleRecognitionResult& result,
        int faceIndex)
    {
        std::vector<int> stepIndices;

        for (const auto& step : result.contextTrace.steps)
        {
            if (!stepReferencesFace(step, faceIndex))
            {
                continue;
            }

            CollectionUtil::addUnique(
                stepIndices,
                step.index);
        }

        return stepIndices;
    }

    std::vector<int> findPortIndicesReferencingFace(
        const HoleRecognitionResult& result,
        int faceIndex)
    {
        std::vector<int> portIndices;

        for (const auto& port : result.contextTrace.ports)
        {
            if (!portReferencesFace(port, faceIndex))
            {
                continue;
            }

            CollectionUtil::addUnique(
                portIndices,
                port.index);
        }

        return portIndices;
    }

    std::vector<int> findPortIndicesReferencingEdge(
        const HoleRecognitionResult& result,
        int edgeIndex)
    {
        std::vector<int> portIndices;

        for (const auto& port : result.contextTrace.ports)
        {
            if (!portReferencesEdge(port, edgeIndex))
            {
                continue;
            }

            CollectionUtil::addUnique(
                portIndices,
                port.index);
        }

        return portIndices;
    }

    std::vector<int> findRunIndicesReachingGroup(
        const HoleRecognitionResult& result,
        int groupIndex)
    {
        std::vector<int> runIndices;

        for (const auto& run : result.contextTrace.runs)
        {
            if (!CollectionUtil::contains(
                    run.reachedGroupIndices,
                    groupIndex))
            {
                continue;
            }

            CollectionUtil::addUnique(
                runIndices,
                run.index);
        }

        return runIndices;
    }

    std::vector<int> findRunIndicesRelatedToFace(
        const HoleRecognitionResult& result,
        int faceIndex)
    {
        std::vector<int> runIndices;

        const auto groupIndices =
            findGroupIndicesContainingFace(
                result,
                faceIndex);

        for (const int groupIndex : groupIndices)
        {
            const auto relatedRunIndices =
                findRunIndicesReachingGroup(
                    result,
                    groupIndex);

            for (const int runIndex : relatedRunIndices)
            {
                CollectionUtil::addUnique(
                    runIndices,
                    runIndex);
            }
        }

        return runIndices;
    }
}
