#ifndef SURFACEQUERY_H
#define SURFACEQUERY_H

namespace OccQtCore
{
    class GeometryModel;

    namespace SurfaceQuery
    {
        bool areOnSameCylinder(
            const GeometryModel& model,
            int lhsFaceIndex,
            int rhsFaceIndex);
    }
}

#endif // SURFACEQUERY_H
