#ifndef HOLECONTEXTGEOMETRYGROUPER_H
#define HOLECONTEXTGEOMETRYGROUPER_H

#include "Feature/HoleRecognitionModel.h"

#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>

#include <vector>

namespace OccQtCore
{
    class GeometryModel;
    struct FaceData;
}

namespace OccQtCore::Feature
{
    class HoleContextGeometryGrouper
    {
    public:
        explicit HoleContextGeometryGrouper(const GeometryModel& model);

        std::vector<HoleContextGeometryGroup> group() const;

        std::vector<HoleContextGeometryGroup> groupFromGeometryRefs(const GeometryRefs& geometryRefs, const HoleContextGeometryGroup& parentGroup) const;

    private:
        // 円筒形ジオメトリ単位の作業グループ
        struct CylindricalGroup
        {
            HoleContextGeometryGroup group;
            std::vector<int> faceIndices;
        };

    private:
        // 円筒形関連の関数群
        std::vector<HoleContextGeometryGroup> buildCylindricalGroups() const;
        bool canMergeCylindricalFace(const CylindricalGroup& group, int faceIndex) const;
        void mergeCylindricalFace(CylindricalGroup& group, int faceIndex) const;
        CylindricalGroup createCylindricalGroup(int faceIndex, int groupIndex) const;
        bool isValidCylindricalGroup(const CylindricalGroup& group) const;
        bool hasFullCircumferentialCoverage(const CylindricalGroup& group) const;
        bool hasTopologicalCircumferentialLoop(const CylindricalGroup& group) const;
        bool isInnerCylindricalFace(const FaceData& face) const;
        bool isAxialEdgeOfCylinder(int edgeIndex, const gp_Dir& axisDirection) const;
        bool computeFaceAxialRange(const FaceData& face, const gp_Pnt& axisPoint, const gp_Dir& axisDirection, double& axialMin, double& axialMax) const;
        bool isAxialRangeConnected(double min1, double max1, double min2, double max2) const;

    private:
        // 平面系ジオメトリ関連の関数群
        std::vector<HoleContextGeometryGroup> buildPlanarGroupsFromFaces(const std::vector<int>& faceIndices, const HoleContextGeometryGroup& parentGroup) const;
        bool canMergePlanarFace(const HoleContextGeometryGroup& group, int faceIndex) const;
        void mergePlanarFace(HoleContextGeometryGroup& group, int faceIndex) const;
        HoleContextGeometryGroup createPlanarGroup(int faceIndex, const HoleContextGeometryGroup& parentGroup) const;
        bool isSamePlane(const gp_Pnt& pointA, const gp_Dir& normalA, const gp_Pnt& pointB, const gp_Dir& normalB) const;

    private:
        const GeometryModel& m_model;
    };
}

#endif // HOLECONTEXTGEOMETRYGROUPER_H
