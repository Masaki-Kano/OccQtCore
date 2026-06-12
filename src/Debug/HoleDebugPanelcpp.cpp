#include "Debug/HoleDebugPanel.h"
#include "ui_HoleDebugPanel.h"

#include <QPushButton>
#include <QRadioButton>
#include <QCheckBox>
#include <QPlainTextEdit>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QStringList>
#include <QVariant>

#include "Feature/HoleRecognitionTypes.h"

namespace
{
    constexpr int ItemTypeRole = Qt::UserRole + 1;
    constexpr int HoleIndexRole = Qt::UserRole + 2;
    constexpr int ElementIndexRole = Qt::UserRole + 3;
    constexpr int ChildIndexRole = Qt::UserRole + 4;

    using SelectType = OccQtCore::Debug::HoleDebugSelectedItem::Type;

    void setItemData(
        QTreeWidgetItem* item,
        SelectType type,
        int holeIndex,
        int elementIndex = -1,
        int childIndex = -1)
    {
        if (item == nullptr)
        {
            return;
        }

        item->setData(0, ItemTypeRole, static_cast<int>(type));
        item->setData(0, HoleIndexRole, holeIndex);
        item->setData(0, ElementIndexRole, elementIndex);
        item->setData(0, ChildIndexRole, childIndex);
    }

