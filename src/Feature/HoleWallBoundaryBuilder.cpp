#include "Feature/HoleWallBoundaryBuilder.h"

#include "Core/CollectionUtil.h"
#include "Geometry/GeometryModel.h"
#include "Geometry/TopologyQuery.h"
#include "Geometry/SurfaceUtil.h"

#include <algorithm>
#include <cmath>
#include <QtGlobal>

namespace
{
    constexpr double AxialSpanTolerance = 1.0e-4;
    constexpr double AxialPositionTolerance = 1.0e-3;
}

namespace OccQtCore::Feature
{
    std::vector<HoleWallBoundary> HoleWallBoundaryBuilder::build(const GeometryModel& model, const std::vector<HoleWall>& walls) const
    {
        std::vector<HoleWallBoundary> boundaries;

        for (const auto& wall : walls)
        {
            collectBoundariesOfWall(model, wall, boundaries);
        }

        return boundaries;
    }

    void HoleWallBoundaryBuilder::collectBoundariesOfWall(const GeometryModel& model, const HoleWall& wall, std::vector<HoleWallBoundary>& boundaries) const
    {
        std::vector<int> edgeIndices;

        for (const int faceIndex : wall.geometryRefs.faceIndices)
        {
            const auto faceEdges = TopologyQuery::edgesOfFace(model, faceIndex);

            for (const int edgeIndex : faceEdges)
            {
                CollectionUtil::addUnique(edgeIndices, edgeIndex);
            }
        }

        CollectionUtil::sortUnique(edgeIndices);

        std::vector<HoleWallBoundary> localBoundaries;

        for (const int edgeIndex : edgeIndices)
        {
            const int localBoundaryIndex =
                static_cast<int>(localBoundaries.size());

            auto boundary =
                buildBoundaryFromEdge(
                    model,
                    wall,
                    edgeIndex,
                    localBoundaryIndex);

            localBoundaries.push_back(boundary);
        }

        const auto mergedBoundaries =
            mergeAxialEndBoundaries(localBoundaries);

        for (auto boundary : mergedBoundaries)
        {
            boundary.index = static_cast<int>(boundaries.size());
            boundary.wallIndex = wall.index;

            boundaries.push_back(boundary);
        }
    }

    HoleWallBoundary HoleWallBoundaryBuilder::buildBoundaryFromEdge(const GeometryModel& model, const HoleWall& wall, int edgeIndex, int boundaryIndex) const
    {
        HoleWallBoundary boundary;

        boundary.index = boundaryIndex;
        boundary.wallIndex = wall.index;

        boundary.geometryRefs.edgeIndices.push_back(edgeIndex);

        boundary.adjacentGeometryRefs =
            collectAdjacentGeometryRefs(model, wall, edgeIndex);

        fillAxialRange(model, wall, boundary);

        boundary.kind = classifyBoundaryKind(wall, boundary);

        boundary.traceStatus = determineTraceStatus(boundary.kind);

        boundary.circumferentialCoverage = 0.0;

        return boundary;
    }

    HoleWallBoundaryKind HoleWallBoundaryBuilder::classifyBoundaryKind(
        const HoleWall& wall,
        const HoleWallBoundary& boundary) const
    {
        const auto& adjacentRefs = boundary.adjacentGeometryRefs;

        if (adjacentRefs.faceIndices.empty())
        {
            return HoleWallBoundaryKind::AxialEnd;
        }

        if (!hasOutsideFace(wall, adjacentRefs))
        {
            return HoleWallBoundaryKind::InternalWallSplit;
        }

        const double axialSpan =
            std::abs(boundary.axialMax - boundary.axialMin);

        const bool isSmallAxialSpan =
            axialSpan < AxialSpanTolerance;

        const bool isNearWallMin =
            std::abs(boundary.axialPosition - wall.axialMin) <
            AxialPositionTolerance;

        const bool isNearWallMax =
            std::abs(boundary.axialPosition - wall.axialMax) <
            AxialPositionTolerance;

        if (isSmallAxialSpan && (isNearWallMin || isNearWallMax))
        {
            return HoleWallBoundaryKind::AxialEnd;
        }

        if (isSmallAxialSpan)
        {
            return HoleWallBoundaryKind::LateralConnection;
        }

        return HoleWallBoundaryKind::Ambiguous;
    }

