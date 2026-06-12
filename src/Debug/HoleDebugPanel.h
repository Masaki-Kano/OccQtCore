#ifndef HOLEDEBUGPANEL_H
#define HOLEDEBUGPANEL_H

#include <QWidget>
#include <QString>

#include "Debug/HoleDebugOptions.h"
#include "Feature/HoleRecognitionTypes.h"

namespace Ui {
class HoleDebugPanel;
}

namespace OccQtCore::Feature {
struct HoleRecognitionResult;
}

class QTreeWidgetItem;

class HoleDebugPanel : public QWidget
{
    Q_OBJECT

public:
    explicit HoleDebugPanel(QWidget *parent = nullptr);
    ~HoleDebugPanel();

    OccQtCore::Debug::HoleDebugDisplayOptions displayOptions() const;
    void setDisplayOptions(const OccQtCore::Debug::HoleDebugDisplayOptions& options);

    void setStatusText(const QString& text);
    void setRecognitionResult(const OccQtCore::Feature::HoleRecognitionResult& result);

signals:
    void buildRequested();
    void refreshDisplayRequested();
    void clearDisplayRequested();
    void exportDetailLogRequested();

    void displayOptionsChanged(const OccQtCore::Debug::HoleDebugDisplayOptions& options);

    void selectedItemChanged(const OccQtCore::Debug::HoleDebugSelectedItem& selected);

private:
    void setupInitialState();
    void setupConnections();
    void emitDisplayOptionsChanged();

    void populateHoleTree(const OccQtCore::Feature::HoleRecognitionResult& result);

    void updateSelectionDetail(QTreeWidgetItem* item);
    OccQtCore::Debug::HoleDebugSelectedItem selectedItemFromTreeItem(QTreeWidgetItem* item) const;

    QString buildHoleDetailText(int holeIndex) const;
    QString buildElementDetailText(int holeIndex, int elementIndex) const;
    QString buildEndDetailText(int holeIndex, int elementIndex, int endIndex) const;
    QString buildWallDetailText(int holeIndex, int elementIndex) const;

private:
    Ui::HoleDebugPanel *ui;
    OccQtCore::Feature::HoleRecognitionResult m_result;
    bool m_hasResult = false;
};

#endif // HOLEDEBUGPANEL_H
