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

    void inspectPickedFace(int faceIndex);
    void clearPickedGeometryDetail();

signals:
    void buildRequested();
    void clearRequested();
    void exportLogRequested();

    void groupSelected(int groupTreeIndex);
    void traceStepSelected(int traceIndex);

    void selectionCleared();

private slots:
    void onBuildClicked();
    void onClearClicked();
    void onExportLogClicked();
    void onTreeCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);

private:
    enum class ItemKind
    {
        Unknown,

        TraceRunsRoot,
        TraceRun,

        ReachedGroupsRoot,
        ReachedGroupLink,

        GroupsRoot,
        Group,

        TraceStep
    };

    void setupConnections();

    void populateTree();

    QTreeWidgetItem* populateTraceRunsRoot();

    QTreeWidgetItem* populateRunReachedGroupsRoot(
        QTreeWidgetItem* parentItem,
        const OccQtCore::Feature::HoleContextTraceRun& run);

    QTreeWidgetItem* populateSessionStepsRoot(
        QTreeWidgetItem* parentItem,
        const OccQtCore::Feature::HoleContextTraceRun& session);

    QTreeWidgetItem* populateGroupsRoot();

private:
    void showTraceRunsRootDetail();
    void showTraceRunDetail(int sessionIndex);

    void showReachedGroupsRootDetail(int sessionIndex);
    void showReachedGroupLinkDetail(
        int groupIndex,
        int sessionIndex);

    void showGroupsRootDetail();
    void showGroupDetail(int groupIndex);

    void showTraceStepDetail(int stepIndex);

private:
    Ui::HoleDebugPanel *ui;
    OccQtCore::Feature::HoleRecognitionResult m_result;
};

#endif // HOLEDEBUGPANEL_H