    HoleWallBoundaryTraceStatus HoleWallBoundaryBuilder::determineTraceStatus(HoleWallBoundaryKind kind) const
    {
        switch (kind)
        {
        case HoleWallBoundaryKind::AxialEnd:
        case HoleWallBoundaryKind::LateralConnection:
            return HoleWallBoundaryTraceStatus::Traceable;

        case HoleWallBoundaryKind::InternalWallSplit:
            return HoleWallBoundaryTraceStatus::Ignored;

        case HoleWallBoundaryKind::Broken:
        case HoleWallBoundaryKind::Ambiguous:
            return HoleWallBoundaryTraceStatus::NotTraceable;

        case HoleWallBoundaryKind::Unknown:
        default:
            return HoleWallBoundaryTraceStatus::Unknown;
        }
    }

    bool HoleWallBoundaryBuilder::isFaceInWall(const HoleWall& wall, int faceIndex) const
    {
        return CollectionUtil::contains(wall.geometryRefs.faceIndices, faceIndex);
    }

    GeometryRefs HoleWallBoundaryBuilder::collectAdjacentGeometryRefs(
        const GeometryModel& model,
        const HoleWall& wall,
        int edgeIndex) const
    {
        GeometryRefs refs;

        const auto adjacentFaces =
            TopologyQuery::adjacentFacesOfEdge(model, edgeIndex);

        for (const int faceIndex : adjacentFaces)
        {
            // Wall内部Faceも一旦入れる。
            // classify側で「全部Wall内部ならInternalWallSplit」と判定する。
            CollectionUtil::addUnique(refs.faceIndices, faceIndex);
        }

        CollectionUtil::sortUnique(refs.faceIndices);

        Q_UNUSED(wall);

        return refs;
    }

    void HoleWallBoundaryBuilder::fillAxialRange(
        const GeometryModel& model,
        const HoleWall& wall,
        HoleWallBoundary& boundary) const
    {
        std::vector<double> axialPositions;

        for (const int edgeIndex : boundary.geometryRefs.edgeIndices)
        {
            const auto& vertexIndices =
                model.graph().verticesOfEdge(edgeIndex);

            for (const int vertexIndex : vertexIndices)
            {
                CollectionUtil::addUnique(
                    boundary.geometryRefs.vertexIndices,
                    vertexIndex);

                const auto& vertex = model.vertexAt(vertexIndex);

                const double axialPosition =
                    SurfaceUtil::projectPointToAxis(
                        wall.axisPoint,
                        wall.axisDirection,
                        vertex->info.point);

                axialPositions.push_back(axialPosition);
            }
        }

        CollectionUtil::sortUnique(boundary.geometryRefs.vertexIndices);

        if (axialPositions.empty())
        {
            boundary.axialMin = 0.0;
            boundary.axialMax = 0.0;
            boundary.axialPosition = 0.0;
            return;
        }

        const auto minMax =
            std::minmax_element(axialPositions.begin(), axialPositions.end());

        boundary.axialMin = *minMax.first;
        boundary.axialMax = *minMax.second;
        boundary.axialPosition =
            0.5 * (boundary.axialMin + boundary.axialMax);
    }

    bool HoleWallBoundaryBuilder::hasOutsideFace(
        const HoleWall& wall,
        const GeometryRefs& adjacentRefs) const
    {
        for (const int adjacentFaceIndex : adjacentRefs.faceIndices)
        {
            if (!isFaceInWall(wall, adjacentFaceIndex))
            {
                return true;
            }
        }

        return false;
    }

