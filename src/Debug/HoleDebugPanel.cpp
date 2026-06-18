#include "Debug/HoleDebugPanel.h"

#include "ui_HoleDebugPanel.h"

#include <QAbstractItemView>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QString>
#include <QStringList>
#include <QTreeWidget>
#include <QTreeWidgetItem>

namespace
{
    constexpr int ItemKindRole = Qt::UserRole + 1;
    constexpr int ItemIndexRole = Qt::UserRole + 2;
    constexpr int ItemSubIndexRole = Qt::UserRole + 3;

    QString formatIntList(const std::vector<int>& values)
    {
        if (values.empty())
        {
            return "なし";
        }

        QStringList texts;
        for (const int value : values)
        {
            texts << QString::number(value);
        }

        return texts.join(", ");
    }

    QString formatPoint(const gp_Pnt& point)
    {
        return QString("(%1, %2, %3)")
        .arg(point.X(), 0, 'f', 4)
            .arg(point.Y(), 0, 'f', 4)
            .arg(point.Z(), 0, 'f', 4);
    }

    QString formatDirection(const gp_Dir& direction)
    {
        return QString("(%1, %2, %3)")
        .arg(direction.X(), 0, 'f', 6)
            .arg(direction.Y(), 0, 'f', 6)
            .arg(direction.Z(), 0, 'f', 6);
    }

    QString formatGroupIndexList(const std::vector<int>& values)
    {
        if (values.empty())
        {
            return "なし";
        }

        QStringList texts;

        for (const int value : values)
        {
            texts << QString("Group[%1]").arg(value);
        }

        return texts.join(", ");
    }

    QString toString(OccQtCore::Feature::HoleContextGeometryGroupKind kind)
    {
        using Kind = OccQtCore::Feature::HoleContextGeometryGroupKind;

        switch (kind)
        {
        case Kind::Unknown:
            return "Unknown";
        case Kind::WallCandidate:
            return "WallCandidate";
        case Kind::BoundaryCandidate:
            return "BoundaryCandidate";
        case Kind::TransitionCandidate:
            return "TransitionCandidate";
        case Kind::Ambiguous:
            return "Ambiguous";
        }

        return "Unknown";
    }
}

HoleDebugPanel::HoleDebugPanel(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::HoleDebugPanel)
{
    ui->setupUi(this);

    ui->plainDetail->setReadOnly(true);
    ui->plainDetail->setLineWrapMode(QPlainTextEdit::NoWrap);

    ui->plainPickedGeometryDetail->setReadOnly(true);
    ui->plainPickedGeometryDetail->setLineWrapMode(QPlainTextEdit::NoWrap);

    ui->treeItems->setHeaderHidden(true);
    ui->treeItems->setSelectionMode(QAbstractItemView::SingleSelection);

    setupConnections();
    clear();

    setStatusText("Ready");
}

HoleDebugPanel::~HoleDebugPanel()
{
    delete ui;
}

void HoleDebugPanel::setRecognitionResult(const OccQtCore::Feature::HoleRecognitionResult& result)
{
    m_result = result;

    populateTree();

    setStatusText(
        QString("Build completed. Runs: %1, Groups: %2, Steps: %3")
            .arg(static_cast<int>(m_result.contextTrace.runs.size()))
            .arg(static_cast<int>(m_result.contextGeometryGroups.size()))
            .arg(static_cast<int>(m_result.contextTrace.steps.size())));
}

void HoleDebugPanel::clear()
{
    m_result = OccQtCore::Feature::HoleRecognitionResult{};

    ui->treeItems->clear();
    ui->plainDetail->clear();

    populateTree();

    setStatusText("Cleared");
}

void HoleDebugPanel::setStatusText(const QString& text)
{
    ui->lblStatus->setText(text);
}

