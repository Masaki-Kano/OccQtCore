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
        explicit HoleContextGeometryGrouper(
            const GeometryModel& model);

        // モデル全体から初期HoleContextGeometryGroupを作る。
        // 初期起点候補として、モデル内の生ジオメトリをBuilderへ配送する。
        std::vector<HoleContextGeometryGroup> group() const;

        // 指定された生ジオメトリ参照からHoleContextGeometryGroupを作る。
        // sourceGroupとの接続可否はここでは判定しない。
        std::vector<HoleContextGeometryGroup> group(
            const GeometryRefs& geometryRefs) const;

    private:
        struct SurfaceKindBuckets
        {
            std::vector<int> planeFaceIndices;
            std::vector<int> cylinderFaceIndices;
            std::vector<int> coneFaceIndices;
            std::vector<int> torusFaceIndices;
            std::vector<int> otherFaceIndices;
        };

    private:
        // GeometryRefs内のFaceをSurfaceKindごとに仕分ける。
        // Grouperは仕分けだけを担当し、穴文脈判定は各Builderへ委譲する。
        SurfaceKindBuckets bucketFacesBySurfaceKind(
            const GeometryRefs& geometryRefs) const;

        void appendGroups(
            std::vector<HoleContextGeometryGroup>& destination,
            std::vector<HoleContextGeometryGroup> source) const;

    private:
        const GeometryModel& m_model;
    };
}

#endif // HOLECONTEXTGEOMETRYGROUPER_H
