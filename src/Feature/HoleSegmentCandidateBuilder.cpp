#include "Feature/HoleSegmentCandidateBuilder.h"

#include "Core/CollectionUtil.h"
#include "Geometry/GeometryModel.h"

#include <QDebug>

namespace OccQtCore::Feature
{
    HoleSegmentCandidateBuilder::HoleSegmentCandidateBuilder(
        const GeometryModel& model,
        const std::vector<HoleWallCandidate>& wallCandidates,
        const std::vector<HoleEndCandidate>& endCandidates)
        : m_model(model)
        , m_wallCandidates(wallCandidates)
        , m_endCandidates(endCandidates)
    {
    }

    std::vector<HoleSegmentCandidate> HoleSegmentCandidateBuilder::build() const
    {
        std::vector<HoleSegmentCandidate> candidates;

        for (const auto& wallCandidate : m_wallCandidates)
        {
            HoleSegmentCandidate candidate;

            candidate.index = static_cast<int>(candidates.size());
            candidate.wallCandidateIndex = wallCandidate.index;

            for (const auto& endCandidate : m_endCandidates)
            {
                if (endCandidate.wallCandidateIndex != wallCandidate.index)
                {
                    continue;
                }

                CollectionUtil::addUnique(
                    candidate.endCandidateIndices,
                    endCandidate.index);
            }

            if (candidate.endCandidateIndices.empty())
            {
                continue;
            }

            candidate.depth =
                calculateSegmentDepth(candidate);

            candidates.push_back(candidate);
        }

        return candidates;
    }

    double HoleSegmentCandidateBuilder::calculateSegmentDepth(const HoleSegmentCandidate& candidate) const
    {
        if (!isValidWallIndex(candidate.wallCandidateIndex))
        {
            return 0.0;
        }

        const auto& wall = m_wallCandidates[candidate.wallCandidateIndex];

        std::vector<int> vertexIndices;

        appendVertexIndices(wall.geometryRefs, vertexIndices);

        qDebug()
            << "Segment" << candidate.index
            << "wall" << candidate.wallCandidateIndex
            << "vertex count" << vertexIndices.size();

        for (const int endIndex : candidate.endCandidateIndices)
        {
            if (!isValidEndIndex(endIndex))
            {
                continue;
            }

            appendVertexIndices(m_endCandidates[endIndex].geometryRefs, vertexIndices);
        }

        CollectionUtil::sortUnique(vertexIndices);

        bool hasPosition = false;
        double minPosition = 0.0;
        double maxPosition = 0.0;

        for (const int vertexIndex : vertexIndices)
        {
            const auto* vertex = m_model.vertexAt(vertexIndex);

            if (vertex == nullptr)
            {
                continue;
            }

            const double position =
                gp_Vec(wall.center, vertex->info.point).Dot(
                    gp_Vec(wall.axisDirection));

            if (!hasPosition)
            {
                minPosition = position;
                maxPosition = position;
                hasPosition = true;
                continue;
            }

            minPosition = std::min(minPosition, position);
            maxPosition = std::max(maxPosition, position);
        }

        if (!hasPosition)
        {
            return 0.0;
        }

        return maxPosition - minPosition;
    }

    void HoleSegmentCandidateBuilder::appendVertexIndices(const GeometryRefs& refs, std::vector<int>& vertexIndices) const
    {
        for (const int vertexIndex : refs.vertexIndices)
        {
            CollectionUtil::addUnique(
                vertexIndices,
                vertexIndex);
        }

        const auto& graph =
            m_model.graph();

        for (const int edgeIndex : refs.edgeIndices)
        {
            const auto edgeVertexIndices =
                graph.verticesOfEdge(edgeIndex);

            for (const int vertexIndex : edgeVertexIndices)
            {
                CollectionUtil::addUnique(
                    vertexIndices,
                    vertexIndex);
            }
        }
    }

    bool HoleSegmentCandidateBuilder::isValidEndIndex(int index) const
    {
        return index >= 0 &&
               index < static_cast<int>(m_endCandidates.size());
    }

    bool HoleSegmentCandidateBuilder::isValidWallIndex(int index) const
    {
        return index >= 0 &&
               index < static_cast<int>(m_wallCandidates.size());
    }
}
