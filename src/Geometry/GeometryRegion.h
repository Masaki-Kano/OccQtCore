#ifndef GEOMETRYREGION_H
#define GEOMETRYREGION_H

#include <vector>

#include "Geometry/GeometryTypes.h"

namespace OccQtCore
{
    struct GeometryRegion
    {
        int index = -1;

        SurfaceKind surfaceKind = SurfaceKind::Unknown;

        std::vector<int> faceIndices;
    };
}

#endif // GEOMETRYREGION_H
