#include "StepLoader.h"

#include <IFSelect_ReturnStatus.hxx>
#include <STEPControl_Reader.hxx>

namespace OccQtCore
{
    StepLoadResult StepLoader::load(const QString& filePath)
    {
        StepLoadResult result;

        STEPControl_Reader reader;

        const IFSelect_ReturnStatus status =
            reader.ReadFile(filePath.toLocal8Bit().constData());

        if (status != IFSelect_RetDone)
        {
            result.errorMessage =
                QString("STEPファイルの読み込みに失敗しました: %1").arg(filePath);
            return result;
        }

        const Standard_Integer rootCount = reader.NbRootsForTransfer();

        if (rootCount <= 0)
        {
            result.errorMessage =
                QString("STEPファイルに転送可能なルート形状がありません: %1").arg(filePath);
            return result;
        }

        reader.TransferRoots();

        result.shape = reader.OneShape();

        if (result.shape.IsNull())
        {
            result.errorMessage =
                QString("STEP形状の取得に失敗しました: %1").arg(filePath);
            return result;
        }

        result.success = true;
        return result;
    }
}
