#include "Feature/HoleGeometryTraceBuilder.h"

namespace OccQtCore::Feature
{
    HoleGeometryTraceBuilder::HoleGeometryTraceBuilder(const GeometryModel& model, const std::vector<HoleWall>& walls, const std::vector<HoleWallBoundary>& boundaries)
    : m_model(model)
    , m_walls(walls)
    , m_boundaries(boundaries)
    {
    }

    std::vector<GeometryTrace> HoleGeometryTraceBuilder::build() const
    {
        std::vector<GeometryTrace> traces;

        for (const auto& boundary : m_boundaries)
        {
            if (!isTraceSource(boundary))
            {
                continue;
            }

            traces.push_back(buildTraceFromBoundary(boundary, static_cast<int>(traces.size())));
        }

        return traces;
    }

    bool HoleGeometryTraceBuilder::isTraceSource(const HoleWallBoundary& boundary) const
    {
        if (boundary.kind != HoleWallBoundaryKind::AxialEnd)
        {
            return false;
        }

        if (boundary.traceStatus != HoleWallBoundaryTraceStatus::Traceable)
        {
            return false;
        }

        if (boundary.adjacentGeometryRefs.faceIndices.empty())
        {
            return false;
        }

        return true;
    }

    GeometryTrace HoleGeometryTraceBuilder::buildTraceFromBoundary(const HoleWallBoundary& boundary, int traceIndex) const
    {
        GeometryTrace trace;

        trace.index = traceIndex;
        trace.sourceBoundaryIndex = boundary.index;
        trace.sourceWallIndex = boundary.wallIndex;
        trace.endReason = GeometryTraceEndReason::Unknown;

        GeometryTraceNode node;

        node.index = 0;
        node.depth = 0;
        node.parentNodeIndex = -1;
        node.geometryRefs = boundary.adjacentGeometryRefs;
        node.axialMin = boundary.axialMin;
        node.axialMax = boundary.axialMax;
        node.note = "Depth0 adjacent geometry from source boundary.";

        trace.nodes.push_back(node);

        trace.endReason = GeometryTraceEndReason::NoHoleWallCandidate;
        trace.note = "Initial trace. Interoretation is not performed.";

        return trace;
    }
}
