#ifndef HOLEDEBUGOPTIONS_H
#define HOLEDEBUGOPTIONS_H

namespace OccQtCore::Debug
{
    struct HoleDebugDisplayOptions
    {
        int targetSegmentIndex = -1;
        int targetWallCandidateIndex = -1;

        bool showWallFaces = true;

        bool showRepresentativeEnds = true;
        bool showRawEnds = false;

        bool showOpenEnds = true;
        bool showBottomEnds = true;
        bool showWallConnectionEnds = true;
    };

    struct HoleDebugLogOptions
    {
        bool logWalls = false;
        bool logEnds = false;
        bool logSegments = true;
        bool logCandidates = true;
    };
}

#endif // HOLEDEBUGOPTIONS_H
