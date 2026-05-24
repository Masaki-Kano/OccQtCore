#ifndef PICKRESULT_H
#define PICKRESULT_H

#include <TopoDS_Shape.hxx>

namespace OccQtCore
{

    enum class PickedShapeType
    {
        Unknown,
        Vertex,
        Edge,
        Wire,
        Face,
        Shell,
        Solid,
        Compound
    };

    struct PickResult
    {
        bool hasShape = false;

        PickedShapeType type = PickedShapeType::Unknown;
        TopoDS_Shape shape;

        // OccView内の表示オブジェクト由来ID。
        // DisplayObjectId型はView側の概念なので、Core側ではintとして持つ。
        int sourceDisplayObjectId = -1;
    };

} // namespace OccQtCore

#endif // PICKRESULT_H
