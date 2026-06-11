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
            SimpleBlind,    // 単純止まり穴
            SimpleThrough,  // 単純貫通穴
            Complex         // 複合穴
        };

        /**
         * @brief 穴端種別
         *
         * 入口/出口は加工方向に依存するため、形状Feature側では扱わない。
         * Feature側では、端が「外部に開いているか」、「底か」、「他Elementへ接続する端か」だけ持つ
         */
        enum class EndType
        {
            Unknown,
            Open,       // 外部に開いた端
            Bottom,     // 止まり穴の底
            Connected,  // 他Elementへ接続する端
        };

        enum class ElementConnectionType
        {
            Unknown,

            Direct,         // End同士が直接同じジオメトリを共有する接続
            SharedPathFace  // 共通の穴経路面を介した接続
        };

        /**
         * @brief 穴端コンポーネント
         *
         * 穴要素の端を表す。
         *
         * 例:
         * - 止まり穴: Open端 + Bottom端
         * - 貫通穴:   Open端 + Open端
         * - 複合穴:   Open/Bottom/Connected の組み合わせ
         */
        struct End
        {
            int sourceEndCandidateIndex = -1;

            GeometryRefs geometryRefs;
            EndType endType = EndType::Unknown;
        };

        /**
         * @brief 穴壁コンポーネント
         *
         * 同軸・同半径の円筒面群をまとめあげたもの。
         * Face 1枚でなく、穴壁としての意味を持つ単位。
         */
        struct Wall
        {
            int sourceEndCandidateIndex = -1;
            GeometryRefs geometryRefs;

            gp_Pnt center;
            gp_Dir axisDirection;

            double radius = 0.0;
        };

        /**
         * @brief 穴要素
         *
         * 単一径の穴区間を表す。
         * 原則として HoleSegmentCandidate 1個から生成される。
         */
        struct Element
        {
            int index = -1;

            int sourceSegmentCandidateIndex = -1;

            Wall wall;
            std::vector<End> ends;

            double depth = 0.0;
        };

        /**
         * @brief 穴要素間の接続情報
         *
         * 複合穴で Element 同士がどう接続しているかを表す。
         */
        struct ElementConnection
        {
            int lhsElementIndex = -1;
            int rhsElementIndex = -1;

            int lhsEndIndex = -1;
            int rhsEndIndex = -1;

            ElementConnectionType type = ElementConnectionType::Unknown;

            GeometryRefs geometryRefs;
        };

        /**
         * @brief 認識済み穴フィーチャ
         *
         * 単純穴なら elements は1つ。
         * 複合穴なら elements が複数になり、
         * elementConnections がそれらの接続関係を表す。
         */
        struct Data
        {
            int index = -1;

            int sourceHoleCandidateIndex = -1;

            Feature::Type featureType = Feature::Type::Hole;
            Hole::Type holeType = Hole::Type::Unknown;

            gp_Pnt axisPoint;
            gp_Dir axisDirection = gp_Dir(0.0, 0.0, 1.0);

            std::vector<Element> elements;
            std::vector<ElementConnection> elementConnections;
        };

        inline const char* holeTypeDisplayName(Type type)
        {
            switch (type)
            {
            case Type::SimpleBlind:
                return "単純止まり穴";

            case Type::SimpleThrough:
                return "単純貫通穴";

            case Type::Complex:
                return "複合穴";

            case Type::Unknown:
            default:
                return "不明";

            }
        }

        inline const char* endTypeDisplayName(EndType type)
        {
            switch (type)
            {
            case EndType::Open:
                return "開口端";

            case EndType::Bottom:
                return "底端";

            case EndType::Connected:
                return "接続端";

            case EndType::Unknown:
            default:
                return "不明";
            }
        }

        inline const char* elementConnectionTypeDisplayName(
            ElementConnectionType type)
        {
            switch (type)
            {
            case ElementConnectionType::Direct:
                return "直接接続";

            case ElementConnectionType::SharedPathFace:
                return "共通経路面接続";

            case ElementConnectionType::Unknown:
            default:
                return "不明";
            }
        }
    }
}

#endif // FEATURETYPES_H
