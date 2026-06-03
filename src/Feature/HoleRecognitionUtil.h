#ifndef HOLERECOGNITIONUTIL_H
#define HOLERECOGNITIONUTIL_H

#include <optional>
#include <vector>

#include "Feature/HoleRecognitionTypes.h"

namespace OccQtCore
{
    class GeometryModel;
}

namespace OccQtCore::Feature::HoleRecognitionUtil
{
    /**
     * @brief 2つの穴壁候補が同一円筒壁として扱えるかを判定する。
     *
     * 半径、軸方向、軸線位置が許容値内で一致する場合に true を返す。
     *
     * この関数では Face 同士の接続関係は判定しない。
     * 実際に同じ穴壁候補としてまとめる場合は、
     * TopologyQuery::hasSharedEdge() などのトポロジー判定と組み合わせる。
     *
     * @param lhs 比較元の穴壁候補。
     * @param rhs 比較先の穴壁候補。
     * @return 同一円筒壁として扱える場合 true。
     */
    bool isSameCylinderCandidate(
        const HoleWallCandidate& lhs,
        const HoleWallCandidate& rhs);

    /**
     * @brief 円筒壁候補グループが周方向に閉じた円筒壁かを判定する。
     *
     * グループ内の円筒FaceのU方向パラメータ幅を合計し、
     * おおむね 2π 以上であれば閉じた円筒壁とみなす。
     *
     * 部分円筒や外径の一部を穴壁として誤認識しないための
     * フィルタとして使用する。
     *
     * @param model 対象ジオメトリモデル。
     * @param wallCandidates 穴壁候補群。
     * @param candidateIndices 判定対象グループに含まれる候補インデックス群。
     * @return 周方向に閉じた円筒壁とみなせる場合 true。
     */
    bool isClosedCylinderWallGroup(
        const GeometryModel& model,
        const std::vector<HoleWallCandidate>& wallCandidates,
        const std::vector<int>& candidateIndices);

    /**
     * @brief 穴壁候補が穴壁らしい向きを持つかを判定する。
     *
     * 構成Faceのうち、円筒Faceの法線が中心軸に対して内向きのものが
     * 1つ以上存在する場合、穴壁らしいと判定する。
     *
     * 外径円筒やボス形状など、外向き円筒面を穴壁として扱わないための
     * フィルタとして使用する。
     *
     * @param model 対象ジオメトリモデル。
     * @param candidate 判定対象の穴壁候補。
     * @return 穴壁らしい向きの場合 true。
     */
    bool isLikelyHoleWallByOrientation(
        const GeometryModel& model,
        const HoleWallCandidate& candidate);

    /**
     * @brief 2つの穴壁候補が同一軸上の候補として扱えるかを判定する
     *
     * 軸方向と軸線位置が許容値内で一致する場合にtrueを返す
     *
     * この関数では半径差、Face同士の接続関係、軸方向の連続性は判定しない
     * 段付き穴・座ぐり穴など。同軸だか半径の異なるセグメントを
     * HoleCandidateとしてまとめるための一次判定に使用する
     *
     * @param lhs 比較元の穴壁候補。
     * @param rhs 比較先の穴壁候補
     * @return 同一軸上の候補として扱える場合true
     */
    bool isSameAxisCandidate(
        const HoleWallCandidate& lhs,
        const HoleWallCandidate& rhs);

    /**
     * @brief 穴壁候補と隣接Faceの接続から穴端候補を生成する。
     *
     * Wall -> Plane、Wall -> Cone/Torus -> Plane などの接続関係から、
     * Open / Bottom などの HoleEndCandidate を生成する。
     *
     * 直接判定できない接続や、現時点で未対応の接続の場合は std::nullopt を返す。
     *
     * @param model 対象ジオメトリモデル。
     * @param wallCandidate 端候補の由来となる穴壁候補。
     * @param adjacentFaceIndex 穴壁境界Edgeの隣接Faceインデックス。
     * @param connectionEdgeIndex 穴壁と隣接Faceを接続するEdgeインデックス。
     * @return 生成できた穴端候補。生成できない場合は std::nullopt。
     */
    std::optional<HoleEndCandidate> buildEndCandidateFromWallConnection(
        const GeometryModel& model,
        const HoleWallCandidate& sourceWallCandidate,
        const std::vector<HoleWallCandidate>& allWallCandidates,
        int adjacentFaceIndex,
        int connectionEdgeIndex);