void HoleDebugPanel::inspectPickedFace(int faceIndex)
{
    QString text;

    text += QString("Picked Face[%1]\n").arg(faceIndex);
    text += "================\n\n";

    text += "Groups containing this face:\n";

    bool foundGroup = false;

    for (const auto& group : m_result.contextGeometryGroups)
    {
        if (std::find(
                group.geometryRefs.faceIndices.begin(),
                group.geometryRefs.faceIndices.end(),
                faceIndex) == group.geometryRefs.faceIndices.end())
        {
            continue;
        }

        foundGroup = true;

        text += QString("  Group[%1] %2 Faces=%3\n")
                    .arg(group.index)
                    .arg(toString(group.kind))
                    .arg(formatIntList(group.geometryRefs.faceIndices));
    }

    if (!foundGroup)
    {
        text += "  なし\n";
    }

    text += "\nSteps observing this face:\n";

    bool foundStep = false;

    for (const auto& step : m_result.contextTrace.steps)
    {
        const bool inOutside =
            std::find(
                step.outsideGeometryRefs.faceIndices.begin(),
                step.outsideGeometryRefs.faceIndices.end(),
                faceIndex) != step.outsideGeometryRefs.faceIndices.end();

        const bool inPort =
            std::find(
                step.portGeometryRefs.faceIndices.begin(),
                step.portGeometryRefs.faceIndices.end(),
                faceIndex) != step.portGeometryRefs.faceIndices.end();

        if (!inOutside && !inPort)
        {
            continue;
        }

        foundStep = true;

        const int runIndex =
            findRunIndexByStepIndex(step.index);

        text += QString("  Step[%1] Run[%2] SourceGroup[%3] SourcePort[%4]")
                    .arg(step.index)
                    .arg(runIndex)
                    .arg(step.sourceGroupIndex)
                    .arg(step.sourcePortIndex);
        if (inOutside)
        {
            text += " Outside";
        }

        if (inPort)
        {
            text += " Port";
        }

        text += "\n";
    }

    if (!foundStep)
    {
        text += "  なし\n";
    }

    text += "\nRuns reaching related groups:\n";

    bool foundRun = false;

    for (const auto& run : m_result.contextTrace.runs)
    {
        bool related = false;

        for (const int groupIndex : run.reachedGroupIndices)
        {
            const auto* group =
                findGroupByGroupIndex(groupIndex);

            if (group == nullptr)
            {
                continue;
            }

            if (std::find(
                    group->geometryRefs.faceIndices.begin(),
                    group->geometryRefs.faceIndices.end(),
                    faceIndex) != group->geometryRefs.faceIndices.end())
            {
                related = true;
                break;
            }
        }

        if (!related)
        {
            continue;
        }

        foundRun = true;

        text += QString("  Run[%1] StartGroup[%2] Groups=%3 Walls=%4\n")
                    .arg(run.index)
                    .arg(run.startGroupIndex)
                    .arg(static_cast<int>(run.reachedGroupIndices.size()))
                    .arg(static_cast<int>(run.reachedWallGroupIndices.size()));
    }

    if (!foundRun)
    {
        text += "  なし\n";
    }

    ui->plainPickedGeometryDetail->setPlainText(text);
}

void HoleDebugPanel::clearPickedGeometryDetail()
{
    ui->plainPickedGeometryDetail->clear();
}

void HoleDebugPanel::onBuildClicked()
{
    setStatusText("Building...");
    emit buildRequested();
}

void HoleDebugPanel::onClearClicked()
{
    clear();
    emit clearRequested();
    emit selectionCleared();
}

void HoleDebugPanel::onExportLogClicked()
{
    qDebug() << "HoleDebugPanel::onExportLogClicked";
    emit exportLogRequested();
}

void HoleDebugPanel::onTreeCurrentItemChanged(
    QTreeWidgetItem* current,
    QTreeWidgetItem* previous)
{
    Q_UNUSED(previous);

    if (current == nullptr)
    {
        ui->plainDetail->clear();
        emit selectionCleared();
        return;
    }

    const auto kind =
        static_cast<ItemKind>(
            current->data(0, ItemKindRole).toInt());

    const int index =
        current->data(0, ItemIndexRole).toInt();

    const int subIndex =
        current->data(0, ItemSubIndexRole).toInt();

    switch (kind)
    {
    case ItemKind::TraceSessionsRoot:
        showTraceSessionsRootDetail();
        emit selectionCleared();
        break;

    case ItemKind::TraceSession:
        showTraceSessionDetail(index);
        emit selectionCleared();
        break;

    case ItemKind::ReachedGroupsRoot:
        showReachedGroupsRootDetail(index);
        emit selectionCleared();
        break;

    case ItemKind::ReachedGroupLink:
        showReachedGroupLinkDetail(index, subIndex);
        emit groupSelected(index);
        break;

    case ItemKind::GroupsRoot:
        showGroupsRootDetail();
        emit selectionCleared();
        break;

    case ItemKind::Group:
        showGroupDetail(index);
        emit groupSelected(index);
        break;

    case ItemKind::Unknown:
    default:
        ui->plainDetail->clear();
        emit selectionCleared();
        break;
    }
}

