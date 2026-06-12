#ifndef HOLERECOGNITIONTYPES_H
#define HOLERECOGNITIONTYPES_H

#include <vector>

#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

#include "Feature/FeatureTypes.h"

namespace OccQtCore::Feature
{
    /**
     * @brief 穴端候補種別
     *
     * 穴認識途中で使用する端候補の分類。
     *
     * FeatureTypes.h の Hole::EndType は最終出力用の端種別であり、
     * こちらは HoleEndCandidate の内部判定用。
     *
     * WallConnection は、別の HoleWallCandidate に接続している端を表す。
     * この時点では Step 確定ではなく、後続のセグメント接続判定で
     * 段付き穴・座ぐり穴などとして解釈できるかを判断する。
     */
    enum class HoleEndCandidateType
    {
        Unknown,
        Open,
        Bottom,
        Connected
    };

    inline const char* holeEndCandidateTypeDisplayName(HoleEndCandidateType type)
    {
        switch (type)
        {
        case HoleEndCandidateType::Open:
            return "Open";

        case HoleEndCandidateType::Bottom:
            return "Bottom";

        case HoleEndCandidateType::Connected:
            return "Connected";

        case HoleEndCandidateType::Unknown:
        default:
            return "Unknown";
        }
    }

    /**
     * @brief 穴壁候補
     *
     * 穴壁として扱える可能性のある円筒Face群。
     *
     * 初期検出では円筒Face 1枚から生成し、
     * 同軸・同半径かつトポロジー的に連続するものを同じ候補としてまとめる。
     *
     * 最終出力用の Hole::Wall ではなく、
     * 認識途中で index 参照を持つ内部表現。
     */
    struct HoleWallCandidate
    {
        int index = -1;

        GeometryRefs geometryRefs;

        gp_Pnt center;
        gp_Dir axisDirection;

        double radius = 0.0;
    };

    /**
     * @brief 穴端候補
     *
     * HoleWallCandidate の境界接続から生成される穴端候補。
     *
     * Open / Bottom / WallConnection などの端種別は持つが、
     * 最終出力用の Hole::End ではなく、
     * 認識途中で index 参照を持つ内部表現。
     *
     * この構造体は端の「位置」を表さない。
     * 位置、軸方向範囲、中心点などは、利用側が
     * WallCandidate / GeometryRefs / CandidateAxis から
     * 文脈に応じて計算する。
     */
    struct HoleEndCandidate
    {
        int index = -1;

        /**
         * @brief 端候補を構成するジオメトリ参照
         *
         * Open:
         *   Open端を特徴づける接続Edgeや遷移Faceなど。
         *   外部に開いた先のFaceは、必ずしも構成Faceとして保持しない。
         *
         * Bottom:
         *   底Face、遷移Face、壁との接続Edgeなど。
         *
         * WallConnection:
         *   他Wallや段差候補との接続を示すFace/Edgeなど。
         */
        GeometryRefs geometryRefs;

        /**
         * @brief この端候補の由来WallCandidate
         *
         * 基本的に HoleEndCandidate は1つの HoleWallCandidate の端から生成される。
         * HoleSegmentCandidate を作るときは、この index で集約する。
         */
        int wallCandidateIndex = -1;

        /**
         * @brief 由来WallCandidate上の境界ループ番号
         *
         * HoleEndCandidateDetectorがWallCandidateの境界Edge群を
         * 円周方向の閉路/連結成分に分けたときのローカルindex
         *
         * 同じwallCandidateIndexかつ同じwallBoundaryLoopIndexの候補は、
         * 同じWall端領域から生成された候補としてマージ対象となる
         *
         */
        int wallBoundaryLoopIndex = -1;

        /**
         * @brief 穴端候補種別
         *
         * 最終出力用の Hole::EndType ではなく、認識途中の分類。
         * WallConnection は後続工程で Step などに解釈される可能性がある。
         */
        HoleEndCandidateType type = HoleEndCandidateType::Unknown;
    };

    /**
     * @brief 穴セグメント候補
     *
     * 単一径の穴区間候補。
     *
     * 1つの HoleWallCandidate と、
     * その Wall 由来の HoleEndCandidate 群から構成される。
     */
    struct HoleSegmentCandidate
    {
        int index = -1;

        int wallCandidateIndex = -1;
        std::vector<int> endCandidateIndices;
        double depth = 0.0;
    };

    enum class HoleReachabilityReason
    {
        Unknown,
        SharedGeometryRef,
        SharedAdjacentFace
    };

    inline const char* holeReachabilityReasonDisplayName(HoleReachabilityReason reason)
    {
        switch (reason)
        {
        case HoleReachabilityReason::SharedGeometryRef:
            return "SharedGeometryRef";

        case HoleReachabilityReason::SharedAdjacentFace:
            return "SharedAdjacentFace";

        case HoleReachabilityReason::Unknown:
        default:
            return "Unknown";
        }
    }

    struct HoleReachability
    {
        int lhsSegmentCandidateIndex = -1;
        int rhsSegmentCandidateIndex = -1;

        int lhsEndCandidateIndex = -1;
        int rhsEndCandidateIndex = -1;

        HoleReachabilityReason reason = HoleReachabilityReason::Unknown;

        GeometryRefs sharedGeometryRefs;
    };

    /**
     * @brief 穴候補
     *
     * HoleSegmentCandidate を軸単位の判断基準でまとめた穴フィーチャ候補。
     *
     * この時点では、同一軸上に存在するセグメント集合であり、
     * すべてのセグメントがトポロジー的に連結した同一穴であるとは限らない。
     *
     * 後続工程で、軸方向位置、端種別、セグメント間の接続関係を評価し、
     * 穴として成立するものを Hole::Data へ変換する。
     *
     * 単純穴なら segmentCandidateIndices は1つ。
     * 段付き穴・座ぐり穴などでは、同軸方向に複数のセグメントが連なる。
     *
     * FeatureTypes.h の Hole::Data は最終出力用。
     * こちらは認識途中で index 参照を持つ内部表現。
     */
    struct HoleCandidate
    {
        int index = -1;
        std::vector<int> segmentCandidateIndices;

        gp_Pnt axisPoint;
        gp_Dir axisDirection = gp_Dir(0.0, 0.0, 1.0);

        std::vector<HoleReachability> reachabilities;
    };

    /**
     * @brief 穴認識結果
     *
     * 穴認識処理で生成された中間候補データ一式。
     *
     * Wall / End / Segment / HoleCandidate は index 参照で互いに関連しているため、
     * 個別の vector ではなく、同じ認識結果セットとして扱う。
     *
     * ここでの HoleCandidate は最終出力用の Hole::Data ではなく、
     * 認識途中または診断用の穴フィーチャ候補。
     */
    struct HoleRecognitionResult
    {
        std::vector<HoleWallCandidate> wallCandidates;
        std::vector<HoleEndCandidate> endCandidates;
        std::vector<HoleSegmentCandidate> segmentCandidates;
        std::vector<HoleCandidate> holeCandidates;

        std::vector<Hole::Data> holes;
    };
}

#endif // HOLERECOGNITIONTYPES_H
