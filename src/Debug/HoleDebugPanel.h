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
    void boundarySelected(int boundaryIndex);
    void geometryTraceNodeSelected(int traceIndex, int nodeIndex);

    void selectionCleared();

private slots:
    void onBuildClicked();
    void onClearClicked();
    void onExportLogClicked();
    void onTreeCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);

private:
    enum class ItemKind
    {
        Unknown = 0,
        WallsRoot,
        Wall,

        WallBoundariesRoot,
        WallBoundary,

        GeometryTrace,
        GeometryTraceNode
    };

    void setupConnections();

    void populateTree();
    QTreeWidgetItem* populateWallsRoot();
    QTreeWidgetItem* populateWallBoundariesRoot(QTreeWidgetItem* parentItem, int wallIndex);

    void populateGeometryTracesOfBoundary(QTreeWidgetItem* boundaryItem, int boundaryIndex);
    void populateGeometryTraceNodes(QTreeWidgetItem* traceItem, int traceIndex);

    void showSelectedItemDetail();
    void showWallsRootDetail();
    void showWallDetail(int wallIndex);
    void showWallBoundariesRootDetail(int wallIndex);
    void showWallBoundaryDetail(int boundaryIndex);
    void showGeometryTraceDetail(int traceIndex);
    void showGeometryTraceNodeDetail(int traceIndex, int nodeIndex);

private:
    Ui::HoleDebugPanel *ui;
    OccQtCore::Feature::HoleRecognitionResult m_result;
};

#endif // HOLEDEBUGPANEL_H
