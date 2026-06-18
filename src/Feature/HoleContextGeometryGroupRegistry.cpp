#include "Feature/HoleContextGeometryGroupRegistry.h"

#include <algorithm>
#include <utility>

namespace OccQtCore::Feature
{
    HoleContextGeometryGroupRegistry::HoleContextGeometryGroupRegistry(
        const GeometryModel& model)
        : m_model(model)
    {
    }

    HoleContextGeometryGroupRegistry::HoleContextGeometryGroupRegistry(
        const GeometryModel& model,
        std::vector<HoleContextGeometryGroup> initialGroups)
        : m_model(model)
        , m_groups(std::move(initialGroups))
    {
        for (int i = 0; i < static_cast<int>(m_groups.size()); ++i)
        {
            m_groups[i].index = i;
        }
    }

    int HoleContextGeometryGroupRegistry::registerOrMerge(
        HoleContextGeometryGroup group)
    {
        const int sameGroupIndex =
            findSameGroupIndex(group);

        if (sameGroupIndex >= 0)
        {
            return sameGroupIndex;
        }

        group.index =
            static_cast<int>(m_groups.size());

        m_groups.push_back(std::move(group));

        return static_cast<int>(m_groups.size()) - 1;
    }

    int HoleContextGeometryGroupRegistry::findEquivalentGroupIndex(
        const HoleContextGeometryGroup& group) const
    {
        return findSameGroupIndex(group);
    }

    const std::vector<HoleContextGeometryGroup>&
    HoleContextGeometryGroupRegistry::groups() const
    {
        return m_groups;
    }

    int HoleContextGeometryGroupRegistry::findSameGroupIndex(
        const HoleContextGeometryGroup& group) const
    {
        for (const auto& existingGroup : m_groups)
        {
            if (isSameGroup(existingGroup, group))
            {
                return existingGroup.index;
            }
        }

        return -1;
    }

    bool HoleContextGeometryGroupRegistry::isSameGroup(
        const HoleContextGeometryGroup& existingGroup,
        const HoleContextGeometryGroup& candidateGroup) const
    {
        if (existingGroup.kind != candidateGroup.kind)
        {
            return false;
        }

        if (isSameFaceSet(
                existingGroup.geometryRefs.faceIndices,
                candidateGroup.geometryRefs.faceIndices))
        {
            return true;
        }

        if (existingGroup.kind == HoleContextGeometryGroupKind::WallCandidate)
        {
            return isFaceSubset(
                       candidateGroup.geometryRefs.faceIndices,
                       existingGroup.geometryRefs.faceIndices)
                   || isFaceSubset(
                       existingGroup.geometryRefs.faceIndices,
                       candidateGroup.geometryRefs.faceIndices);
        }

        return false;
    }

    bool HoleContextGeometryGroupRegistry::isSameFaceSet(
        std::vector<int> lhs,
        std::vector<int> rhs) const
    {
        std::sort(lhs.begin(), lhs.end());
        lhs.erase(
            std::unique(lhs.begin(), lhs.end()),
            lhs.end());

        std::sort(rhs.begin(), rhs.end());
        rhs.erase(
            std::unique(rhs.begin(), rhs.end()),
            rhs.end());

        return lhs == rhs;
    }

    bool HoleContextGeometryGroupRegistry::isFaceSubset(
        std::vector<int> subset,
        std::vector<int> superset) const
    {
        std::sort(subset.begin(), subset.end());
        subset.erase(
            std::unique(subset.begin(), subset.end()),
            subset.end());

        std::sort(superset.begin(), superset.end());
        superset.erase(
            std::unique(superset.begin(), superset.end()),
            superset.end());

        return std::includes(
            superset.begin(),
            superset.end(),
            subset.begin(),
            subset.end());
    }
}
