#ifndef HOLECONTEXTPLANARGROUPBUILDER_H
#define HOLECONTEXTPLANARGROUPBUILDER_H

#include "Feature/HoleRecognitionModel.h"

#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

#include <vector>

namespace OccQtCore
{
class GeometryModel;
}

namespace OccQtCore::Feature
{
    class HoleContextPlanarGroupBuilder
    {
    public:
        explicit HoleContextPlanarGroupBuilder(
            const GeometryModel& model);

        // 指定された平面Face群からBoundaryCandidateを作る。
        std::vector<HoleContextGeometryGroup> build(
            const std::vector<int>& faceIndices) const;

    private:
        struct WorkingGroup
        {
            HoleContextGeometryGroup group;
            std::vector<int> faceIndices;
        };

    private:
        std::vector<HoleContextGeometryGroup> buildFromFaces(
            const std::vector<int>& faceIndices) const;

        // Faceを既存WorkingGroupに統合できるか。
        // 生ジオメトリを同じ平面単位でまとめられるかを見る。
        bool canMergeFace(
            const WorkingGroup& group,
            int faceIndex) const;

        void mergeFace(
            WorkingGroup& group,
            int faceIndex) const;

        WorkingGroup createWorkingGroup(
            int faceIndex,
            int groupIndex) const;

        // まとめた結果がBoundaryCandidateとして成立するかを見る。
        bool isValidBoundaryCandidate(
            const WorkingGroup& group) const;

        bool isSamePlane(
            const gp_Pnt& pointA,
            const gp_Dir& normalA,
            const gp_Pnt& pointB,
            const gp_Dir& normalB) const;

    private:
        const GeometryModel& m_model;
    };
}

#endif // HOLECONTEXTPLANARGROUPBUILDER_H
