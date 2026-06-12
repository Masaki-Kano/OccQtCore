#ifndef HOLEDEBUGOPTIONS_H
#define HOLEDEBUGOPTIONS_H

namespace OccQtCore::Debug
{
    enum class HoleDebugDisplayScope
    {
        All,
        Selected
    };

    struct HoleDebugSelectedItem
    {
        enum class Type
        {
            None,

            Hole,
            Element,
            Wall,
            End,
        };

        Type type = Type::None;

        int holeIndex = -1;
        int elementIndex = -1;
        int childIndex = -1;
    };

    struct HoleDebugDisplayOptions
    {
        HoleDebugDisplayScope scope =
            HoleDebugDisplayScope::Selected;

        HoleDebugSelectedItem selectedItem;
    };

    struct HoleDebugLogOptions
    {
        bool logSummary = true;

        bool logHoles = true;
        bool logElements = true;
        bool logWalls = true;
        bool logEnds = true;
        bool logConnections = true;

        bool logSourceCandidates = true;
    };
}

#endif // HOLEDEBUGOPTIONS_H
