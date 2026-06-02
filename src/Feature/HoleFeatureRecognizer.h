#ifndef HOLEFEATURERECOGNIZER_H
#define HOLEFEATURERECOGNIZER_H

#include <vector>

#include "Feature/FeatureTypes.h"
#include "Feature/HoleRecognitionTypes.h"

namespace OccQtCore
{
    class GeometryModel;
}

namespace OccQtCore::Feature
{
    /**
     * @brief 穴フィーチャ認識クラス
     *
     * GeometryModel から穴フィーチャを認識する。
     *
     * 認識処理の大まかな段階:
     *
     * 1. 円筒Face群から HoleWallCandidate を生成する
     * 2. HoleWallCandidate の端Edgeをたどり、HoleEndCandidate を生成する
     * 3. HoleWallCandidate と HoleEndCandidate 群から HoleSegmentCandidate を作る
     * 4. HoleSegmentCandidate をまとめて HoleCandidate を作る
     * 5. HoleCandidate を最終出力用の Hole::Data に変換する
     *
     * HoleWallCandidate / HoleEndCandidate / HoleSegmentCandidate / HoleCandidate は
     * 認識途中の内部表現であり、FeatureTypes.h の Hole::Data 系が最終出力用の型。
     */
    class HoleFeatureRecognizer
    {
    public:
        /**
         * @brief GeometryModelから穴フィーチャを認識する
         *
         * @param model 対象のジオメトリモデル
         * @return 認識済みの穴フィーチャ群
         */
        std::vector<Hole::Data> recognize(
            const GeometryModel& model) const;

        /**
         * @brief 穴壁候補を検出する。
         *
         * 円筒Faceを抽出し、同軸・同半径かつトポロジー的に連続するものを
         * 1つの HoleWallCandidate としてまとめる。
         *
         * @param model 対象ジオメトリモデル。
         * @return 検出された穴壁候補群。
         */
        std::vector<HoleWallCandidate> detectWallCandidates(
            const GeometryModel& model) const;

        /**
         * @brief 穴端候補を生成する。
         *
         * 各HoleWallCandidateの境界接続をたどり、
         * Open/Bottom/Stepなどの穴端候補を生成する。
         *
         * @param model 対象のジオメトリモデル
         * @param wallCandidates 穴端候補群
         * @return 生成された穴端候補数
         */
        std::vector<HoleEndCandidate> detectEndCandidates(
            const GeometryModel& model,
            const std::vector<HoleWallCandidate>& wallCandidate) const;

        /**
         * @brief 穴セグメント候補を生成する。
         *
         * 1つの HoleWallCandidate と、その壁由来の HoleEndCandidate 群から、
         * 単一径の穴区間候補を生成する。
         *
         * @param model 対象ジオメトリモデル。
         * @param wallCandidates 穴壁候補群。
         * @param endCandidates 穴端候補群。
         * @return 生成された穴セグメント候補群。
         */
        std::vector<HoleSegmentCandidate> buildSegmentCandidates(
            const GeometryModel& model,
            const std::vector<HoleWallCandidate>& wallCandidates,
            const std::vector<HoleEndCandidate>& endCandidates) const;

        /**
         * @brief 穴候補を生成する。
         *
         * 穴セグメント候補群を、単純穴、段付き穴、座ぐり穴などの
         * 穴フィーチャ候補としてまとめる。
         *
         * @param model 対象のジオメトリモデル
         * @param wallCandites 穴壁候補群
         * @param endCandidates 穴端候補群
         * @param segmentCandidates 穴セグメント候補群
         * @return
         */
        std::vector<HoleCandidate> buildHoleCandidates(
            const GeometryModel& model,
            const std::vector<HoleWallCandidate>& wallCandites,
            const std::vector<HoleEndCandidate>& endCandidates,
            const std::vector<HoleSegmentCandidate>& segmentCandidates) const;

    };
}

#endif // HOLEFEATURERECOGNIZER_H
