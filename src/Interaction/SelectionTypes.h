#ifndef SELECTIONTYPES_H
#define SELECTIONTYPES_H

#include <TopoDS_Shape.hxx>

#include "View/DisplayTypes.h"

namespace OccQtCore
{
    enum class GeometryElementKind
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

    inline const char* geometryElementKindDisplayName(
        GeometryElementKind kind)
    {
        switch (kind)
        {
        case GeometryElementKind::Vertex:
            return "Vertex";

        case GeometryElementKind::Edge:
            return "Edge";

        case GeometryElementKind::Wire:
            return "Wire";

        case GeometryElementKind::Face:
            return "Face";

        case GeometryElementKind::Shell:
            return "Shell";

        case GeometryElementKind::Solid:
            return "Solid";

        case GeometryElementKind::Compound:
            return "Compound";

        case GeometryElementKind::Unknown:
        default:
            return "Unknown";
        }
    }

    struct PickResult
    {
        bool hasShape = false;

        GeometryElementKind elementKind =
            GeometryElementKind::Unknown;

        TopoDS_Shape shape;

        DisplayObjectId sourceDisplayObjectId = -1;
    };

    enum class SelectionTargetKind
    {
        None,

        WorkpieceGeometry,

        DebugContextGroup,
        DebugTracePort,
        DebugTraceStep
    };

    inline const char* selectionTargetKindDisplayName(
        SelectionTargetKind kind)
    {
        switch (kind)
        {
        case SelectionTargetKind::WorkpieceGeometry:
            return "WorkpieceGeometry";

        case SelectionTargetKind::DebugContextGroup:
            return "DebugContextGroup";

        case SelectionTargetKind::DebugTracePort:
            return "DebugTracePort";

        case SelectionTargetKind::DebugTraceStep:
            return "DebugTraceStep";

        case SelectionTargetKind::None:
        default:
            return "None";
        }
    }

    struct CurrentSelection
    {
        SelectionTargetKind targetKind =
            SelectionTargetKind::None;

        GeometryElementKind elementKind =
            GeometryElementKind::Unknown;

        TopoDS_Shape shape;

        int elementIndex = -1;

        DisplayObjectId sourceDisplayObjectId = -1;

        bool isValid() const
        {
            return targetKind != SelectionTargetKind::None
                   && elementKind != GeometryElementKind::Unknown
                   && elementIndex >= 0;
        }
    };
}

#endif // SELECTIONTYPES_H