void HoleDebugPanel::setupConnections()
{
    connect(ui->btnBuild, &QPushButton::clicked,
            this, &HoleDebugPanel::onBuildClicked);

    connect(ui->btnClear, &QPushButton::clicked,
            this, &HoleDebugPanel::onClearClicked);

    connect(ui->btnExportLog, &QPushButton::clicked,
            this, &HoleDebugPanel::onExportLogClicked);

    connect(ui->treeItems,
            &QTreeWidget::currentItemChanged,
            this,
            &HoleDebugPanel::onTreeCurrentItemChanged);
}

void HoleDebugPanel::populateTree()
{
    ui->treeItems->clear();
    ui->plainDetail->clear();

    auto* sessionsRoot = populateTraceSessionsRoot();
    sessionsRoot->setExpanded(true);

    auto* groupsRoot = populateGroupsRoot();

    for (const auto& group : m_result.contextGeometryGroups)
    {
        auto* groupItem = new QTreeWidgetItem(groupsRoot);

        groupItem->setText(
            0,
            QString("Group[%1] %2")
                .arg(group.index)
                .arg(toString(group.kind)));

        groupItem->setData(
            0,
            ItemKindRole,
            static_cast<int>(ItemKind::Group));

        groupItem->setData(0, ItemIndexRole, group.index);
        groupItem->setData(0, ItemSubIndexRole, -1);
    }

    groupsRoot->setExpanded(false);
}

QTreeWidgetItem* HoleDebugPanel::populateTraceSessionsRoot()
{
    auto* rootItem = new QTreeWidgetItem(ui->treeItems);

    rootItem->setText(
        0,
        QString("Trace Runs (%1)")
            .arg(static_cast<int>(m_result.contextTrace.runs.size())));

    rootItem->setData(
        0,
        ItemKindRole,
        static_cast<int>(ItemKind::TraceSessionsRoot));

    rootItem->setData(0, ItemIndexRole, -1);
    rootItem->setData(0, ItemSubIndexRole, -1);

    for (const auto& run : m_result.contextTrace.runs)
    {
        auto* runItem = new QTreeWidgetItem(rootItem);

        runItem->setText(
            0,
            QString("Run[%1] start=Group[%2] Groups=%3 Walls=%4")
                .arg(run.index)
                .arg(run.startGroupIndex)
                .arg(static_cast<int>(run.reachedGroupIndices.size()))
                .arg(static_cast<int>(run.reachedWallGroupIndices.size())));

        runItem->setData(
            0,
            ItemKindRole,
            static_cast<int>(ItemKind::TraceSession));

        runItem->setData(0, ItemIndexRole, run.index);
        runItem->setData(0, ItemSubIndexRole, -1);

        populateSessionReachedGroupsRoot(runItem, run);
    }

    rootItem->setExpanded(true);

    return rootItem;
}

QTreeWidgetItem* HoleDebugPanel::populateSessionReachedGroupsRoot(
    QTreeWidgetItem* parentItem,
    const OccQtCore::Feature::HoleContextTraceRun& run)
{
    auto* rootItem = new QTreeWidgetItem(parentItem);

    rootItem->setText(
        0,
        QString("Run Groups (%1)")
            .arg(static_cast<int>(run.reachedGroupIndices.size())));

    rootItem->setData(
        0,
        ItemKindRole,
        static_cast<int>(ItemKind::ReachedGroupsRoot));

    rootItem->setData(0, ItemIndexRole, run.index);
    rootItem->setData(0, ItemSubIndexRole, -1);

    for (const int groupIndex : run.reachedGroupIndices)
    {
        const auto* group =
            findGroupByGroupIndex(groupIndex);

        auto* groupItem = new QTreeWidgetItem(rootItem);

        if (group != nullptr)
        {
            groupItem->setText(
                0,
                QString("Group[%1] %2 Faces=%3")
                    .arg(group->index)
                    .arg(toString(group->kind))
                    .arg(formatIntList(group->geometryRefs.faceIndices)));
        }
        else
        {
            groupItem->setText(
                0,
                QString("Group[%1] <missing>")
                    .arg(groupIndex));
        }

        groupItem->setData(
            0,
            ItemKindRole,
            static_cast<int>(ItemKind::ReachedGroupLink));

        groupItem->setData(0, ItemIndexRole, groupIndex);
        groupItem->setData(0, ItemSubIndexRole, run.index);
    }

    rootItem->setExpanded(false);

    return rootItem;
}

