#ifndef HOLECONTEXTQUERY_H
#define HOLECONTEXTQUERY_H

#include "Feature/HoleRecognitionModel.h"

#include <vector>

namespace OccQtCore::Feature::HoleContextQuery
{
    // ------------------------------------------------------------
    // Index lookup
    // ------------------------------------------------------------

    const HoleContextGeometryGroup* findGroupByIndex(
        const std::vector<HoleContextGeometryGroup>& groups,
        int groupIndex);

    const HoleContextTraceRun* findRunByIndex(
        const std::vector<HoleContextTraceRun>& runs,
        int runIndex);

    const HoleContextTraceStep* findStepByIndex(
        const std::vector<HoleContextTraceStep>& steps,
        int stepIndex);

    const HoleContextTracePort* findPortByIndex(
        const std::vector<HoleContextTracePort>& ports,
        int portIndex);

    const HoleContextGeometryGroup* findGroupByIndex(
        const HoleRecognitionResult& result,
        int groupIndex);

    const HoleContextTraceRun* findRunByIndex(
        const HoleRecognitionResult& result,
        int runIndex);

    const HoleContextTraceStep* findStepByIndex(
        const HoleRecognitionResult& result,
        int stepIndex);

    const HoleContextTracePort* findPortByIndex(
        const HoleRecognitionResult& result,
        int portIndex);

    // ------------------------------------------------------------
    // Geometry reference query
    // ------------------------------------------------------------

    bool groupContainsFace(
        const HoleContextGeometryGroup& group,
        int faceIndex);

    bool portReferencesFace(
        const HoleContextTracePort& port,
        int faceIndex);

    bool portReferencesEdge(
        const HoleContextTracePort& port,
        int edgeIndex);

    bool stepOutsideReferencesFace(
        const HoleContextTraceStep& step,
        int faceIndex);

    bool stepPortReferencesFace(
        const HoleContextTraceStep& step,
        int faceIndex);

    bool stepReferencesFace(
        const HoleContextTraceStep& step,
        int faceIndex);

    // ------------------------------------------------------------
    // Reverse lookup
    // ------------------------------------------------------------

    int findRunIndexByStepIndex(
        const HoleRecognitionResult& result,
        int stepIndex);

    int findRunIndexByPortIndex(
        const HoleRecognitionResult& result,
        int portIndex);

    std::vector<int> findGroupIndicesContainingFace(
        const HoleRecognitionResult& result,
        int faceIndex);

    std::vector<int> findStepIndicesReferencingFace(
        const HoleRecognitionResult& result,
        int faceIndex);

    std::vector<int> findPortIndicesReferencingFace(
        const HoleRecognitionResult& result,
        int faceIndex);

    std::vector<int> findPortIndicesReferencingEdge(
        const HoleRecognitionResult& result,
        int edgeIndex);

    std::vector<int> findRunIndicesReachingGroup(
        const HoleRecognitionResult& result,
        int groupIndex);

    std::vector<int> findRunIndicesRelatedToFace(
        const HoleRecognitionResult& result,
        int faceIndex);
}

#endif // HOLECONTEXTQUERY_H
