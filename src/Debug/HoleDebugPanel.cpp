#include "Debug/HoleDebugPanel.h"

#include "ui_HoleDebugPanel.h"

#include <QAbstractItemView>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QString>
#include <QStringList>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <QDebug>

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

    QString toString(OccQtCore::Feature::HoleContextGeometryGroupKind kind)
    {
        using Kind = OccQtCore::Feature::HoleContextGeometryGroupKind;

        switch (kind)
        {
        case Kind::Unknown:
            return "Unknown";
        case Kind::Cylindrical:
            return "Cylindrical";
        case Kind::Planar:
            return "Planar";
        case Kind::Conical:
            return "Conical";
        case Kind::Toroidal:
            return "Toroidal";
        case Kind::Mixed:
            return "Mixed";
        case Kind::Ambiguous:
            return "Ambiguous";
        }

        return "Unknown";
    }

    QString toString(OccQtCore::Feature::HoleContextTracePortKind kind)
    {
        using Kind = OccQtCore::Feature::HoleContextTracePortKind;

        switch (kind)
        {
        case Kind::Unknown:
            return "Unknown";
        case Kind::ExternalTransition:
            return "ExternalTransition";
        case Kind::InternalLoop:
            return "InternalLoop";
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
        QString("Build completed. Groups: %1, Ports: %2")
            .arg(static_cast<int>(m_result.contextGeometryGroups.size()))
            .arg(static_cast<int>(m_result.contextTracePorts.size())));
}

void HoleDebugPanel::clear()
{
    m_result = OccQtCore::Feature::HoleRecognitionResult{};

    ui->treeItems->clear();
    ui->plainDetail->clear();

    populateGroupsRoot();

    setStatusText("Cleared");
}