QTreeWidgetItem* HoleDebugPanel::populateGroupsRoot()
{
    auto* groupsRoot = new QTreeWidgetItem(ui->treeItems);

    groupsRoot->setText(
        0,
        QString("Context Groups (%1)")
            .arg(static_cast<int>(m_result.contextGeometryGroups.size())));

    groupsRoot->setData(
        0,
        ItemKindRole,
        static_cast<int>(ItemKind::GroupsRoot));

    groupsRoot->setData(0, ItemIndexRole, -1);
    groupsRoot->setData(0, ItemSubIndexRole, -1);
    groupsRoot->setExpanded(true);

    return groupsRoot;
}

void HoleDebugPanel::showTraceSessionsRootDetail()
{
    QString text;

    text += "種別: TraceRunsRoot\n";
    text += QString("RunCount: %1\n")
                .arg(static_cast<int>(m_result.contextTrace.runs.size()));
    text += QString("GroupCount: %1\n")
                .arg(static_cast<int>(m_result.contextGeometryGroups.size()));
    text += QString("StepCount: %1\n")
                .arg(static_cast<int>(m_result.contextTrace.steps.size()));

    ui->plainDetail->setPlainText(text);
}

void HoleDebugPanel::showTraceSessionDetail(int runIndex)
{
    const auto* run =
        findRunByRunIndex(runIndex);

    if (run == nullptr)
    {
        ui->plainDetail->setPlainText("Invalid trace run index.");
        return;
    }

    QString text;

    text += "種別: ContextTraceRun\n";
    text += QString("RunIndex: %1\n").arg(run->index);
    text += QString("StartGroupIndex: %1\n").arg(run->startGroupIndex);
    text += QString("ReachedGroupCount: %1\n")
                .arg(static_cast<int>(run->reachedGroupIndices.size()));
    text += QString("ReachedWallCount: %1\n")
                .arg(static_cast<int>(run->reachedWallGroupIndices.size()));
    text += QString("StepCount: %1\n")
                .arg(static_cast<int>(run->traceStepIndices.size()));
    text += QString("PortCount: %1\n")
                .arg(static_cast<int>(run->tracePortIndices.size()));
    text += QString("Completed: %1\n")
                .arg(run->completed ? "true" : "false");

    text += "\nReachedGroups:\n";
    text += formatGroupIndexList(run->reachedGroupIndices);
    text += "\n";

    text += "\nReachedWalls:\n";
    text += formatGroupIndexList(run->reachedWallGroupIndices);
    text += "\n";

    text += "\nTracePorts:\n";
    text += formatIntList(run->tracePortIndices);
    text += "\n";

    if (!run->note.empty())
    {
        text += "\nNote:\n";
        text += QString::fromStdString(run->note);
        text += "\n";
    }

    ui->plainDetail->setPlainText(text);
}

void HoleDebugPanel::showReachedGroupsRootDetail(int runIndex)
{
    const auto* run =
        findRunByRunIndex(runIndex);

    if (run == nullptr)
    {
        ui->plainDetail->setPlainText("Invalid trace run index.");
        return;
    }

    QString text;

    text += "種別: ReachedGroupsRoot\n";
    text += QString("RunIndex: %1\n").arg(run->index);
    text += QString("ReachedGroupCount: %1\n")
                .arg(static_cast<int>(run->reachedGroupIndices.size()));

    ui->plainDetail->setPlainText(text);
}

