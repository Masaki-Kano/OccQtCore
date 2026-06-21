#ifndef HOLEELEMENTBUILDER_H
#define HOLEELEMENTBUILDER_H

#include <vector>

#include "Feature/HoleElement.h"

namespace OccQtCore
{
    class GeometryModel;

    namespace Feature
    {
        class HoleElementBuilder
        {
        public:
            std::vector<HoleElement> build(const GeometryModel& model) const;

        private:
            std::vector<int> collectCylinderFaceIndices(const GeometryModel& model) const;

            std::vector<std::vector<int>> buildCylinderComponents(const GeometryModel& model, const std::vector<int>& cylinderFaceIndices) const;

            bool areConnectedCylinderFaces(const GeometryModel& model, int lhsFaceIndex, int rhsFaceIndex) const;
        };
    }
}

#endif // HOLEELEMENTBUILDER_H
