#include "StepLoader.h"

#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QDir>
#include <QUuid>

#include <IFSelect_ReturnStatus.hxx>
#include <STEPControl_Reader.hxx>

namespace
{
    bool containsNonAscii(const QString& text)
    {
        for (const QChar ch : text)
        {
            if (ch.unicode() > 0x7F)
            {
                return true;
            }
        }

        return false;
    }

    QString prepareReadablePathForOcc(
        const QString& originalPath,
        QTemporaryDir& tempDir,
        QString* errorMessage)
    {
        const QFileInfo originalInfo(originalPath);

        if (!originalInfo.exists())
        {
            if (errorMessage)
            {
                *errorMessage = QString("STEPファイルが存在しません: %1").arg(originalPath);
            }
            return {};
        }

        // 英数字パスなら、そのままOCCTに渡す
        if (!containsNonAscii(originalPath))
        {
            return originalPath;
        }

        if (!tempDir.isValid())
        {
            if (errorMessage)
            {
                *errorMessage = "STEPファイル読み込み用の一時ファイル作成に失敗しました。";
            }
            return {};
        }

        const QString safeFileName = QString("step_%1.step").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));

        const QString safePath = QDir(tempDir.path()).filePath(safeFileName);

        if (!QFile::copy(originalPath, safePath))
        {
            if (errorMessage)
            {
                *errorMessage = QString("STEPファイルの一時コピーに失敗しました: %1").arg(originalPath);
            }
            return {};
        }

        return safePath;
    }
}

namespace OccQtCore
{
    StepLoadResult StepLoader::load(const QString& filePath)
    {
        StepLoadResult result;

        QTemporaryDir tempDir(QDir::tempPath() + "/OccQtCoreStepLoad_XXXXXX");

        QString prepareError;
        const QString occReadblePath =
            prepareReadablePathForOcc(filePath, tempDir, &prepareError);

        if (occReadblePath.isEmpty())
        {
            result.success = false;
            result.errorMessage = prepareError;
            return result;
        }

        STEPControl_Reader reader;

        const QByteArray pathBytes = QDir::toNativeSeparators(occReadblePath).toLocal8Bit();
        const IFSelect_ReturnStatus status = reader.ReadFile(pathBytes.constData());

        if (status != IFSelect_RetDone)
        {
            result.success = false;
            result.errorMessage = QString("STEPファイルの読み込みに失敗しました: %1").arg(filePath);
            return result;
        }

        reader.TransferRoots();

        result.shape = reader.OneShape();
        result.success = !result.shape.IsNull();

        if (!result.success)
        {
            result.errorMessage = QString("STEPファイルから形状を取得できませんでした: %1").arg(filePath);
        }

        return result;
    }
}
