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
}

#endif // FEATURETYPES_H
