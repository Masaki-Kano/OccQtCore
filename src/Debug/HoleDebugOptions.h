#ifndef HOLEDEBUGOPTIONS_H
#define HOLEDEBUGOPTIONS_H

namespace OccQtCore::Debug
{
    enum class HoleDebugDisplayScope
    {
        AllCandidates,
        SelectedCandidate
    };

    struct HoleDebugSelectedCandidate
    {
        enum class Type
        {
            None,
            Wall,
            End,
            Segment,
            Hole
        };

        Type type = Type::None;
        int index = -1;
    };

    struct HoleDebugDisplayOptions
    {
        HoleDebugDisplayScope scope =
            HoleDebugDisplayScope::SelectedCandidate;

        HoleDebugSelectedCandidate selectedCandidate;
    };

    struct HoleDebugLogOptions
    {
        bool logSummary = true;
        bool logWalls = true;
        bool logEnds = true;
        bool logSegments = true;
        bool logCandidates = true;
    };
}

#endif // HOLEDEBUGOPTIONS_H
