#ifndef HOLERECOGNITIONWORKINGDATA_H
#define HOLERECOGNITIONWORKINGDATA_H

#include <string>
#include <vector>

namespace OccQtCore::Feature
{
    enum class CylindricalWallPromotionRejectReason
    {
        None,

        NotWallCandidateKind,
        EmptyFaces,
        MissingReference,
        InvalidRadius,

        InsufficientCircumferentialCoverage,
        MissingTopologicalCircumferentialLoop,
        NoInnerCylindricalFace
    };

    struct CylindricalWallPromotionResult
    {
        bool accepted = false;

        CylindricalWallPromotionRejectReason rejectReason =
            CylindricalWallPromotionRejectReason::None;
    };

    struct CylindricalWorkingGroupDebugInfo
    {
        int index = -1;

        std::vector<int> faceIndices;

        double radius = 0.0;

        CylindricalWallPromotionResult promotion;

        std::string note;
    };

    struct HoleRecognitionWorkingData
    {
        std::vector<CylindricalWorkingGroupDebugInfo>
            cylindricalWorkingGroups;

        void clear()
        {
            cylindricalWorkingGroups.clear();
        }
    };
}

#endif // HOLERECOGNITIONWORKINGDATA_H
