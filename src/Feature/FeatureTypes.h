#ifndef FEATURETYPES_H
#define FEATURETYPES_H

#include <vector>

#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>

namespace OccQtCore::Feature
{
    /**
     * @brief フィーチャ種別
     */
    enum class Type
    {
        Unknown,
        Hole,
        Pocket,
        Slot
    };

    /**
     * @brief フィーチャコンポーネントを構成するジオメトリ参照
     *
     * GeometryModel 内の Face / Wire / Edge / Vertex の index を保持する。
     * TopoDS_* の実体は持たず、元形状への参照情報だけを保持する。
     *
     * GeometryGraph が「形状全体の接続関係」を表すのに対し、
     * GeometryRefs は「このフィーチャコンポーネントが、
     * 元のどのジオメトリ要素から構成されたか」を表す。
     */
    struct GeometryRefs
    {
        std::vector<int> faceIndices;
        std::vector<int> wireIndices;
        std::vector<int> edgeIndices;
        std::vector<int> vertexIndices;
    };

    namespace Hole
    {

        /**
         * @brief 穴種別
         */
        enum class Type
        {
            Unknown,

            SimpleBlind,        // 単純止まり穴
            SimpleThrough,      // 単純貫通穴

            SteppedBlind,       // 段付き止まり穴
            SteppedThrough,     // 段付き貫通穴

            ConterBore,         // 座ぐり穴
            CounterSink         // 皿穴
        };

        /**
         * @brief 穴端種別
         *
         * 入口/出口は加工方向に依存するため、形状Feature側では扱わない。
         * Feature側では、端が「外部に開いているか」、「底か」、「段差か」だけ持つ
         */
        enum class EndType
        {
            Unknown,

            Open,   // 外部に空いた端。入口/出口は加工方向で決まる
            Bottom, // 止まり穴の底
            Step    // 段付き穴・座ぐり穴などの段差端
        };

        /**
         * @brief 穴端コンポーネント
         *
         * 穴要素の端を表す。
         *
         * 例:
         * - 止まり穴: Open端 + Bottom端
         * - 貫通穴:   Open端 + Open端
         * - 段付き穴: Open端 + Step端、Step端 + Bottom/Open端
         */
        struct End
        {
            GeometryRefs geometryRefs;

            EndType endType = EndType::Unknown;

            gp_Pnt center;
            gp_Dir axisDirection;       // 穴軸方向
            gp_Dir normalDirection;     // 端面法線、必要に応じて使用する

            double radius = 0.0;
        };

        /**
         * @brief 穴壁コンポーネント
         *
         * 同軸・同半径の円筒面群をまとめあげたもの。
         * Face 1枚でなく、穴壁としての意味を持つ単位
         *
         * 円筒Faceが複数に分かれている場合でも、
         * 同じ穴壁として扱えるものは geometryRefs.faceIndicesにまとめる
         */
        struct Wall
        {
            GeometryRefs geometryRefs;

            gp_Pnt center;
            gp_Dir axisDirection;

            double radius = 0.0;
            double depth = 0.0;
        };

        /**
         * @brief 穴要素
         *
         * 単一径の穴区間を表す。
         * 基本的には 1つの Wall と複数の End で構成される。
         *
         * 単純穴なら Element は1つ。
         * 段付き穴・座ぐり穴などでは、同軸方向に複数の Element が連なる。
         */
        struct Element
        {
            Wall wall;
            std::vector<End> ends;
        };

        /**
         * @brief 認識済み穴フィーチャ
         *
         * 単純穴なら elements は1つ。
         * 段付き穴・座ぐり穴などでは、同軸方向に複数の Element が連なる。
         */
        struct Data
        {
            Feature::Type featureType = Feature::Type::Hole;
            Hole::Type holeType = Hole::Type::Unknown;

            std::vector<Element> elements;
        };
    }
}

#endif // FEATURETYPES_H
