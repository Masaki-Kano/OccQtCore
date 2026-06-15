#ifndef DISPLAYTYPES_H
#define DISPLAYTYPES_H

namespace OccQtCore
{
    using DisplayObjectId = int;

    enum class DisplayLayer
    {
        Shape,
        PickHighlight,

        Analysis,
        AnalysisContextGroup,
        AnalysisTracePort,

        Helper,
        Temporary
    };

}

#endif // DISPLAYTYPES_H
