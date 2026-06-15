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

    void groupSelected(int groupTreeIndex);
    void tracePortSelected(int portTreeIndex);

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

        GroupsRoot,
        Group,

        TracePortsRoot,
        TracePort,
    };

    void setupConnections();

    void populateTree();

    QTreeWidgetItem* populateGroupsRoot();

    QTreeWidgetItem* populateTracePortsRoot(
        QTreeWidgetItem* parentItem,
        int sourceGroupIndex);

    void showGroupsRootDetail();
    void showGroupDetail(int groupTreeIndex);

    void showTracePortsRootDetail(int sourceGroupIndex);
    void showTracePortDetail(int portTreeIndex);

    void showGeometryTraceDetail(int traceIndex);

    void showGeometryTraceNodeDetail(
        int traceIndex,
        int nodeIndex);

private:
    Ui::HoleDebugPanel *ui;
    OccQtCore::Feature::HoleRecognitionResult m_result;
};

#endif // HOLEDEBUGPANEL_H
