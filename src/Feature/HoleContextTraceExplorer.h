#ifndef HOLECONTEXTTRACEEXPLORER_H
#define HOLECONTEXTTRACEEXPLORER_H

#include "Feature/HoleRecognitionModel.h"
#include "Feature/HoleRecognitionWorkingData.h"

#include <set>
#include <vector>
#include <utility>
#include <map>

namespace OccQtCore
{
    class GeometryModel;
}

namespace OccQtCore::Feature
{
    class HoleContextConnectionPolicy;
    class HoleContextGeometryGroupRegistry;
    class HoleContextGeometryGrouper;
    class HoleContextTracePortBuilder;
    class HoleContextTraversalPolicy;

    class HoleContextTraceExplorer
    {
        public:
            explicit HoleContextTraceExplorer(
                const GeometryModel& model,
                HoleRecognitionWorkingData* workingData = nullptr);

            HoleRecognitionResult explore() const;

        private:
            struct TraceWorkState
            {
                int index = -1;
                int startGroupIndex = -1;

                std::set<int> visitedGroupIndices;
                std::set<std::pair<int, int>> visitedEdges;

                std::map<int, int> parentGroupIndexByGroupIndex;

                std::vector<int> reachedGroupIndices;
                std::vector<int> traceStepIndices;
                std::vector<int> tracePortIndices;
            };

        private:
            std::vector<HoleContextGeometryGroup> buildObservedGroups(
                HoleContextGeometryGrouper& contextGrouper,
                const HoleContextTraceStep& step) const;

            void exploreDepthFirst(
                HoleContextGeometryGroupRegistry& groupRegistry,
                HoleContextGeometryGrouper& contextGrouper,
                HoleContextTracePortBuilder& tracePortBuilder,
                HoleContextTraversalPolicy& traversalPolicy,
                HoleContextConnectionPolicy& connectionPolicy,
                HoleRecognitionResult& result,
                TraceWorkState& session,
                int sourceGroupIndex,
                int depth) const;

        private:
            const GeometryModel& m_model;
            HoleRecognitionWorkingData* m_workingData = nullptr;
    };
}

#endif // HOLECONTEXTTRACEEXPLORER_H
