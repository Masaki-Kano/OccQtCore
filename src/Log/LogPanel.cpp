#include "LogPanel.h"
#include "AppLogger.h"
#include "LogMessage.h"

#include <QPlainTextEdit>
#include <QVBoxLayout>

namespace OccQtCore
{
    LogPanel::LogPanel(QWidget* parent)
        : QWidget(parent)
    {
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);

        m_textEdit = new QPlainTextEdit(this);
        m_textEdit->setReadOnly(true);

        layout->addWidget(m_textEdit);
    }

    void LogPanel::setLogger(AppLogger* logger)
    {
        if (m_logger)
        {
            disconnect(m_logger, nullptr, this, nullptr);
        }

        m_logger = logger;

        if (m_logger)
        {
            connect(m_logger, &AppLogger::messageAdded,
                    this, &LogPanel::onMessageAdded);
        }
    }

    void LogPanel::onMessageAdded(const OccQtCore::LogMessage& message)
    {
        m_textEdit->appendPlainText(formatMessage(message));
    }

    QString LogPanel::formatMessage(const OccQtCore::LogMessage& message) const
    {
        QString levelText;

        switch (message.level)
        {
        case LogLevel::Info:
            levelText = "INFO";
            break;
        case LogLevel::Warning:
            levelText = "WARN";
            break;
        case LogLevel::Error:
            levelText = "ERROR";
            break;
        case LogLevel::Debug:
            levelText = "DEBUG";
            break;
        }

        return QString("[%1] [%2] %3")
            .arg(message.time.toString("HH:mm:ss"))
            .arg(levelText)
            .arg(message.text);
    }
}
