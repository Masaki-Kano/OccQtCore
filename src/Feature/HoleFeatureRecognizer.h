#ifndef HOLEFEATURERECOGNIZER_H
#define HOLEFEATURERECOGNIZER_H

#include "Feature/HoleRecognitionModel.h"
#include "Feature/HoleRecognitionWorkingData.h"

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

        HoleRecognitionResult recognizeCandidates(
            const GeometryModel& model,
            HoleRecognitionWorkingData* workingData = nullptr) const;

    };
}

#endif // HOLEFEATURERECOGNIZER_H
