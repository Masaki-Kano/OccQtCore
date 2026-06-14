#ifndef HOLEWALLBOUNDARYBUILDER_H
#define HOLEWALLBOUNDARYBUILDER_H

#include <vector>

#include "Feature/HoleRecognitionModel.h"

namespace OccQtCore
{
    class GeometryModel;
}

namespace OccQtCore::Feature
{
    class HoleWallBoundaryBuilder
    {
    public:
        std::vector<HoleWallBoundary> build(const GeometryModel& model, const std::vector<HoleWall>& walls) const;

    private:
        void collectBoundariesOfWall(const GeometryModel& model, const HoleWall& wall, std::vector<HoleWallBoundary>& boundaries) const;

        HoleWallBoundary buildBoundaryFromEdge(
            const GeometryModel& model,
            const HoleWall& wall,
            int edgeIndex,
            int boundaryIndex) const;

        HoleWallBoundaryKind classifyBoundaryKind(
            const HoleWall& wall,
            const HoleWallBoundary& boundary) const;

        HoleWallBoundaryTraceStatus determineTraceStatus(HoleWallBoundaryKind kind) const;

        bool isFaceInWall(const HoleWall& wall, int faceIndex) const;

        GeometryRefs collectAdjacentGeometryRefs(const GeometryModel& model, const HoleWall& wall, int edgeIndex) const;

        void fillAxialRange(const GeometryModel& model, const HoleWall& wall, HoleWallBoundary& boundary) const;

        bool hasOutsideFace(const HoleWall& wall, const GeometryRefs& adjacentRefs) const;

        std::vector<HoleWallBoundary> mergeAxialEndBoundaries(
            const std::vector<HoleWallBoundary>& boundaries) const;

        bool canMergeAxialEndBoundary(
            const HoleWallBoundary& lhs,
            const HoleWallBoundary& rhs) const;

        void mergeBoundary(
            HoleWallBoundary& target,
            const HoleWallBoundary& source) const;
    };
}

#endif // HOLEWALLBOUNDARYBUILDER_H
