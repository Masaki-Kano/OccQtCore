#ifndef HOLERECOGNITIONMODEL_H
#define HOLERECOGNITIONMODEL_H

#include <string>
#include <vector>

#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

#include "Feature/FeatureTypes.h"

namespace OccQtCore::Feature
{

    /**
    * @brief 穴壁
    *
    * CAD上で複数Faceに分割されていても、
    * 同じ円筒壁として説明できるFace群を1つに束ねたもの。
    *
    * HoleWall は Face 1枚ではなく、円筒壁としての意味単位。
    * 生の円筒Face情報は GeometryModel / FaceData / FaceInfo を参照する。
    */
    struct HoleWall
    {
        int index = -1;
        bool isValid = false;

        GeometryRefs geometryRefs;

        gp_Pnt axisPoint;
        gp_Dir axisDirection;

        double radius = 0.0;

        double axialMin = 0.0;
        double axialMax = 0.0;
    };

    enum class HoleWallBoundaryKind
    {
        Unknown,
        AxialEnd,
        LateralConnection,
        InternalWallSplit,
        Broken,
        Ambiguous
    };

    enum class HoleWallBoundaryTraceStatus
    {
        Unknown,
        Traceable,
        Ignored,
        NotTraceable
    };

    struct HoleWallBoundary
    {
        int index = -1;

        int wallIndex = -1;

        HoleWallBoundaryKind kind = HoleWallBoundaryKind::Unknown;
        HoleWallBoundaryTraceStatus traceStatus =
            HoleWallBoundaryTraceStatus::Unknown;

        GeometryRefs geometryRefs;
        GeometryRefs adjacentGeometryRefs;

        double axialMin = 0.0;
        double axialMax = 0.0;

        double axialPosition = 0.0;

        double circumferentialCoverage = 0.0;

        std::string note;
    };

    enum class GeometryTraceEndReason
    {
        Unknown,

        ReachedHoleWall,
        NoHoleWallCandidate,
        OutOfHoleContext,
        Ambiguous,
        LoopDetected,
        MaxDepthReached
    };

    struct GeometryTraceNode
    {
        int index = -1;
        int depth = -1;
        int parentNodeIndex = -1;

        GeometryRefs geometryRefs;

        double axialMin = 0.0;
        double axialMax = 0.0;

        std::string note;
    };

    struct GeometryTrace
    {
        int index = -1;
        int sourceBoundaryIndex = -1;
        int sourceWallIndex = -1;

        GeometryTraceEndReason endReason = GeometryTraceEndReason::Unknown;

        std::vector<GeometryTraceNode> nodes;

        std::string note;
    };

    /**
     * @brief 穴認識結果
     *
     * V2穴認識で生成された中間データ一式。
     *
     * HoleWall から始まり、HoleSection / GeometryTrace /
     * HoleTerminal / HoleConnection / HoleAssembly へ展開される。
     */
    struct HoleRecognitionResult
    {
        std::vector<HoleWall> walls;
        std::vector<HoleWallBoundary> wallBoundaries;
        std::vector<GeometryTrace> geometryTraces;
    };
}

#endif // HOLERECOGNITIONMODEL_H
