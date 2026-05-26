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
     * @brief 穴端候補
     *
     * ジオメトリ条件だけで抽出した穴端候補。
     *
     * 現時点では、平面Face上の円形InnerWireをOpenとして扱う。
     * この段階では、穴壁とトポロジー接続しているかはまだ見ない
     */
    struct HoleEndCandidate
    {
        int faceIndex = -1;
        int wireIndex = -1;

        Hole::EndType endType = Hole::EndType::Unknown;
    };

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
        int faceIndex = -1;

        gp_Pnt center;
        gp_Dir axisDirection;

        double radius = 0.0;
    };

    /**
     * @brief 穴端コンポーネント
     *
     * HoleEndCandidate を、トポロジー接続や構成ジオメトリを確認したうえで
     * 穴端として意味を持つ単位に昇格したもの。
     */
    struct HoleEndComponent
    {
        GeometryRefs geometryRefs;

        Hole::EndType endType = Hole::EndType::Unknown;

        gp_Pnt center;
        gp_Dir axisDirection;
        gp_Dir normalDirection;

        double radius = 0.0;

        std::vector<int> adjacentFaceIndices;
    };

    /**
     * @brief 穴壁コンポーネント
     *
     * 同軸・同半径かつトポロジー的に連続している円筒Face群を
     * 1つの穴壁としてまとめ上げたもの。
     */
    struct HoleWallComponent
    {
        GeometryRefs geometryRefs;

        gp_Pnt center;
        gp_Dir axisDirection;

        double radius = 0.0;
        double depth = 0.0;
    };

    /**
     * @brief 穴要素候補
     *
     * 単一径の穴区間候補。
     *
     * WallComponent 群と EndComponent 群のトポロジー接続を確認し、
     * 穴要素として成立しそうなまとまりを表す。
     *
     * この段階ではまだ確定Elementではない。
     */
    struct HoleElementCandidate
    {
        std::vector<HoleWallComponent> walls;
        std::vector<HoleEndComponent> ends;
    };

    /**
     * @brief 穴フィーチャ候補
     *
     * 穴要素候補をまとめた穴候補。
     *
     * 単純穴なら elements は1つ。
     * 段付き穴・座ぐり穴などでは、同軸方向に複数の elements が連なる。
     */
    struct HoleCandidate
    {
        std::vector<HoleElementCandidate> elements;
    };

    /**
     * @brief 穴フィーチャ認識クラス
     *
     * 認識処理の段階:
     *
     * 1. ジオメトリ条件だけで End / Wall の Candidate を生成する
     * 2. Candidate をトポロジー接続で検証・統合し Component に昇格する
     * 3. EndComponent と WallComponent から HoleElementCandidate を作る
     * 4. HoleElementCandidate をまとめて HoleCandidate を作る
     * 5. HoleCandidate を Hole::Data に確定する
     */
    class HoleFeatureRecognizer
    {
    public:
        /**
         * @brief GeometryModelから穴フィーチャを認識する
         */
        std::vector<Hole::Data> recognize(const GeometryModel& model) const;

        std::vector<HoleEndCandidate> detectEndCandidates(const GeometryModel& model) const;

        std::vector<HoleEndComponent> buildEndComponents(const GeometryModel& model, const std::vector<HoleEndCandidate>& endCandidates) const;

    };
}

#endif // HOLEFEATURERECOGNIZER_H
