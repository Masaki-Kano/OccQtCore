#include "Log/AppLogReporter.h"
#include "Log/AppLogger.h"

namespace OccQtCore
{
    AppLogReporter::AppLogReporter(AppLogger* logger)
        : m_logger(logger)
        , m_geometryReporter(logger)
        , m_holeRecognitionReporter(logger)
    {
    }

    void AppLogReporter::logSelection(const CurrentSelection& currentSelection) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

        if (!currentSelection.isValid())
        {
            m_logger->info("選択: なし");
            return;
        }

        m_logger->info(
            QString("選択: %1, インデックス=%2, 表示オブジェクトID=%3")
                .arg(
                    geometryElementKindDisplayName(
                        currentSelection.elementKind))
                .arg(currentSelection.elementIndex)
                .arg(currentSelection.sourceDisplayObjectId));
    }

    void AppLogReporter::logStepLoaded(const QString& filePath) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

        m_logger->info(QString("STEPファイルを読み込みました: %1").arg(filePath));
    }

    void AppLogReporter::logStepLoadFailed(const QString& errorMessage) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

        m_logger->error(QString("STEPファイルの読み込みに失敗しました: %1")
                            .arg(errorMessage));
    }

    void AppLogReporter::logActionStarted(const QString& actionName) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

        m_logger->info(QString("操作開始: %1").arg(actionName));
    }

    void AppLogReporter::logActionFinished(const QString& actionName) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

        m_logger->info(QString("操作完了: %1").arg(actionName));
    }

    void AppLogReporter::logActionFailed(
        const QString& actionName,
        const QString& reason) const
    {
        if (m_logger == nullptr)
        {
            return;
        }

        m_logger->error(
            QString("操作失敗: %1, 理由=%2")
            .arg(actionName)
            .arg(reason));
    }

    void AppLogReporter::logGeometry(const GeometryLogReport& report) const
    {
        m_geometryReporter.logGeometry(report);
    }

    void AppLogReporter::logGeometryElement(const GeometryElementLogReport& report) const
    {
        m_geometryReporter.logGeometryElement(report);
    }

    void AppLogReporter::logHoleRecognition(const HoleRecognitionLogReport& report) const
    {
        m_holeRecognitionReporter.logHoleRecognition(report);
    }
}