    QString formatIndexList(const std::vector<int>& indices)
    {
        if (indices.empty())
        {
            return "なし";
        }

        QString text;

        for (int i = 0; i < static_cast<int>(indices.size()); ++i)
        {
            if (i > 0)
            {
                text += ", ";
            }

            text += QString::number(indices[i]);
        }

        return text;
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

    QString formatGeometryRefs(
        const OccQtCore::Feature::GeometryRefs& refs)
    {
        QString text;

        text += QString("Faces: %1\n")
                    .arg(formatIndexList(refs.faceIndices));

        text += QString("Wires: %1\n")
                    .arg(formatIndexList(refs.wireIndices));

        text += QString("Edges: %1\n")
                    .arg(formatIndexList(refs.edgeIndices));

        text += QString("Vertices: %1\n")
                    .arg(formatIndexList(refs.vertexIndices));

        return text;
    }

    QString formatReachability(
        const OccQtCore::Feature::HoleReachability& reachability)
    {
        QString text;

        text += QString("  Segment[%1] <-> Segment[%2]\n")
                    .arg(reachability.lhsSegmentCandidateIndex)
                    .arg(reachability.rhsSegmentCandidateIndex);

        text += QString("    End[%1] <-> End[%2]\n")
                    .arg(reachability.lhsEndCandidateIndex)
                    .arg(reachability.rhsEndCandidateIndex);

        text += QString("    Reason: %1\n")
                    .arg(holeReachabilityReasonDisplayName(reachability.reason));

        text += QString("    Shared Faces: %1\n")
                    .arg(formatIndexList(
                        reachability.sharedGeometryRefs.faceIndices));

        text += QString("    Shared Edges: %1\n")
                    .arg(formatIndexList(
                        reachability.sharedGeometryRefs.edgeIndices));

        text += QString("    Shared Vertices: %1\n")
                    .arg(formatIndexList(
                        reachability.sharedGeometryRefs.vertexIndices));

        return text;
    }
}

HoleDebugPanel::HoleDebugPanel(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::HoleDebugPanel)
{
    ui->setupUi(this);

    setupInitialState();
    setupConnections();
}

HoleDebugPanel::~HoleDebugPanel()
{
    delete ui;
}

OccQtCore::Debug::HoleDebugDisplayOptions HoleDebugPanel::displayOptions() const
{
    using namespace OccQtCore::Debug;

    HoleDebugDisplayOptions options;

    if (ui->radioShowAllCandidates->isChecked())
    {
        options.scope = HoleDebugDisplayScope::All;
    }
    else
    {
        options.scope = HoleDebugDisplayScope::Selected;
    }

    return options;
}

void HoleDebugPanel::setDisplayOptions(const OccQtCore::Debug::HoleDebugDisplayOptions& options)
{
    using namespace OccQtCore::Debug;

    switch (options.scope)
    {
    case HoleDebugDisplayScope::All:
        ui->radioShowAllCandidates->setChecked(true);
        break;

    case HoleDebugDisplayScope::Selected:
    default:
        ui->radioShowSelected->setChecked(true);
        break;
    }
}

void HoleDebugPanel::setStatusText(const QString& text)
{
    ui->labelStatus->setText(text);
}

void HoleDebugPanel::setRecognitionResult(const OccQtCore::Feature::HoleRecognitionResult& result)
{
    m_result = result;
    m_hasResult = true;

    populateHoleTree(result);

    ui->editSelectedDetail->setPlainText("穴フィーチャ一覧から項目を選択してください。");
}

void HoleDebugPanel::setupInitialState()
{
    ui->labelStatus->setText("状態: 未生成");

    ui->radioShowSelected->setChecked(true);

    ui->treeCandidates->setHeaderLabel("穴フィーチャ");

    ui->editSelectedDetail->setReadOnly(true);
    ui->editSelectedDetail->setPlainText(
        "穴フィーチャー一覧から項目を選択してください。");
}

void HoleDebugPanel::setupConnections()
{
    connect(
        ui->btnBuildCandidates,
        &QPushButton::clicked,
        this,
        &HoleDebugPanel::buildRequested);

    connect(
        ui->btnRefreshDisplay,
        &QPushButton::clicked,
        this,
        &HoleDebugPanel::refreshDisplayRequested);

    connect(
        ui->btnClearDisplay,
        &QPushButton::clicked,
        this,
        &HoleDebugPanel::clearDisplayRequested);

    connect(
        ui->btnExportDetailLog,
        &QPushButton::clicked,
        this,
        &HoleDebugPanel::exportDetailLogRequested);

    const auto emitChanged =
        [this]()
    {
        emitDisplayOptionsChanged();
    };

    connect(
        ui->radioShowAllCandidates,
        &QRadioButton::toggled,
        this,
        emitChanged);

    connect(
        ui->radioShowSelected,
        &QRadioButton::toggled,
        this,
        emitChanged);

    connect(
        ui->treeCandidates,
        &QTreeWidget::currentItemChanged,
        this,
        [this](QTreeWidgetItem* current, QTreeWidgetItem*)
        {
            updateSelectionDetail(current);

            emit selectedItemChanged(
                selectedItemFromTreeItem(current));
        });
}

void HoleDebugPanel::emitDisplayOptionsChanged()
{
    emit displayOptionsChanged(displayOptions());
}

void HoleDebugPanel::populateHoleTree(const OccQtCore::Feature::HoleRecognitionResult& result)
{
    ui->treeCandidates->clear();

    for (const auto& hole : result.holes)
    {
        auto* holeItem = new QTreeWidgetItem(
            ui->treeCandidates,
            QStringList{
                QString("Hole[%1] %2")
                .arg(hole.index)
                .arg(OccQtCore::Feature::Hole::holeTypeDisplayName(hole.holeType))
            });

        setItemData(holeItem, SelectType::Hole, hole.index);

        for (const auto& element : hole.elements)
        {
            auto* elementItem = new QTreeWidgetItem(
                holeItem,
                QStringList{
                    QString("Element[%1] R=%2 Depth=%3")
                    .arg(element.index)
                    .arg(element.wall.radius, 0, 'f', 3)
                    .arg(element.depth, 0, 'f', 3)
                });

            setItemData(elementItem, SelectType::Element, hole.index, element.index);

            for (int endIndex = 0; endIndex < static_cast<int>(element.ends.size()); ++endIndex)
            {
                const auto& end = element.ends[endIndex];

                auto* endItem = new QTreeWidgetItem(
                    elementItem,
                    QStringList{
                        QString("End[%1] %2")
                        .arg(endIndex)
                        .arg(OccQtCore::Feature::Hole::endTypeDisplayName(end.endType))
                    });

                setItemData(endItem, SelectType::End, hole.index, element.index, endIndex);
            }

            constexpr int wallIndex = 0;

            auto* wallItem = new QTreeWidgetItem(
                elementItem,
                QStringList{
                    QString("Wall[%1] R=%2")
                    .arg(wallIndex)
                    .arg(element.wall.radius, 0, 'f', 3)
                });

            setItemData(wallItem, SelectType::Wall, hole.index, element.index, wallIndex);
        }
    }

    ui->treeCandidates->expandAll();
}

OccQtCore::Debug::HoleDebugSelectedItem HoleDebugPanel::selectedItemFromTreeItem(QTreeWidgetItem* item) const
{
    OccQtCore::Debug::HoleDebugSelectedItem selected;

    if (item == nullptr)
    {
        return selected;
    }

    const QVariant typeValue = item->data(0, ItemTypeRole);

    if (!typeValue.isValid())
    {
        return selected;
    }

    selected.type = static_cast<OccQtCore::Debug::HoleDebugSelectedItem::Type>(typeValue.toInt());

    selected.holeIndex = item->data(0, HoleIndexRole).toInt();

    selected.elementIndex = item->data(0, ElementIndexRole).toInt();

    selected.childIndex = item->data(0, ChildIndexRole).toInt();

    return selected;
}

void HoleDebugPanel::updateSelectionDetail(QTreeWidgetItem* item)
{
    const auto selected = selectedItemFromTreeItem(item);

    if (!m_hasResult)
    {
        ui->editSelectedDetail->setPlainText("穴認識結果がありません。");
        return;
    }

    using Type = OccQtCore::Debug::HoleDebugSelectedItem::Type;

    switch (selected.type)
    {
    case Type::Hole:
        ui->editSelectedDetail->setPlainText(buildHoleDetailText(selected.holeIndex));
        break;

    case Type::Element:
        ui->editSelectedDetail->setPlainText(buildElementDetailText(selected.holeIndex, selected.elementIndex));
        break;

    case Type::End:
        ui->editSelectedDetail->setPlainText(buildEndDetailText(selected.holeIndex, selected.elementIndex, selected.childIndex));
        break;

    case Type::Wall:
        ui->editSelectedDetail->setPlainText(buildWallDetailText(selected.holeIndex, selected.childIndex));
        break;

    case Type::None:
    default:
        ui->editSelectedDetail->setPlainText("穴フィーチャ一覧から項目を選択してください。");
    }
}

QString HoleDebugPanel::buildHoleDetailText(int holeIndex) const
{
    if (holeIndex < 0 ||
        holeIndex >= static_cast<int>(m_result.holes.size()))
    {
        return QString("Hole[%1] は存在しません。").arg(holeIndex);
    }

    const auto& hole = m_result.holes[holeIndex];

    QString text;

    text += "種別: 穴フィーチャ\n";
    text += QString("Index: %1\n").arg(hole.index);
    text += QString("HoleType: %1\n").arg(OccQtCore::Feature::Hole::holeTypeDisplayName(hole.holeType));

    text += "\n";
    text += QString("AxisPoint: %1\n").arg(formatPoint(hole.axisPoint));
    text += QString("AxisDirextion: %1\n").arg(formatDirection(hole.axisDirection));

    text += "\n";
    text += QString("Elements: %1\n").arg(hole.elements.size());

    text += "\n";
    text += "Source Data:\n";
    text += QString("   SourceHoleCandidateIndex: %1\n").arg(hole.sourceHoleCandidateIndex);

    if (hole.sourceHoleCandidateIndex >= 0 &&
        hole.sourceHoleCandidateIndex < static_cast<int>(m_result.holeCandidates.size()))
    {
        const auto& candidate =
            m_result.holeCandidates[hole.sourceHoleCandidateIndex];

        text += QString("  SourceSegments: %1\n")
                    .arg(formatIndexList(candidate.segmentCandidateIndices));

        text += "\n";
        text += "  Reachabilities:\n";

        if (candidate.reachabilities.empty())
        {
            text += "    なし\n";
        }
        else
        {
            for (const auto& reachability : candidate.reachabilities)
            {
                text += formatReachability(reachability);
            }
        }
    }

    return text;
}

QString HoleDebugPanel::buildElementDetailText(int holeIndex, int elementIndex) const
{
    if (holeIndex < 0 ||
        holeIndex >= static_cast<int>(m_result.holes.size()))
    {
        return QString("Hole[%1] は存在しません。").arg(holeIndex);
    }

    const auto& hole = m_result.holes[holeIndex];

    if (elementIndex < 0 ||
        elementIndex >= static_cast<int>(hole.elements.size()))
    {
        return QString("Element[%1] は存在しません。").arg(elementIndex);
    }

    const auto& element =
        hole.elements[elementIndex];

    QString text;

    text += "種別: 穴要素\n";
    text += QString("HoleIndex: %1\n").arg(hole.index);
    text += QString("Index: %1\n").arg(element.index);
    text += QString("Depth: %1\n").arg(element.depth, 0, 'f', 4);
    text += QString("EndCount: %1\n").arg(element.ends.size());

    text += "\n";
    text += "Wall:\n";
    text += QString("  Radius: %1\n").arg(element.wall.radius, 0, 'f', 4);
    text += QString("  Center: %1\n").arg(formatPoint(element.wall.center));
    text += QString("  AxisDirection: %1\n")
                .arg(formatDirection(element.wall.axisDirection));

    text += "\n";
    text += "Source Data:\n";
    text += QString("  SourceSegmentCandidateIndex: %1\n")
                .arg(element.sourceSegmentCandidateIndex);

    if (element.sourceSegmentCandidateIndex >= 0 &&
        element.sourceSegmentCandidateIndex < static_cast<int>(m_result.segmentCandidates.size()))
    {
        const auto& segment =
            m_result.segmentCandidates[element.sourceSegmentCandidateIndex];

        text += QString("  SourceWallCandidateIndex: %1\n")
                    .arg(segment.wallCandidateIndex);

        text += QString("  SourceEndCandidateIndices: %1\n")
                    .arg(formatIndexList(segment.endCandidateIndices));
    }

    return text;
}

QString HoleDebugPanel::buildWallDetailText(
    int holeIndex,
    int elementIndex) const
{
    if (holeIndex < 0 ||
        holeIndex >= static_cast<int>(m_result.holes.size()))
    {
        return QString("Hole[%1] は存在しません。").arg(holeIndex);
    }

    const auto& hole =
        m_result.holes[holeIndex];

    if (elementIndex < 0 ||
        elementIndex >= static_cast<int>(hole.elements.size()))
    {
        return QString("Element[%1] は存在しません。").arg(elementIndex);
    }

    const auto& element =
        hole.elements[elementIndex];

    const auto& wall =
        element.wall;

    QString text;

    text += "種別: 穴壁\n";
    text += QString("HoleIndex: %1\n").arg(hole.index);
    text += QString("ElementIndex: %1\n").arg(elementIndex);
    text += "Index: 0\n";

    text += QString("Radius: %1\n").arg(wall.radius, 0, 'f', 4);

    text += "\n";
    text += QString("Center: %1\n")
                .arg(formatPoint(wall.center));

    text += QString("AxisDirection: %1\n")
                .arg(formatDirection(wall.axisDirection));

    text += "\n";
    text += "GeometryRefs:\n";
    text += formatGeometryRefs(wall.geometryRefs);

    text += "\n";
    text += "Source Data:\n";
    text += QString("  SourceSegmentCandidateIndex: %1\n")
                .arg(element.sourceSegmentCandidateIndex);

    if (element.sourceSegmentCandidateIndex >= 0 &&
        element.sourceSegmentCandidateIndex < static_cast<int>(m_result.segmentCandidates.size()))
    {
        const auto& segment =
            m_result.segmentCandidates[element.sourceSegmentCandidateIndex];

        text += QString("  SourceWallCandidateIndex: %1\n")
                    .arg(segment.wallCandidateIndex);
    }

    return text;
}

QString HoleDebugPanel::buildEndDetailText(
    int holeIndex,
    int elementIndex,
    int endIndex) const
{
    if (holeIndex < 0 ||
        holeIndex >= static_cast<int>(m_result.holes.size()))
    {
        return QString("Hole[%1] は存在しません。").arg(holeIndex);
    }

    const auto& hole =
        m_result.holes[holeIndex];

    if (elementIndex < 0 ||
        elementIndex >= static_cast<int>(hole.elements.size()))
    {
        return QString("Element[%1] は存在しません。").arg(elementIndex);
    }

    const auto& element =
        hole.elements[elementIndex];

    if (endIndex < 0 ||
        endIndex >= static_cast<int>(element.ends.size()))
    {
        return QString("End[%1] は存在しません。").arg(endIndex);
    }

    const auto& end =
        element.ends[endIndex];

    QString text;

    text += "種別: 穴端\n";
    text += QString("HoleIndex: %1\n").arg(hole.index);
    text += QString("ElementIndex: %1\n").arg(element.index);
    text += QString("Index: %1\n").arg(endIndex);
    text += QString("EndType: %1\n")
                .arg(OccQtCore::Feature::Hole::endTypeDisplayName(
                    end.endType));

    text += "\n";
    text += "GeometryRefs:\n";
    text += formatGeometryRefs(end.geometryRefs);

    text += "\n";
    text += "Source Data:\n";
    text += QString("  SourceEndCandidateIndex: %1\n")
                .arg(end.sourceEndCandidateIndex);

    if (end.sourceEndCandidateIndex >= 0 &&
        end.sourceEndCandidateIndex < static_cast<int>(m_result.endCandidates.size()))
    {
        const auto& sourceEnd =
            m_result.endCandidates[end.sourceEndCandidateIndex];

        text += QString("  SourceType: %1\n")
                    .arg(OccQtCore::Feature::holeEndCandidateTypeDisplayName(
                        sourceEnd.type));

        text += QString("  SourceWallCandidateIndex: %1\n")
                    .arg(sourceEnd.wallCandidateIndex);

        text += QString("  WallBoundaryLoopIndex: %1\n")
                    .arg(sourceEnd.wallBoundaryLoopIndex);
    }

    return text;
}




