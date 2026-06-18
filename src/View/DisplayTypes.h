#ifndef DISPLAYTYPES_H
#define DISPLAYTYPES_H

namespace OccQtCore
{
    using DisplayObjectId = int;

    enum class DisplayLayer
    {
        Model,
        PickOverlay,
        TemporaryOverlay
    };

    enum class DisplayObjectSourceKind
    {
        Unknown,

        Model,
        Pick,

        DebugContextGroup,
        DebugTracePort,
        DebugTraceStep
    };

}

#endif // DISPLAYTYPES_H
