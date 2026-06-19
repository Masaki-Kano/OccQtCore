#ifndef DISPLAYTYPES_H
#define DISPLAYTYPES_H

namespace OccQtCore
{
    using DisplayObjectId = int;

    enum class DisplayLayer
    {
        Model,
        TemporaryOverlay
    };

    enum class DisplayObjectSourceKind
    {
        Unknown,

        Model,

        DebugContextGroup,
        DebugTracePort,
        DebugTraceStep
    };

}

#endif // DISPLAYTYPES_H
