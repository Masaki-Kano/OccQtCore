#ifndef HOLEFEATURERECOGNIZER_H
#define HOLEFEATURERECOGNIZER_H

#include <vector>

#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>

#include "Feature/FeatureTypes.h"

namespace OccQtCore
{
    class GeometryModel;
}

namespace OccQtCore::Feature
{
    /**
     * @brief 穴壁候補
     *
     * ジオメトリ条件だけで抽出した穴壁候補。
     *
     * 基本的に円筒Face 1枚を候補として扱う。
     * 同軸・同半径の判定や、トポロジー接続によるまとめあげは
     * wallComponentへの昇格時に行う。
     */
    struct HoleWallCandidate
    {
        int index = -1;
        int faceIndex = -1;

        gp_Pnt center;
        gp_Dir axisDirection;

        double radius = 0.0;
    };

    /**
     * @brief 穴壁コンポーネント
     *
     * 同軸・同半径かつトポロジー的に連続している円筒Face群を
     * 1つの穴壁としてまとめ上げたもの。
     */
    struct HoleWallComponent
    {
        int index = -1;

        GeometryRefs geometryRefs;

        gp_Pnt center;
        gp_Dir axisDirection;

        double radius = 0.0;
        double depth = 0.0;
    };

    /**
     * @brief 穴端コンポーネント
     *
     * HoleWallComponentの端Edgeからたどって直接生成される穴端。
     *
     * EndComponentはWallComponent由来で生成するため、
     * Wallとの接続関係は生成時点で担保される
     */
    struct HoleEndComponent
    {
        int index = -1;

        /**
         * @brief 端を構成するジオメトリ参照
         *
         * Open:
         *   外部Face / 接続Edge など
         *
         * Bottom:
         *   底Face / 壁との接続Edge など
         *
         * Step:
         *   段差Face / 接続Edge など
         */
        GeometryRefs geometryRefs;

        /**
         * @brief この端の由来WallComponent
         *
         * 基本的に EndComponent は1つの WallComponent の端から生成される。
         * HoleElementCandidate を作るときは、この index で集約する。
         */
        int wallComponentIndex = -1;

        /**
         * @brief 穴端種別
         *
         * Open:
         *   外部に開いた端
         *
         * Bottom:
         *   止まり穴の底
         *
         * Step:
         *   段付き穴・座ぐり穴などの段差端
         */
        Hole::EndType endType = Hole::EndType::Unknown;

        gp_Pnt center;
        gp_Dir axisDirection;
        gp_Dir normalDirection;

        double radius = 0.0;
    };

    /**
     * @brief 穴要素
     *
     * 単一径の穴区間。
     *
     * 1つの HoleWallComponent と、
     * その Wall 由来の HoleEndComponent 群から構成される。
     *
     * FeatureTypes.h の Hole::Element は最終出力用。
     * こちらは認識途中で index 参照を持つ内部表現。
     */
    struct HoleElement
    {
        int index = -1;

        int wallComponentIndex = -1;
        std::vector<int> endComponentIndices;

        Hole::Type type = Hole::Type::Unknown;
    };

    enum class HoleElementStepConnectionDirection
    {
        Unknown,

        // Bottom側Elementの半径 > Open側Elementの半径
        LargerToSmaller,

        // Bottom側Elementの半径 < Open側Elementの半径
        SmallerToLarger
    };

    enum class HoleElementStepConnectionReason
    {
        Unknown,

        // BottomEndのFace群に、OpenEndのEdgeが含まれる
        BottomFaceContainsOpenEdge,

        // BottomEndのFace群が、Open側WallのFace群と隣接する
        BottomEndFaceAdjacentToOpenWall
    };

    struct HoleElementStepConnection
    {
        int candidateIndex = -1;

        int bottomElementIndex = -1;
        int openElementIndex = -1;

        int bottomEndComponentIndex = -1;
        int openEndComponentIndex = -1;

        double bottomElementRadius = 0.0;
        double openElementRadius = 0.0;

        HoleElementStepConnectionDirection direction =
            HoleElementStepConnectionDirection::Unknown;

        HoleElementStepConnectionReason reason =
            HoleElementStepConnectionReason::Unknown;
    };

    /**
     * @brief 穴フィーチャ候補
     *
     * 複数の HoleElement をまとめた穴候補。
     *
     * 単純穴なら elementIndices は1つ。
     * 段付き穴・座ぐり穴などでは、同軸方向に複数の HoleElement が連なる。
     */
    struct HoleCandidate
    {
        int index = -1;

        std::vector<int> elementIndices;

        Hole::Type type = Hole::Type::Unknown;
    };

    /**
     * @brief 穴フィーチャ認識クラス
     *
     * 認識処理の段階:
     *
     * 1. Cylinder Face から HoleWallCandidate を生成する
     * 2. HoleWallCandidate を検証・統合し HoleWallComponent に昇格する
     * 3. HoleWallComponent の端Edgeをたどり、HoleEndComponent を直接生成する
     * 4. HoleWallComponent と HoleEndComponent 群から HoleElementCandidate を作る
     * 5. HoleElementCandidate をまとめて HoleCandidate を作る
     * 6. HoleCandidate を Hole::Data に確定する
     */
    class HoleFeatureRecognizer
    {
    public:
        /**
         * @brief GeometryModelから穴フィーチャを認識する
         */
        std::vector<Hole::Data> recognize(const GeometryModel& model) const;

        std::vector<HoleWallCandidate> detectWallCandidates(const GeometryModel& model) const;
        std::vector<HoleWallComponent> buildWallComponents(const GeometryModel& model, const std::vector<HoleWallCandidate>& wallCandidates) const;
        std::vector<HoleEndComponent> buildEndComponentsFromWallComponents(const GeometryModel& model, const std::vector<HoleWallComponent>& wallComponents) const;
        std::vector<HoleElement> buildHoleElements(const GeometryModel& model, const std::vector<HoleWallComponent>& wallComponents, const std::vector<HoleEndComponent>& endComponents) const;
        std::vector<HoleCandidate> buildHoleCandidatesFromElements(
            const std::vector<HoleWallComponent>& wallComponents,
            const std::vector<HoleElement>& holeElements) const;
        std::vector<HoleElementStepConnection> buildStepConnectionsInHoleCandidates(
            const GeometryModel& model,
            const std::vector<HoleCandidate>& candidates,
            const std::vector<HoleElement>& elements,
            const std::vector<HoleWallComponent>& wallComponents,
            const std::vector<HoleEndComponent>& endComponents) const;

    };
}

#endif // HOLEFEATURERECOGNIZER_H
