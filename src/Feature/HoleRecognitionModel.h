#ifndef HOLERECOGNITIONMODEL_H
#define HOLERECOGNITIONMODEL_H

#include <string>
#include <vector>

#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

#include "Feature/FeatureTypes.h"
#include "Geometry/GeometryTypes.h"

namespace OccQtCore::Feature
{
    /**
     * @brief 穴文脈ジオメトリグループ種別
     *
     * 生ジオメトリを穴探索で扱いやすい単位へまとめた結果、
     * そのGroupが穴文脈上どのような候補として扱えるかを表す。
     *
     * 面の幾何種別そのものは Geometry::SurfaceKindで表す。
     * ここでは Bottom / Open / Step / Conterboreなどの
     * 最終的な穴意味は確定しない
     */
    enum class HoleContextGeometryGroupKind
    {
        Unknown,

        WallCandidate,          // 穴壁候補。主に内周円筒面
        BoundaryCandidate,      // 穴境界候補。入口 / 底 / 肩 / 終端になり得る面。
        TransitionCandidate,    // 遷移候補。 面取り / R / テーパーなど

        Ambiguous
    };

    enum class HoleContextTracePortKind
    {
        Unknown,

        ExternalTransition,

        InternalLoop,

        Ambiguous
    };

    enum class HoleContextTraceStepKind
    {
        Unknown,
        NoOutsideFace,
        OutsideFace,
        ReachedExistingGroup,
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

        // 穴文脈上の粗い役割。
        // WallCandidate / BoundaryCandidate / TransitionCandidate など。
        HoleContextGeometryGroupKind kind =
            HoleContextGeometryGroupKind::Unknown;

        GeometryRefs geometryRefs;

        // Groupの代表点を持つ場合にtrue。
        // WallCandidate + Cylinder: 軸上代表点
        // BoundaryCandidate + Plane: 平面上代表点
        // TransitionCandidate + Cone/Torus: 代表点
        bool hasReferencePoint = false;
        gp_Pnt referencePoint;

        // Groupの代表方向を持つ場合にtrue。
        // WallCandidate + Cylinder: 円筒軸方向
        // BoundaryCandidate + Plane: 平面法線
        // TransitionCandidate + Cone/Torus: 代表軸方向
        bool hasReferenceDirection = false;
        gp_Dir referenceDirection;

        // WallCandidate + Cylinder: 半径
        // BoundaryCandidate + Plane: 未使用
        // TransitionCandidate + Cone/Torus: 代表半径として利用予定
        double radius = 0.0;

        // referenceDirection または親Groupの文脈方向に対する範囲/代表位置。
        // WallCandidate + Cylinder: 軸方向範囲
        // BoundaryCandidate + Plane: 親軸に対する位置など
        // TransitionCandidate + Cone/Torus: 代表方向に対する範囲として利用予定
        double parameterMin = 0.0;
        double parameterMax = 0.0;
        double parameterPosition = 0.0;

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

    struct HoleContextTraceStep
    {
        int index = -1;

        int sourceGroupIndex = -1;
        int sourcePortIndex = -1;

        HoleContextTraceStepKind kind = HoleContextTraceStepKind::Unknown;

        GeometryRefs portGeometryRefs;
        GeometryRefs outsideGeometryRefs;

        std::vector<int> observedGroupIndices;

        std::string note;
    };

    /**
     * @brief 穴文脈トレースセッション
     *
     * 1つのSeed Wallから開始したDFS探索結果。
     *
     * デバッグツリーではこのSessionを1単位として表示する。
     */
    struct HoleContextTraceSession
    {
        int index = -1;

        int seedGroupIndex = -1;

        std::vector<int> reachedGroupIndices;
        std::vector<int> reachedWallGroupIndices;

        std::vector<int> traceStepIndices;

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

        std::vector<HoleContextTraceSession> contextTraceSessions;
    };
}

#endif // HOLERECOGNITIONMODEL_H
