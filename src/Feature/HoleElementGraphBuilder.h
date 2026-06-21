#ifndef HOLEELEMENTGRAPHBUILDER_H
#define HOLEELEMENTGRAPHBUILDER_H

#include "Feature/HoleElementGraph.h"

namespace OccQtCore
{
    class GeometryModel;

    namespace Feature
    {
        class HoleElementGraphBuilder
        {
        public:
            HoleElementGraph build(const GeometryModel& model) const;
        };
    }
}

#endif // HOLEELEMENTGRAPHBUILDER_H
