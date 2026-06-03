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
        bool logWalls = true;
        bool logEnds = true;
        bool logSegments = true;
    };
}

#endif // HOLEDEBUGOPTIONS_H
