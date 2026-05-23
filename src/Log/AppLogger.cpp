#include "AppLogger.h"

namespace OccQtCore
{
    AppLogger::AppLogger(QObject* parent)
        : QObject(parent)
    {
    }

    void AppLogger::info(const QString& text)
    {
        addMessage(LogLevel::Info, text);
    }

    void AppLogger::warn(const QString& text)
    {
        addMessage(LogLevel::Warning, text);
    }

    void AppLogger::error(const QString& text)
    {
        addMessage(LogLevel::Error, text);
    }

    void AppLogger::debug(const QString& text)
    {
        addMessage(LogLevel::Debug, text);
    }

    void AppLogger::addMessage(LogLevel level, const QString& text)
    {
        LogMessage message;
        message.level = level;
        message.text = text;
        message.time = QDateTime::currentDateTime();

        emit messageAdded(message);
    }
}
