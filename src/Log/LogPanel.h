#ifndef LOGPANEL_H
#define LOGPANEL_H

#include <QWidget>

class QPlainTextEdit;

namespace OccQtCore
{
    class AppLogger;
    struct LogMessage;

    class LogPanel : public QWidget
    {
        Q_OBJECT

    public:
        explicit LogPanel(QWidget* parent = nullptr);

        void setLogger(AppLogger* logger);

    private slots:
        void onMessageAdded(const OccQtCore::LogMessage& message);

    private:
        QString formatMessage(const OccQtCore::LogMessage& message) const;

    private:
        QPlainTextEdit* m_textEdit = nullptr;
        AppLogger* m_logger = nullptr;
    };
}

#endif // LOGPANEL_H
