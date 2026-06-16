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
     * @brief 穴文脈ジオメトリグループ種別
     *
     * 穴として後段で解釈しやすいように、
     * 生のジオメトリを幾何条件でまとめた単位の種別。
     *
     * ここでは Bottom / Open / Step などの穴意味は確定しない
     */
    enum class HoleContextGeometryGroupKind
    {
        Unknown,

        Cylindrical,
        Planar,
        Conical,
        Toroidal,
        Mixed,

        Ambiguous
    };

    enum class HoleContextTracePortKind
    {
        Unknown,

        ExternalTransition,

        InternalLoop,

        Ambiguous
    };

    /**
     * @brief 穴文脈でまとめたジオメトリ単位
     *
     * 生のFace群を、穴として後段で説明しやすい幾何単位でまとめたもの。
     * ここでは穴の最終意味は確定しない。
     */
    struct HoleContextGeometryGroup
    {
        int index = -1;

        HoleContextGeometryGroupKind kind = HoleContextGeometryGroupKind::Unknown;

        GeometryRefs geometryRefs;

        bool hasAxis = false;

        gp_Pnt axisPoint;
        gp_Dir axisDirection;

        double radius = 0.0;

        double axialMin = 0.0;
        double axialMax = 0.0;
        double axialPosition = 0.0;

        std::string note;
    };

    /**
     * @brief 穴文脈トレースポート
     *
     * Group 上に存在する閉じた Edge ループ。
     *
     * sourceGroup 外へ接続するものは Trace の入口になる。
     * sourceGroup 内だけで完結するものは内部ループとして保持できるが、
     * Trace 起点にはしない。
     *
     * Port は接続先の意味までは持たない。
     * Open / Bottom / Step などの穴意味は後段の Trace / Interpreter で確定する。
     */
    struct HoleContextTracePort
    {
        int index = -1;

        int sourceGroupIndex = -1;

        HoleContextTracePortKind kind = HoleContextTracePortKind::Unknown;

        GeometryRefs geometryRefs;

        double axialMin = 0.0;
        double axialMax = 0.0;
        double axialPosition = 0.0;

        double circumferentialCoverage = 0.0;

        std::string note;
    };

    enum class HoleContextTraceStepKind
    {
        Unknown,
        NoOutsideFace,
        OutsideFace,
        ReachedExistingGroup,
        Ambiguous
    };

    struct HoleContextTraceStep
    {
        int index = -1;

        int sourceGroupIndex = -1;
        int sourcePortIndex = -1;

        HoleContextTraceStepKind kind = HoleContextTraceStepKind::Unknown;

        GeometryRefs portGeometryRefs;

        GeometryRefs outsideGeometryRefs;

        std::vector<int> adjacentExistingGroupIndices;

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
        std::vector<HoleContextGeometryGroup> contextGeometryGroups;
        std::vector<HoleContextTracePort> contextTracePorts;
        std::vector<HoleContextTraceStep> contextTraceSteps;
    };
}

#endif // HOLERECOGNITIONMODEL_H