    bool isWallOnCandidateAxis(
        const HoleCandidateAxis& candidateAxis,
        const HoleWallCandidate& wallCandidate);

    /**
     * @brief HoleCandidate の評価用共通軸を生成する。
     *
     * Candidate に含まれる最初の有効な HoleSegmentCandidate の
     * Wall 軸を代表軸として使用する。
     *
     * @param candidate 対象の穴候補。
     * @param segmentCandidates 穴セグメント候補群。
     * @param wallCandidates 穴壁候補群。
     * @return 生成した共通軸。有効な軸を取得できない場合は isValid=false。
     */
    HoleCandidateAxis buildHoleCandidateAxis(
        const HoleCandidate& candidate,
        const std::vector<HoleSegmentCandidate>& segmentCandidates,
        const std::vector<HoleWallCandidate>& wallCandidate);

    /**
     * @brief HoleSegmentCandidate を HoleCandidateAxis 上の範囲へ変換する。
     *
     * Segment が持つ各 HoleEndCandidate::center を Candidate 共通軸へ射影し、
     * その最小値・最大値を範囲として返す。
     *
     * @param segment 対象セグメント。
     * @param candidateAxis Candidate 評価用共通軸。
     * @param endCandidates 穴端候補群。
     * @return Candidate 共通軸上で見た Segment 範囲。
     */
    HoleSegmentRangeOnCandidateAxis buildHoleSegmentRangeOnCandidateAxis(
        const HoleSegmentCandidate& segment,
        const HoleCandidateAxis& candidateAxis,
        const std::vector<HoleEndCandidate>& endCandidates);

    bool hasSharedEndGeometryRef(
        const HoleEndCandidate& lhs,
        const HoleEndCandidate& rhs);

    HoleSegmentConnectionKind classifyAdjacentSegmentConnection(
        const GeometryModel& model,
        const HoleSegmentRangeOnCandidateAxis& currentRange,
        const HoleSegmentRangeOnCandidateAxis& nextRange,
        const std::vector<HoleEndCandidate>& endCandidates);

    HoleSegmentConnection buildHoleSegmentConnection(
        const HoleSegmentRangeOnCandidateAxis& currentRange,
        const HoleSegmentRangeOnCandidateAxis& nextRange,
        const std::vector<HoleEndCandidate>& endCandidates);

    std::vector<HoleSegmentConnection> buildHoleSegmentConnections(
        const HoleCandidate& candidate,
        const std::vector<HoleEndCandidate>& endCandidates);

    std::vector<HoleSegmentChain> buildHoleSegmentChains(
        const HoleCandidate& candidate,
        const std::vector<HoleSegmentConnection>& connections);

    bool isPointOnPlaneFace(
        const GeometryModel& model,
        int faceIndex,
        const gp_Pnt& point,
        double tolerance);

    bool hasPlaneFaceContainingEndCenter(
        const GeometryModel& model,
        const HoleEndCandidate& planeEnd,
        const HoleEndCandidate& targetEnd);

    bool isShoulderPlaneConnection(
        const GeometryModel& model,
        const HoleEndCandidate& currentEnd,
        const HoleEndCandidate& nextEnd);

    /**
     * @brief 2つの穴端候補が同一端として扱えるかを判定する。
     *
     * detectEndCandidates() で同じ端候補を複数経路から検出した場合の
     * 重複除外に使用する。
     *
     * @param lhs 比較元の穴端候補。
     * @param rhs 比較先の穴端候補。
     * @return 同一端として扱える場合 true。
     */
    bool isSameEndCandidate(
        const HoleEndCandidate& lhs,
        const HoleEndCandidate& rhs);

    void appendOrMergeEndCandidate(
        std::vector<HoleEndCandidate>& candidates,
        const HoleEndCandidate& candidate);

}

#endif // HOLERECOGNITIONUTIL_H