void HoleDebugPanel::setStatusText(const QString& text)
{
    ui->lblStatus->setText(text);
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
    case ItemKind::GroupsRoot:
        showGroupsRootDetail();
        emit selectionCleared();
        break;

    case ItemKind::Group:
        showGroupDetail(index);
        emit groupSelected(index);
        break;

    case ItemKind::TracePortsRoot:
        showTracePortsRootDetail(index);
        emit selectionCleared();
        break;

    case ItemKind::TracePort:
        showTracePortDetail(index);
        emit tracePortSelected(index);
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

    auto* groupsRoot = populateGroupsRoot();

    for (int i = 0; i < static_cast<int>(m_result.contextGeometryGroups.size()); ++i)
    {
        const auto& group = m_result.contextGeometryGroups[i];

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

        groupItem->setData(0, ItemIndexRole, i);
        groupItem->setData(0, ItemSubIndexRole, -1);

        populateTracePortsRoot(groupItem, group.index);
    }

    groupsRoot->setExpanded(true);
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

QTreeWidgetItem* HoleDebugPanel::populateTracePortsRoot(
    QTreeWidgetItem* parentItem,
    int sourceGroupIndex)
{
    int portCount = 0;

    for (const auto& port : m_result.contextTracePorts)
    {
        if (port.sourceGroupIndex == sourceGroupIndex)
        {
            ++portCount;
        }
    }

    auto* rootItem = new QTreeWidgetItem(parentItem);

    rootItem->setText(
        0,
        QString("Ports (%1)").arg(portCount));

    rootItem->setData(
        0,
        ItemKindRole,
        static_cast<int>(ItemKind::TracePortsRoot));

    rootItem->setData(0, ItemIndexRole, sourceGroupIndex);
    rootItem->setData(0, ItemSubIndexRole, -1);

    for (int i = 0; i < static_cast<int>(m_result.contextTracePorts.size()); ++i)
    {
        const auto& port = m_result.contextTracePorts[i];

        if (port.sourceGroupIndex != sourceGroupIndex)
        {
            continue;
        }

        auto* portItem = new QTreeWidgetItem(rootItem);

        portItem->setText(
            0,
            QString("Port[%1] %2 Edges=%3")
                .arg(port.index)
                .arg(toString(port.kind))
                .arg(static_cast<int>(port.geometryRefs.edgeIndices.size())));

        portItem->setData(
            0,
            ItemKindRole,
            static_cast<int>(ItemKind::TracePort));

        portItem->setData(0, ItemIndexRole, i);
        portItem->setData(0, ItemSubIndexRole, -1);
    }

    rootItem->setExpanded(true);

    return rootItem;
}

void HoleDebugPanel::showGroupsRootDetail()
{
    QString text;

    text += "種別: ContextGroupsRoot\n";
    text += QString("GroupCount: %1\n")
                .arg(static_cast<int>(m_result.contextGeometryGroups.size()));
    text += QString("PortCount: %1\n")
                .arg(static_cast<int>(m_result.contextTracePorts.size()));

    ui->plainDetail->setPlainText(text);
}

void HoleDebugPanel::showGroupDetail(int groupTreeIndex)
{
    if (groupTreeIndex < 0 ||
        groupTreeIndex >= static_cast<int>(m_result.contextGeometryGroups.size()))
    {
        ui->plainDetail->setPlainText("Invalid context group index.");
        return;
    }

    const auto& group = m_result.contextGeometryGroups[groupTreeIndex];

    QString text;

    text += "種別: ContextGeometryGroup\n";
    text += QString("TreeIndex: %1\n").arg(groupTreeIndex);
    text += QString("GroupIndex: %1\n").arg(group.index);
    text += QString("Kind: %1\n").arg(toString(group.kind));

    text += "\nGeometryRefs:\n";
    text += QString("Faces: %1\n").arg(formatIntList(group.geometryRefs.faceIndices));
    text += QString("Wires: %1\n").arg(formatIntList(group.geometryRefs.wireIndices));
    text += QString("Edges: %1\n").arg(formatIntList(group.geometryRefs.edgeIndices));
    text += QString("Vertices: %1\n").arg(formatIntList(group.geometryRefs.vertexIndices));

    text += "\nGeometry:\n";
    text += QString("HasAxis: %1\n").arg(group.hasAxis ? "true" : "false");

    if (group.hasAxis)
    {
        text += QString("AxisPoint: %1\n").arg(formatPoint(group.axisPoint));
        text += QString("AxisDirection: %1\n").arg(formatDirection(group.axisDirection));
        text += QString("Radius: %1\n").arg(group.radius, 0, 'f', 4);
        text += QString("AxialMin: %1\n").arg(group.axialMin, 0, 'f', 4);
        text += QString("AxialMax: %1\n").arg(group.axialMax, 0, 'f', 4);
        text += QString("AxialPosition: %1\n").arg(group.axialPosition, 0, 'f', 4);
    }

    if (!group.note.empty())
    {
        text += "\nNote:\n";
        text += QString::fromStdString(group.note);
        text += "\n";
    }

    ui->plainDetail->setPlainText(text);
}

void HoleDebugPanel::showTracePortsRootDetail(int sourceGroupIndex)
{
    int count = 0;

    for (const auto& port : m_result.contextTracePorts)
    {
        if (port.sourceGroupIndex == sourceGroupIndex)
        {
            ++count;
        }
    }

    QString text;

    text += "種別: TracePortsRoot\n";
    text += QString("SourceGroupIndex: %1\n").arg(sourceGroupIndex);
    text += QString("PortCount: %1\n").arg(count);

    ui->plainDetail->setPlainText(text);
}

void HoleDebugPanel::showTracePortDetail(int portTreeIndex)
{
    if (portTreeIndex < 0 ||
        portTreeIndex >= static_cast<int>(m_result.contextTracePorts.size()))
    {
        ui->plainDetail->setPlainText("Invalid trace port index.");
        return;
    }

    const auto& port = m_result.contextTracePorts[portTreeIndex];

    QString text;

    text += "種別: ContextTracePort\n";
    text += QString("TreeIndex: %1\n").arg(portTreeIndex);
    text += QString("PortIndex: %1\n").arg(port.index);
    text += QString("SourceGroupIndex: %1\n").arg(port.sourceGroupIndex);
    text += QString("Kind: %1\n").arg(toString(port.kind));

    text += "\nGeometryRefs:\n";
    text += QString("Faces: %1\n").arg(formatIntList(port.geometryRefs.faceIndices));
    text += QString("Wires: %1\n").arg(formatIntList(port.geometryRefs.wireIndices));
    text += QString("Edges: %1\n").arg(formatIntList(port.geometryRefs.edgeIndices));
    text += QString("Vertices: %1\n").arg(formatIntList(port.geometryRefs.vertexIndices));

    text += "\nGeometry:\n";
    text += QString("AxialMin: %1\n").arg(port.axialMin, 0, 'f', 4);
    text += QString("AxialMax: %1\n").arg(port.axialMax, 0, 'f', 4);
    text += QString("AxialPosition: %1\n").arg(port.axialPosition, 0, 'f', 4);
    text += QString("CircumferentialCoverage: %1\n")
                .arg(port.circumferentialCoverage, 0, 'f', 4);

    if (!port.note.empty())
    {
        text += "\nNote:\n";
        text += QString::fromStdString(port.note);
        text += "\n";
    }

    ui->plainDetail->setPlainText(text);
}



