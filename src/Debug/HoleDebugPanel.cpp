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

    QString toString(OccQtCore::Feature::HoleWallBoundaryKind kind)
    {
        using Kind = OccQtCore::Feature::HoleWallBoundaryKind;

        switch (kind)
        {
        case Kind::Unknown:
            return "Unknown";
        case Kind::AxialEnd:
            return "AxialEnd";
        case Kind::LateralConnection:
            return "LateralConnection";
        case Kind::InternalWallSplit:
            return "InternalWallSplit";
        case Kind::Broken:
            return "Broken";
        case Kind::Ambiguous:
            return "Ambiguous";
        }

        return "Unknown";
    }

    QString toString(OccQtCore::Feature::HoleWallBoundaryTraceStatus status)
    {
        using Status = OccQtCore::Feature::HoleWallBoundaryTraceStatus;

        switch (status)
        {
        case Status::Unknown:
            return "Unknown";
        case Status::Traceable:
            return "Traceable";
        case Status::Ignored:
            return "Ignored";
        case Status::NotTraceable:
            return "NotTraceable";
        }

        return "Unknown";
    }

    QString toString(OccQtCore::Feature::GeometryTraceEndReason reason)
    {
        using Reason = OccQtCore::Feature::GeometryTraceEndReason;

        switch (reason)
        {
        case Reason::Unknown:
            return "Unknown";
        case Reason::ReachedHoleWall:
            return "ReachedHoleWall";
        case Reason::NoHoleWallCandidate:
            return "NoHoleWallCandidate";
        case Reason::OutOfHoleContext:
            return "OutOfHoleContext";
        case Reason::Ambiguous:
            return "Ambiguous";
        case Reason::LoopDetected:
            return "LoopDetected";
        case Reason::MaxDepthReached:
            return "MaxDepthReached";
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

    setStatusText(QString("Build completed. Walls: %1").arg(static_cast<int>(m_result.walls.size())));
}

void HoleDebugPanel::clear()
{
    m_result = OccQtCore::Feature::HoleRecognitionResult{};

    ui->treeItems->clear();
    ui->plainDetail->clear();

    populateWallsRoot();

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
    case ItemKind::WallsRoot:
        showWallsRootDetail();
        emit selectionCleared();
        break;

    case ItemKind::Wall:
        showWallDetail(index);
        emit wallSelected(index);
        break;

    case ItemKind::WallBoundariesRoot:
        showWallBoundariesRootDetail(index);
        emit selectionCleared();
        break;

    case ItemKind::WallBoundary:
        showWallBoundaryDetail(index);
        emit boundarySelected(index);
        break;

    case ItemKind::GeometryTrace:
        showGeometryTraceDetail(index);
        emit selectionCleared();
        break;

    case ItemKind::GeometryTraceNode:
        showGeometryTraceNodeDetail(index, subIndex);
        emit geometryTraceNodeSelected(index, subIndex);
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

    auto* wallsRoot = populateWallsRoot();

    for (int i = 0; i < static_cast<int>(m_result.walls.size()); ++i)
    {
        const auto& wall = m_result.walls[i];

        auto* wallItem = new QTreeWidgetItem(wallsRoot);
        wallItem->setText(
            0,
            QString("Wall[%1]").arg(wall.index));

        wallItem->setData(
            0,
            ItemKindRole,
            static_cast<int>(ItemKind::Wall));

        // ここは TreeIndex として i を入れる
        wallItem->setData(0, ItemIndexRole, i);

        populateWallBoundariesRoot(wallItem, wall.index);
    }

    wallsRoot->setExpanded(true);
}

QTreeWidgetItem* HoleDebugPanel::populateWallsRoot()
{
    auto* wallsRoot = new QTreeWidgetItem(ui->treeItems);
    wallsRoot->setText(0, "Walls");
    wallsRoot->setData(0, ItemKindRole, static_cast<int>(ItemKind::WallsRoot));
    wallsRoot->setData(0, ItemIndexRole, -1);
    wallsRoot->setExpanded(true);

    return wallsRoot;
}

QTreeWidgetItem* HoleDebugPanel::populateWallBoundariesRoot(
    QTreeWidgetItem* parentItem,
    int wallIndex)
{
    int boundaryCount = 0;

    for (const auto& boundary : m_result.wallBoundaries)
    {
        if (boundary.wallIndex == wallIndex)
        {
            ++boundaryCount;
        }
    }

    auto* rootItem = new QTreeWidgetItem(parentItem);

    rootItem->setText(
        0,
        QString("Boundaries (%1)").arg(boundaryCount));

    rootItem->setData(
        0,
        ItemKindRole,
        static_cast<int>(ItemKind::WallBoundariesRoot));

    // ここは「このWall配下のBoundary一覧」という意味で wallIndex を入れてもいい
    rootItem->setData(0, ItemIndexRole, wallIndex);
    rootItem->setData(0, ItemSubIndexRole, -1);

    for (int i = 0; i < static_cast<int>(m_result.wallBoundaries.size()); ++i)
    {
        const auto& boundary = m_result.wallBoundaries[i];

        if (boundary.wallIndex != wallIndex)
        {
            continue;
        }

        auto* boundaryItem = new QTreeWidgetItem(rootItem);

        boundaryItem->setText(
            0,
            QString("Boundary[%1] %2 / %3")
                .arg(boundary.index)
                .arg(toString(boundary.kind))
                .arg(toString(boundary.traceStatus)));

        boundaryItem->setData(
            0,
            ItemKindRole,
            static_cast<int>(ItemKind::WallBoundary));

        boundaryItem->setData(0, ItemIndexRole, i);
        boundaryItem->setData(0, ItemSubIndexRole, -1);

        populateGeometryTracesOfBoundary(boundaryItem, i);
    }

    rootItem->setExpanded(true);

    return rootItem;
}

void HoleDebugPanel::populateGeometryTracesOfBoundary(
    QTreeWidgetItem* boundaryItem,
    int boundaryIndex)
{
    if (boundaryItem == nullptr)
    {
        return;
    }

    if (boundaryIndex < 0 ||
        boundaryIndex >= static_cast<int>(m_result.wallBoundaries.size()))
    {
        return;
    }

    const auto& boundary = m_result.wallBoundaries[boundaryIndex];

    for (int traceIndex = 0;
         traceIndex < static_cast<int>(m_result.geometryTraces.size());
         ++traceIndex)
    {
        const auto& trace = m_result.geometryTraces[traceIndex];

        if (trace.sourceBoundaryIndex != boundary.index)
        {
            continue;
        }

        auto* traceItem = new QTreeWidgetItem(boundaryItem);

        traceItem->setText(
            0,
            QString("Trace[%1] %2")
                .arg(trace.index)
                .arg(toString(trace.endReason)));

        traceItem->setData(
            0,
            ItemKindRole,
            static_cast<int>(ItemKind::GeometryTrace));

        traceItem->setData(0, ItemIndexRole, traceIndex);
        traceItem->setData(0, ItemSubIndexRole, -1);

        populateGeometryTraceNodes(traceItem, traceIndex);
    }
}

void HoleDebugPanel::populateGeometryTraceNodes(
    QTreeWidgetItem* traceItem,
    int traceIndex)
{
    if (traceItem == nullptr)
    {
        return;
    }

    if (traceIndex < 0 ||
        traceIndex >= static_cast<int>(m_result.geometryTraces.size()))
    {
        return;
    }

    const auto& trace = m_result.geometryTraces[traceIndex];

    for (int nodeIndex = 0;
         nodeIndex < static_cast<int>(trace.nodes.size());
         ++nodeIndex)
    {
        const auto& node = trace.nodes[nodeIndex];

        auto* nodeItem = new QTreeWidgetItem(traceItem);

        nodeItem->setText(
            0,
            QString("Node[%1] depth=%2 Faces=%3")
                .arg(node.index)
                .arg(node.depth)
                .arg(formatIntList(node.geometryRefs.faceIndices)));

        nodeItem->setData(
            0,
            ItemKindRole,
            static_cast<int>(ItemKind::GeometryTraceNode));

        nodeItem->setData(0, ItemIndexRole, traceIndex);
        nodeItem->setData(0, ItemSubIndexRole, nodeIndex);
    }
}

void HoleDebugPanel::showWallsRootDetail()
{
    ui->plainDetail->setPlainText(QString("Walls: %1").arg(static_cast<int>(m_result.walls.size())));
}

void HoleDebugPanel::showWallDetail(int wallIndex)
{
    if (wallIndex < 0 || wallIndex >= static_cast<int>(m_result.walls.size()))
    {
        ui->plainDetail->setPlainText("Invalid wall index.");
        return;
    }

    const auto& wall = m_result.walls[wallIndex];

    QString text;

    text += "種別: 穴壁\n";
    text += QString("TreeIndex: %1\n").arg(wallIndex);
    text += QString("WallIndex: %1\n").arg(wall.index);

    text += "\nGeometryRefs:\n";
    text += QString("Faces: %1\n").arg(formatIntList(wall.geometryRefs.faceIndices));
    text += QString("Wires: %1\n").arg(formatIntList(wall.geometryRefs.wireIndices));
    text += QString("Edges: %1\n").arg(formatIntList(wall.geometryRefs.edgeIndices));
    text += QString("Vertices: %1\n").arg(formatIntList(wall.geometryRefs.vertexIndices));

    text += "\nGeometry:\n";
    text += QString("AxisPoint: %1\n").arg(formatPoint(wall.axisPoint));
    text += QString("AxisDirection: %1\n").arg(formatDirection(wall.axisDirection));
    text += QString("Radius: %1\n").arg(wall.radius, 0, 'f', 4);
    text += QString("AxialMin: %1\n").arg(wall.axialMin, 0, 'f', 4);
    text += QString("AxialMax: %1\n").arg(wall.axialMax, 0, 'f', 4);

    ui->plainDetail->setPlainText(text);
}

void HoleDebugPanel::showWallBoundariesRootDetail(int wallIndex)
{
    int count = 0;

    for (const auto& boundary : m_result.wallBoundaries)
    {
        if (boundary.wallIndex == wallIndex)
        {
            ++count;
        }
    }

    QString text;

    text += "種別: WallBoundariesRoot\n";
    text += QString("WallIndex: %1\n").arg(wallIndex);
    text += QString("BoundaryCount: %1\n").arg(count);

    ui->plainDetail->setPlainText(text);
}

void HoleDebugPanel::showWallBoundaryDetail(int boundaryIndex)
{
    if (boundaryIndex < 0 ||
        boundaryIndex >= static_cast<int>(m_result.wallBoundaries.size()))
    {
        ui->plainDetail->setPlainText("Invalid wall boundary index.");
        return;
    }

    const auto& boundary = m_result.wallBoundaries[boundaryIndex];

    QString text;

    text += "種別: 穴壁境界\n";
    text += QString("TreeIndex: %1\n").arg(boundaryIndex);
    text += QString("BoundaryIndex: %1\n").arg(boundary.index);
    text += QString("WallIndex: %1\n").arg(boundary.wallIndex);
    text += QString("Kind: %1\n").arg(toString(boundary.kind));
    text += QString("TraceStatus: %1\n").arg(toString(boundary.traceStatus));

    text += "\nGeometryRefs:\n";
    text += QString("Faces: %1\n")
                .arg(formatIntList(boundary.geometryRefs.faceIndices));
    text += QString("Wires: %1\n")
                .arg(formatIntList(boundary.geometryRefs.wireIndices));
    text += QString("Edges: %1\n")
                .arg(formatIntList(boundary.geometryRefs.edgeIndices));
    text += QString("Vertices: %1\n")
                .arg(formatIntList(boundary.geometryRefs.vertexIndices));

    text += "\nAdjacentGeometryRefs:\n";
    text += QString("Faces: %1\n")
                .arg(formatIntList(boundary.adjacentGeometryRefs.faceIndices));
    text += QString("Wires: %1\n")
                .arg(formatIntList(boundary.adjacentGeometryRefs.wireIndices));
    text += QString("Edges: %1\n")
                .arg(formatIntList(boundary.adjacentGeometryRefs.edgeIndices));
    text += QString("Vertices: %1\n")
                .arg(formatIntList(boundary.adjacentGeometryRefs.vertexIndices));

    text += "\nGeometry:\n";
    text += QString("AxialMin: %1\n")
                .arg(boundary.axialMin, 0, 'f', 4);
    text += QString("AxialMax: %1\n")
                .arg(boundary.axialMax, 0, 'f', 4);
    text += QString("AxialPosition: %1\n")
                .arg(boundary.axialPosition, 0, 'f', 4);
    text += QString("CircumferentialCoverage: %1\n")
                .arg(boundary.circumferentialCoverage, 0, 'f', 4);

    if (!boundary.note.empty())
    {
        text += "\nNote:\n";
        text += QString::fromStdString(boundary.note);
        text += "\n";
    }

    ui->plainDetail->setPlainText(text);
}

void HoleDebugPanel::showGeometryTraceDetail(int traceIndex)
{
    if (traceIndex < 0 ||
        traceIndex >= static_cast<int>(m_result.geometryTraces.size()))
    {
        ui->plainDetail->setPlainText("Invalid geometry trace index.");
        return;
    }

    const auto& trace = m_result.geometryTraces[traceIndex];

    QString text;

    text += "種別: GeometryTrace\n";
    text += QString("TreeIndex: %1\n").arg(traceIndex);
    text += QString("TraceIndex: %1\n").arg(trace.index);
    text += QString("SourceBoundaryIndex: %1\n")
                .arg(trace.sourceBoundaryIndex);
    text += QString("EndReason: %1\n")
                .arg(toString(trace.endReason));
    text += QString("NodeCount: %1\n")
                .arg(static_cast<int>(trace.nodes.size()));

    if (!trace.note.empty())
    {
        text += "\nNote:\n";
        text += QString::fromStdString(trace.note);
        text += "\n";
    }

    ui->plainDetail->setPlainText(text);
}

void HoleDebugPanel::showGeometryTraceNodeDetail(
    int traceIndex,
    int nodeIndex)
{
    if (traceIndex < 0 ||
        traceIndex >= static_cast<int>(m_result.geometryTraces.size()))
    {
        ui->plainDetail->setPlainText("Invalid geometry trace index.");
        return;
    }

    const auto& trace = m_result.geometryTraces[traceIndex];

    if (nodeIndex < 0 ||
        nodeIndex >= static_cast<int>(trace.nodes.size()))
    {
        ui->plainDetail->setPlainText("Invalid geometry trace node index.");
        return;
    }

    const auto& node = trace.nodes[nodeIndex];

    QString text;

    text += "種別: GeometryTraceNode\n";
    text += QString("TraceTreeIndex: %1\n").arg(traceIndex);
    text += QString("TraceIndex: %1\n").arg(trace.index);
    text += QString("NodeTreeIndex: %1\n").arg(nodeIndex);
    text += QString("NodeIndex: %1\n").arg(node.index);
    text += QString("Depth: %1\n").arg(node.depth);
    text += QString("ParentNodeIndex: %1\n")
                .arg(node.parentNodeIndex);

    text += "\nGeometryRefs:\n";
    text += QString("Faces: %1\n")
                .arg(formatIntList(node.geometryRefs.faceIndices));
    text += QString("Wires: %1\n")
                .arg(formatIntList(node.geometryRefs.wireIndices));
    text += QString("Edges: %1\n")
                .arg(formatIntList(node.geometryRefs.edgeIndices));
    text += QString("Vertices: %1\n")
                .arg(formatIntList(node.geometryRefs.vertexIndices));

    text += "\nGeometry:\n";
    text += QString("AxialMin: %1\n")
                .arg(node.axialMin, 0, 'f', 4);
    text += QString("AxialMax: %1\n")
                .arg(node.axialMax, 0, 'f', 4);

    if (!node.note.empty())
    {
        text += "\nNote:\n";
        text += QString::fromStdString(node.note);
        text += "\n";
    }

    ui->plainDetail->setPlainText(text);
}



