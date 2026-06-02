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
