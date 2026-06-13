#ifndef HOLEFEATURERECOGNIZER_H
#define HOLEFEATURERECOGNIZER_H

#include <vector>

#include "Feature/FeatureTypes.h"
#include "Feature/HoleRecognitionModel.h"

namespace OccQtCore
{
    class GeometryModel;
}

namespace OccQtCore::Feature
{
    class HoleFeatureRecognizer
    {
    public:
        /**
         * @brief GeometryModelから穴フィーチャを認識する
         *
         * @param model 対象のジオメトリモデル
         * @return 認識済みの穴フィーチャ群
         */
        HoleRecognitionResult recognize(const GeometryModel& model) const;

    };
}

#endif // HOLEFEATURERECOGNIZER_H
