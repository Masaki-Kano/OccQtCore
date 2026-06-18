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

        TraceSessionsRoot,
        TraceSession,

        ReachedGroupsRoot,
        ReachedGroupLink,

        SessionStepsRoot,

        GroupsRoot,
        Group,

        TraceStep
    };

    void setupConnections();

    void populateTree();

    QTreeWidgetItem* populateTraceSessionsRoot();

    QTreeWidgetItem* populateSessionReachedGroupsRoot(
        QTreeWidgetItem* parentItem,
        const OccQtCore::Feature::HoleContextTraceSession& session);

    QTreeWidgetItem* populateSessionStepsRoot(
        QTreeWidgetItem* parentItem,
        const OccQtCore::Feature::HoleContextTraceSession& session);

    QTreeWidgetItem* populateGroupsRoot();

private:
    void showTraceSessionsRootDetail();
    void showTraceSessionDetail(int sessionIndex);

    void showReachedGroupsRootDetail(int sessionIndex);
    void showReachedGroupLinkDetail(
        int groupIndex,
        int sessionIndex);

    void showSessionStepsRootDetail(int sessionIndex);

    void showGroupsRootDetail();
    void showGroupDetail(int groupIndex);

    void showTraceStepDetail(int stepIndex);

    const OccQtCore::Feature::HoleContextTraceSession*
    findSessionBySessionIndex(int sessionIndex) const;

    const OccQtCore::Feature::HoleContextGeometryGroup*
    findGroupByGroupIndex(int groupIndex) const;

    const OccQtCore::Feature::HoleContextTraceStep*
    findStepByStepIndex(int stepIndex) const;

private:
    Ui::HoleDebugPanel *ui;
    OccQtCore::Feature::HoleRecognitionResult m_result;
};

#endif // HOLEDEBUGPANEL_H
