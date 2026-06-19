#ifndef APPLOGGER_H
#define APPLOGGER_H

#include <QObject>

#include "LogMessage.h"

namespace OccQtCore
{
    class AppLogger : public QObject
    {
        Q_OBJECT

    public:
        explicit AppLogger(QObject* parent = nullptr);

        void info(const QString& text);
        void warn(const QString& text);
        void error(const QString& text);
        void debug(const QString& text);

    signals:
        void messageAdded(const OccQtCore::LogMessage& message);

    private:
        void addMessage(LogLevel level, const QString& text);
    };
}

#endif // APPLOGGER_H
