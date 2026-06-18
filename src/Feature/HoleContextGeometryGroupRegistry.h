#ifndef HOLECONTEXTGEOMETRYGROUPREGISTRY_H
#define HOLECONTEXTGEOMETRYGROUPREGISTRY_H

#include "Feature/HoleRecognitionModel.h"

#include <vector>

namespace OccQtCore
{
class GeometryModel;
}

namespace OccQtCore::Feature
{
    class HoleContextGeometryGroupRegistry
    {
        public:
            explicit HoleContextGeometryGroupRegistry(
                const GeometryModel& model);

            HoleContextGeometryGroupRegistry(
                const GeometryModel& model,
                std::vector<HoleContextGeometryGroup> initialGroups);

            int registerOrMerge(
                HoleContextGeometryGroup group);

            int findEquivalentGroupIndex(
                const HoleContextGeometryGroup& group) const;

            const std::vector<HoleContextGeometryGroup>& groups() const;

            bool isSameGroup(
                const HoleContextGeometryGroup& existingGroup,
                const HoleContextGeometryGroup& candidateGroup) const;

        private:
            int findSameGroupIndex(
                const HoleContextGeometryGroup& group) const;

            bool isSameFaceSet(
                std::vector<int> lhs,
                std::vector<int> rhs) const;

            bool isFaceSubset(
                std::vector<int> subset,
                std::vector<int> superset) const;

        private:
            const GeometryModel& m_model;
            std::vector<HoleContextGeometryGroup> m_groups;
    };
}

#endif // HOLECONTEXTGEOMETRYGROUPREGISTRY_H
