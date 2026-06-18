#ifndef HOLECONTEXTCYLINDRICALGROUPBUILDER_H
#define HOLECONTEXTCYLINDRICALGROUPBUILDER_H

#include "Feature/HoleRecognitionModel.h"

#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

#include <vector>

namespace OccQtCore
{
class GeometryModel;
struct FaceData;
}

namespace OccQtCore
{
class GeometryModel;
struct FaceData;
}

namespace OccQtCore::Feature
{
    class HoleContextCylindricalGroupBuilder
    {
    public:
        explicit HoleContextCylindricalGroupBuilder(
            const GeometryModel& model);

        // モデル全体から円筒Faceを収集し、WallCandidateを作る。
        std::vector<HoleContextGeometryGroup> build() const;

        // 指定された円筒Face群からWallCandidateを作る。
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
        // 生ジオメトリを同じ単位でまとめられるかを見る。
        bool canMergeFace(
            const WorkingGroup& group,
            int faceIndex) const;

        void mergeFace(
            WorkingGroup& group,
            int faceIndex) const;

        WorkingGroup createWorkingGroup(
            int faceIndex,
            int groupIndex) const;

        // まとめた結果が穴文脈上のWallCandidateとして成立するかを見る。
        bool isValidWallCandidate(
            const WorkingGroup& group) const;

        bool hasFullCircumferentialCoverage(
            const WorkingGroup& group) const;

        bool hasTopologicalCircumferentialLoop(
            const WorkingGroup& group) const;

        bool isFaceConnectedToGroup(
            const WorkingGroup& group,
            int faceIndex) const;

        bool hasInnerCylindricalFace(
            const WorkingGroup& group) const;

        bool isInnerCylindricalFace(
            const FaceData& face) const;

        bool isAxialEdgeOfCylinder(
            int edgeIndex,
            const gp_Dir& referenceDirection) const;

        bool computeFaceParameterRange(
            const FaceData& face,
            const gp_Pnt& referencePoint,
            const gp_Dir& referenceDirection,
            double& parameterMin,
            double& parameterMax) const;

        bool isParameterRangeConnected(
            double min1,
            double max1,
            double min2,
            double max2) const;

    private:
        const GeometryModel& m_model;
    };
}

#endif // HOLECONTEXTCYLINDRICALGROUPBUILDER_H
