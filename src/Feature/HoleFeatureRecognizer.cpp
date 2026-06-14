#include "Feature/HoleFeatureRecognizer.h"
#include "Feature/HoleWallBuilder.h"
#include "Feature/HoleWallBoundaryBuilder.h"

#include "Geometry/GeometryModel.h"
#include "Geometry/TopologyQuery.h"

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
        const GeometryModel& model) const
    {
        HoleRecognitionResult result;

        HoleWallBuilder wallBuilder;
        result.walls = wallBuilder.build(model);

        HoleWallBoundaryBuilder boundaryBuilder;
        result.wallBoundaries = boundaryBuilder.build(model, result.walls);

        return result;
    }
}
