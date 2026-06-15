#ifndef HOLEGEOMETRYTRACEBUILDER_H
#define HOLEGEOMETRYTRACEBUILDER_H

#include "Feature/HoleRecognitionModel.h"

#include <vector>

namespace OccQtCore
{
    class GeometryModel;
}

namespace OccQtCore::Feature
{
    class HoleGeometryTraceBuilder
    {
    public:
        HoleGeometryTraceBuilder(const GeometryModel& model, const std::vector<HoleWall>& walls, const std::vector<HoleWallBoundary>& boundaries);

        std::vector<GeometryTrace> build() const;

    private:
        bool isTraceSource(const HoleWallBoundary& boundary) const;

        GeometryTrace buildTraceFromBoundary(const HoleWallBoundary& boundary, int traceIndex) const;

    private:
        const GeometryModel& m_model;
        const std::vector<HoleWall>& m_walls;
        const std::vector<HoleWallBoundary>& m_boundaries;
    };
}

#endif // HOLEGEOMETRYTRACEBUILDER_H
