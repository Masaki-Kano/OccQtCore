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

    if (!current)
    {
        ui->plainDetail->clear();
        emit selectionCleared();
        return;
    }

    const auto kind = static_cast<ItemKind>(
        current->data(0, ItemKindRole).toInt());

    const int index = current->data(0, ItemIndexRole).toInt();

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
        showWallBoundariesRootDetail();
        emit selectionCleared();
        break;

    case ItemKind::WallBoundary:
        showWallBoundaryDetail(index);
        emit boundarySelected(index);
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
        auto* wallItem = new QTreeWidgetItem(wallsRoot);
        wallItem->setText(0, QString("Wall[%1]").arg(i));
        wallItem->setData(0, ItemKindRole, static_cast<int>(ItemKind::Wall));
        wallItem->setData(0, ItemIndexRole, i);
    }

    populateWallBoundariesRoot();

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

QTreeWidgetItem* HoleDebugPanel::populateWallBoundariesRoot()
{
    auto* rootItem = new QTreeWidgetItem(ui->treeItems);

    rootItem->setText(
        0,
        QString("Wall Boundaries (%1)")
            .arg(static_cast<int>(m_result.wallBoundaries.size())));

    rootItem->setData(
        0,
        ItemKindRole,
        static_cast<int>(ItemKind::WallBoundariesRoot));

    rootItem->setData(0, ItemIndexRole, -1);

    for (int i = 0; i < static_cast<int>(m_result.wallBoundaries.size()); ++i)
    {
        const auto& boundary = m_result.wallBoundaries[i];

        auto* boundaryItem = new QTreeWidgetItem(rootItem);

        boundaryItem->setText(
            0,
            QString("Boundary[%1] Wall=%2 %3")
                .arg(i)
                .arg(boundary.wallIndex)
                .arg(toString(boundary.kind)));

        boundaryItem->setData(
            0,
            ItemKindRole,
            static_cast<int>(ItemKind::WallBoundary));

        boundaryItem->setData(0, ItemIndexRole, i);
    }

    rootItem->setExpanded(true);

    return rootItem;
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

void HoleDebugPanel::showWallBoundariesRootDetail()
{
    QString text;

    text += "種別: 穴壁境界一覧\n";
    text += QString("Count: %1\n")
                .arg(static_cast<int>(m_result.wallBoundaries.size()));

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





