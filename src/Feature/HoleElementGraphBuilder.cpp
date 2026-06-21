#include "Feature/HoleElementGraphBuilder.h"

#include "Feature/HoleElementBuilder.h"
#include "Geometry/GeometryModel.h"

#include <utility>

namespace OccQtCore::Feature
{
    HoleElementGraph HoleElementGraphBuilder::build(const GeometryModel& model) const
    {
        HoleElementGraph graph;

        HoleElementBuilder elementBuilder;

        auto elements =
            elementBuilder.build(model);

        for (auto& element : elements)
        {
            graph.addElement(
                std::move(element));
        }

        return graph;
    }
}
