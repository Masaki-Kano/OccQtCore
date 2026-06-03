#ifndef HOLEDEBUGPANEL_H
#define HOLEDEBUGPANEL_H

#include <QWidget>

#include "Debug/HoleDebugOptions.h"

namespace Ui {
class HoleDebugPanel;
}

class HoleDebugPanel : public QWidget
{
    Q_OBJECT

public:
    explicit HoleDebugPanel(QWidget *parent = nullptr);
    ~HoleDebugPanel();

    OccQtCore::Debug::HoleDebugDisplayOptions displayOptions() const;
    void setDisplayOptions(const OccQtCore::Debug::HoleDebugDisplayOptions& options);

    bool isDebugEnabled() const;
    void setDebugEnabled(bool enabled);

signals:
    void debugEnabledChanged(bool enabled);
    void displayOptionsChanged(const OccQtCore::Debug::HoleDebugDisplayOptions& options);

    void refreshRequested();
    void rebuildRequested();

private:
    void setupConnections();
    void emitDisplayOptionsChanged();

private:
    Ui::HoleDebugPanel *ui;
};

#endif // HOLEDEBUGPANEL_H
