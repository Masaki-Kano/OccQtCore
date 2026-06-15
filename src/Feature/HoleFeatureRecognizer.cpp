#include "Feature/HoleFeatureRecognizer.h"
#include "Feature/HoleContextGeometryGrouper.h"
#include "Feature/HoleContextTracePortBuilder.h"

#include "Geometry/GeometryModel.h"
#include "Geometry/TopologyQuery.h"

#include <qDebug>

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

        HoleContextGeometryGrouper contextGrouper(model);
        result.contextGeometryGroups = contextGrouper.group();

        HoleContextTracePortBuilder tracePortBuilder(model);
        result.contextTracePorts =
            tracePortBuilder.build(result.contextGeometryGroups);

        qDebug() << "ContextGeometryGroups:"
                 << static_cast<int>(result.contextGeometryGroups.size());

        qDebug() << "ContextTracePorts:"
                 << static_cast<int>(result.contextTracePorts.size());

        return result;
    }
}
