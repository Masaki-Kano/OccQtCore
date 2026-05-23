#ifndef LOGMESSAGE_H
#define LOGMESSAGE_H

#include <QString>
#include <QDateTime>

namespace OccQtCore
{
    enum class LogLevel
    {
        Info,
        Warning,
        Error,
        Debug
    };

    struct LogMessage
    {
        LogLevel level = LogLevel::Info;
        QString text;
        QDateTime time;
    };
}

#endif // LOGMESSAGE_H
