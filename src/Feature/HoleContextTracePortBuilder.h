#ifndef HOLECONTEXTTRACEPORTBUILDER_H
#define HOLECONTEXTTRACEPORTBUILDER_H

#include <vector>

#include "Feature/HoleRecognitionModel.h"
#include "Geometry/GeometryModel.h"

namespace OccQtCore::Feature
{
    /**
    * @brief 穴文脈 TracePort を生成する。
    *
    * TracePort は、Group 上に存在する閉じた Edge ループ。
    *
    * 閉路であることを Port 候補の成立条件とし、
    * sourceGroup 外へ接続するループを ExternalTransition、
    * sourceGroup 内だけで完結するループを InternalLoop として分類する。
    *
    * Open / Bottom / Step などの穴意味はここでは確定しない。
    */
    class HoleContextTracePortBuilder
    {
    public:
        explicit HoleContextTracePortBuilder(const GeometryModel& model);

        std::vector<HoleContextTracePort> build(
            const std::vector<HoleContextGeometryGroup>& groups) const;

        std::vector<HoleContextTracePort> buildForGroup(
            const std::vector<HoleContextGeometryGroup>& groups,
            int sourceGroupIndex,
            int startPortIndex = 0) const;

    private:
        struct EdgeComponent
        {
            std::vector<int> edgeIndices;
            std::vector<int> vertexIndices;
        };

    private:
        std::vector<HoleContextTracePort> buildPortsOfGroup(
            const HoleContextGeometryGroup& group,
            int& nextPortIndex) const;

        std::vector<int> collectPortCandidateEdgesOfGroup(
            const HoleContextGeometryGroup& group) const;

        bool isPortCandidateEdge(
            const HoleContextGeometryGroup& group,
            int edgeIndex) const;

        bool isOpenBoundaryEdgeOfGroup(
            const HoleContextGeometryGroup& group,
            int edgeIndex) const;

        std::vector<EdgeComponent> buildEdgeComponents(
            const std::vector<int>& edgeIndices) const;

        HoleContextTracePort buildPortFromComponent(
            const HoleContextGeometryGroup& group,
            const EdgeComponent& component,
            int portIndex) const;

        void fillAxialRange(
            const HoleContextGeometryGroup& group,
            HoleContextTracePort& port) const;

        bool hasOutsideFace(
            const HoleContextGeometryGroup& group,
            const std::vector<int>& edgeIndices) const;

        bool hasOutsideFace(
            const HoleContextGeometryGroup& group,
            int edgeIndex) const;

        bool containsFaceIndex(
            const HoleContextGeometryGroup& group,
            int faceIndex) const;

    private:
        const GeometryModel& m_model;
    };
}

#endif // HOLECONTEXTTRACEPORTBUILDER_H
