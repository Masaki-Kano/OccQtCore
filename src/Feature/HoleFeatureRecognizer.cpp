#include "Feature/HoleFeatureRecognizer.h"
#include "Feature/HoleContextTraceExplorer.h"

#include "Geometry/GeometryModel.h"

namespace OccQtCore::Feature
{
    HoleRecognitionResult HoleFeatureRecognizer::recognize(
        const GeometryModel& model) const
    {
        (void)model;

        // TODO:
        // V2 の HoleAssembly / HoleFeature 生成までできたら、
        // Hole::Data へ変換する。
        return {};
    }

    HoleRecognitionResult HoleFeatureRecognizer::recognizeCandidates(
        const GeometryModel& model,
        HoleRecognitionWorkingData* workingData) const
    {
        if (workingData != nullptr)
        {
            workingData->clear();
        }

        HoleContextTraceExplorer explorer(
            model,
            workingData);

        return explorer.explore();
    }
}
