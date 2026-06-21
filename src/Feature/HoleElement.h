#ifndef HOLEELEMENT_H
#define HOLEELEMENT_H

#include <vector>

namespace OccQtCore::Feature
{
    enum class HoleElementGeometryKind
    {
        Unknown,
        Cylindrical,
        Planar,
        Conical,
        Toroidal,
        Spherical,
        Freeform
    };

    enum class HoleElementKind
    {
        Unknown,
        Wall,
        Boundary,
        Transition
    };

    struct HoleElement
    {
        int index = -1;

        // どの境界条件でまとめられた要素か
        HoleElementGeometryKind geometryKind = HoleElementGeometryKind::Unknown;

        // 穴の中での意味、これはあとから決める
        HoleElementKind kind = HoleElementKind::Unknown;

        std::vector<int> faceIndices;
    };
}

#endif // HOLEELEMENT_H
