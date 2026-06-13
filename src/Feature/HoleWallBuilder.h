#ifndef HOLEWALLBUILDER_H
#define HOLEWALLBUILDER_H

#include <vector>

#include "Feature/HoleRecognitionModel.h"
#include "Geometry/GeometryTypes.h"

namespace OccQtCore
{
    class GeometryModel;
    struct FaceData;
}

namespace OccQtCore::Feature
{
    /**
    * @brief GeometryModel から穴壁として成立する HoleWall を構築する。
    *
    * 円筒Faceを抽出し、内周円筒・同軸同径・軸方向連続・
    * 円周方向閉路を満たすものだけを HoleWall として返す。
    */
    class HoleWallBuilder
    {
    public:
        HoleWallBuilder() = default;

        std::vector<HoleWall> build(const GeometryModel& model) const;

    private:
        struct WallGroup
        {
            HoleWall wall;
            std::vector<int> faceIndices;
        };

        bool canMerge(
            const GeometryModel& model,
            const WallGroup& group,
            int faceIndex) const;

        void mergeFace(
            const GeometryModel& model,
            WallGroup& group,
            int faceIndex) const;

        WallGroup createGroup(
            const GeometryModel& model,
            int faceIndex,
            int groupIndex) const;

        bool isValidHoleWallGroup(
            const GeometryModel& model,
            const WallGroup& group) const;

        bool hasFullCircumferentialCoverage(
            const GeometryModel& model,
            const WallGroup& group) const;

        bool hasTopologicalCircumferentialLoop(
            const GeometryModel& model,
            const WallGroup& group) const;

        bool isInnerCylindricalFace(
            const FaceData& face) const;

        bool isAxialEdgeOfCylinder(
            const GeometryModel& model,
            int edgeIndex,
            const gp_Dir& axisDirection) const;

        bool isAxialRangeConnected(
            double min1,
            double max1,
            double min2,
            double max2) const;

        bool computeFaceAxialRange(
            const FaceData& face,
            const gp_Pnt& axisPoint,
            const gp_Dir& axisDirection,
            double& axialMin,
            double& axialMax) const;
    };
}

#endif // HOLEWALLBUILDER_H