void HoleDebugPanel::showReachedGroupLinkDetail(
    int groupIndex,
    int runIndex)
{
    const auto* group =
        findGroupByGroupIndex(groupIndex);

    QString text;

    text += "種別: ReachedGroupLink\n";
    text += QString("SessionIndex: %1\n").arg(runIndex);
    text += QString("GroupIndex: %1\n").arg(groupIndex);

    if (group == nullptr)
    {
        text += "\nGroup not found.\n";
        ui->plainDetail->setPlainText(text);
        return;
    }

    text += QString("Kind: %1\n").arg(toString(group->kind));

    text += "\nGeometryRefs:\n";
    text += QString("Faces: %1\n").arg(formatIntList(group->geometryRefs.faceIndices));
    text += QString("Wires: %1\n").arg(formatIntList(group->geometryRefs.wireIndices));
    text += QString("Edges: %1\n").arg(formatIntList(group->geometryRefs.edgeIndices));
    text += QString("Vertices: %1\n").arg(formatIntList(group->geometryRefs.vertexIndices));

    if (!group->note.empty())
    {
        text += "\nNote:\n";
        text += QString::fromStdString(group->note);
        text += "\n";
    }

    ui->plainDetail->setPlainText(text);
}

void HoleDebugPanel::showGroupsRootDetail()
{
    QString text;

    text += "種別: ContextGroupsRoot\n";
    text += QString("GroupCount: %1\n")
                .arg(static_cast<int>(m_result.contextGeometryGroups.size()));
    text += QString("RunCount: %1\n")
                .arg(static_cast<int>(m_result.contextTrace.runs.size()));
    text += QString("StepCount: %1\n")
                .arg(static_cast<int>(m_result.contextTrace.steps.size()));

    ui->plainDetail->setPlainText(text);
}

void HoleDebugPanel::showGroupDetail(int groupIndex)
{
    const auto* group =
        findGroupByGroupIndex(groupIndex);

    if (group == nullptr)
    {
        ui->plainDetail->setPlainText("Invalid context group index.");
        return;
    }

    QString text;

    text += "種別: ContextGeometryGroup\n";
    text += QString("GroupIndex: %1\n").arg(group->index);
    text += QString("Kind: %1\n").arg(toString(group->kind));

    text += "\nGeometryRefs:\n";
    text += QString("Faces: %1\n").arg(formatIntList(group->geometryRefs.faceIndices));
    text += QString("Wires: %1\n").arg(formatIntList(group->geometryRefs.wireIndices));
    text += QString("Edges: %1\n").arg(formatIntList(group->geometryRefs.edgeIndices));
    text += QString("Vertices: %1\n").arg(formatIntList(group->geometryRefs.vertexIndices));

    text += "\nGeometry:\n";
    text += QString("HasAxis: %1\n").arg(group->hasReferenceDirection ? "true" : "false");

    if (group->hasReferenceDirection)
    {
        text += QString("AxisPoint: %1\n").arg(formatPoint(group->referencePoint));
        text += QString("AxisDirection: %1\n").arg(formatDirection(group->referenceDirection));
        text += QString("Radius: %1\n").arg(group->radius, 0, 'f', 4);
        text += QString("AxialMin: %1\n").arg(group->parameterMin, 0, 'f', 4);
        text += QString("AxialMax: %1\n").arg(group->parameterMax, 0, 'f', 4);
        text += QString("AxialPosition: %1\n").arg(group->parameterPosition, 0, 'f', 4);
    }

    if (!group->note.empty())
    {
        text += "\nNote:\n";
        text += QString::fromStdString(group->note);
        text += "\n";
    }

    ui->plainDetail->setPlainText(text);
}

const OccQtCore::Feature::HoleContextTraceRun*
HoleDebugPanel::findRunByRunIndex(int runIndex) const
{
    for (const auto& run : m_result.contextTrace.runs)
    {
        if (run.index == runIndex)
        {
            return &run;
        }
    }

    return nullptr;
}

const OccQtCore::Feature::HoleContextGeometryGroup*
HoleDebugPanel::findGroupByGroupIndex(int groupIndex) const
{
    for (const auto& group : m_result.contextGeometryGroups)
    {
        if (group.index == groupIndex)
        {
            return &group;
        }
    }

    return nullptr;
}

int HoleDebugPanel::findRunIndexByStepIndex(int stepIndex) const
{
    for (const auto& run : m_result.contextTrace.runs)
    {
        if (std::find(
                run.traceStepIndices.begin(),
                run.traceStepIndices.end(),
                stepIndex) != run.traceStepIndices.end())
        {
            return run.index;
        }
    }

    return -1;
}
