#ifndef HOLEDEBUGPANEL_H
#define HOLEDEBUGPANEL_H

#include <QWidget>
#include <QString>

#include "Feature/HoleRecognitionModel.h"

class QTreeWidgetItem;

namespace Ui
{
    class HoleDebugPanel;
}

class HoleDebugPanel : public QWidget
{
    Q_OBJECT

public:
    explicit HoleDebugPanel(QWidget *parent = nullptr);
    ~HoleDebugPanel() override;

    void setRecognitionResult(const OccQtCore::Feature::HoleRecognitionResult& result);
    void clear();
    void setStatusText(const QString& text);

signals:
    void buildRequested();
    void clearRequested();
    void exportLogRequested();

    void wallSelected(int wallIndex);
    void selectionCleared();

private slots:
    void onBuildClicked();
    void onClearClicked();
    void onExportLogClicked();
    void onTreeItemSelectionChanged();
    void onTreeCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);

private:
    enum class ItemKind
    {
        Unknown = 0,
        WallsRoot,
        Wall
    };

    void setupConnections();

    void populateTree();
    QTreeWidgetItem* populateWallsRoot();

    void showSelectedItemDetail();
    void showWallsRootDetail();
    void showWallDetail(int wallIndex);

private:
    Ui::HoleDebugPanel *ui;
    OccQtCore::Feature::HoleRecognitionResult m_result;
};

#endif // HOLEDEBUGPANEL_H