    std::vector<HoleWallBoundary> HoleWallBoundaryBuilder::mergeAxialEndBoundaries(
        const std::vector<HoleWallBoundary>& boundaries) const
    {
        std::vector<HoleWallBoundary> mergedBoundaries;

        for (const auto& boundary : boundaries)
        {
            bool merged = false;

            for (auto& existing : mergedBoundaries)
            {
                if (canMergeAxialEndBoundary(existing, boundary))
                {
                    mergeBoundary(existing, boundary);
                    merged = true;
                    break;
                }
            }

            if (!merged)
            {
                mergedBoundaries.push_back(boundary);
            }
        }

        return mergedBoundaries;
    }

    bool HoleWallBoundaryBuilder::canMergeAxialEndBoundary(
        const HoleWallBoundary& lhs,
        const HoleWallBoundary& rhs) const
    {
        if (lhs.wallIndex != rhs.wallIndex)
        {
            return false;
        }

        if (lhs.kind != HoleWallBoundaryKind::AxialEnd ||
            rhs.kind != HoleWallBoundaryKind::AxialEnd)
        {
            return false;
        }

        if (lhs.traceStatus != rhs.traceStatus)
        {
            return false;
        }

        if (std::abs(lhs.axialPosition - rhs.axialPosition) >
            AxialPositionTolerance)
        {
            return false;
        }

        return true;
    }

    void HoleWallBoundaryBuilder::mergeBoundary(
        HoleWallBoundary& target,
        const HoleWallBoundary& source) const
    {
        for (const int faceIndex : source.geometryRefs.faceIndices)
        {
            CollectionUtil::addUnique(target.geometryRefs.faceIndices, faceIndex);
        }

        for (const int wireIndex : source.geometryRefs.wireIndices)
        {
            CollectionUtil::addUnique(target.geometryRefs.wireIndices, wireIndex);
        }

        for (const int edgeIndex : source.geometryRefs.edgeIndices)
        {
            CollectionUtil::addUnique(target.geometryRefs.edgeIndices, edgeIndex);
        }

        for (const int vertexIndex : source.geometryRefs.vertexIndices)
        {
            CollectionUtil::addUnique(target.geometryRefs.vertexIndices, vertexIndex);
        }

        for (const int faceIndex : source.adjacentGeometryRefs.faceIndices)
        {
            CollectionUtil::addUnique(target.adjacentGeometryRefs.faceIndices, faceIndex);
        }

        for (const int wireIndex : source.adjacentGeometryRefs.wireIndices)
        {
            CollectionUtil::addUnique(target.adjacentGeometryRefs.wireIndices, wireIndex);
        }

        for (const int edgeIndex : source.adjacentGeometryRefs.edgeIndices)
        {
            CollectionUtil::addUnique(target.adjacentGeometryRefs.edgeIndices, edgeIndex);
        }

        for (const int vertexIndex : source.adjacentGeometryRefs.vertexIndices)
        {
            CollectionUtil::addUnique(target.adjacentGeometryRefs.vertexIndices, vertexIndex);
        }

        CollectionUtil::sortUnique(target.geometryRefs.faceIndices);
        CollectionUtil::sortUnique(target.geometryRefs.wireIndices);
        CollectionUtil::sortUnique(target.geometryRefs.edgeIndices);
        CollectionUtil::sortUnique(target.geometryRefs.vertexIndices);

        CollectionUtil::sortUnique(target.adjacentGeometryRefs.faceIndices);
        CollectionUtil::sortUnique(target.adjacentGeometryRefs.wireIndices);
        CollectionUtil::sortUnique(target.adjacentGeometryRefs.edgeIndices);
        CollectionUtil::sortUnique(target.adjacentGeometryRefs.vertexIndices);

        target.axialMin = std::min(target.axialMin, source.axialMin);
        target.axialMax = std::max(target.axialMax, source.axialMax);
        target.axialPosition = 0.5 * (target.axialMin + target.axialMax);

        if (target.note.empty())
        {
            target.note = "Merged axial end boundary";
        }
    }
}
