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
    constexpr int CandidateTypeRole = Qt::UserRole + 1;
    constexpr int CandidateIndexRole = Qt::UserRole + 2;

    using RecognitionResult = OccQtCore::Feature::HoleRecognitionResult;
    using SelectType = OccQtCore::Debug::HoleDebugSelectedCandidate::Type;

    void setCandidateData(
        QTreeWidgetItem* item,
        SelectType type,
        int index)
    {
        if (item == nullptr)
        {
            return;
        }

        item->setData(0, CandidateTypeRole, static_cast<int>(type));
        item->setData(0, CandidateIndexRole, index);
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
        options.scope = HoleDebugDisplayScope::AllCandidates;
    }
    else
    {
        options.scope = HoleDebugDisplayScope::SelectedCandidate;
    }

    return options;
}

void HoleDebugPanel::setDisplayOptions(const OccQtCore::Debug::HoleDebugDisplayOptions& options)
{
    using namespace OccQtCore::Debug;

    switch (options.scope)
    {
    case HoleDebugDisplayScope::AllCandidates:
        ui->radioShowAllCandidates->setChecked(true);
        break;

    case HoleDebugDisplayScope::SelectedCandidate:
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
    m_hasRexult = true;

    populateCandidateTree(result);

    ui->editSelectedDetail->setPlainText("候補一覧から項目を選択してください。");
}

void HoleDebugPanel::setupInitialState()
{
    ui->labelStatus->setText("状態: 未生成");

    ui->radioShowSelected->setChecked(true);

    ui->treeCandidates->setHeaderLabel("候補");

    ui->editSelectedDetail->setReadOnly(true);
    ui->editSelectedDetail->setPlainText(
        "候補一覧から項目を選択してください。");
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

            emit selectedCandidateChanged(
                selectedCandidateFromItem(current));
        });
}

void HoleDebugPanel::emitDisplayOptionsChanged()
{
    emit displayOptionsChanged(displayOptions());
}

void HoleDebugPanel::populateCandidateTree(const OccQtCore::Feature::HoleRecognitionResult& result)
{
    ui->treeCandidates->clear();

    for (const auto& hole : result.holeCandidates)
    {
        auto* holeItem = new QTreeWidgetItem(
            ui->treeCandidates,
            QStringList{
                QString("HoleCandidate[%1]  Segments=%2")
                .arg(hole.index)
                    .arg(hole.segmentCandidateIndices.size())
            });

        setCandidateData(holeItem, SelectType::Hole, hole.index);

        for (int segmentIndex : hole.segmentCandidateIndices)
        {
            if (segmentIndex < 0 ||
                segmentIndex >= static_cast<int>(result.segmentCandidates.size()))
            {
                continue;
            }

            const auto& segment = result.segmentCandidates[segmentIndex];

            auto* segmentItem = new QTreeWidgetItem(
                holeItem,
                QStringList{
                    QString("Segment[%1]")
                    .arg(segment.index)
                });

            setCandidateData(
                segmentItem,
                SelectType::Segment,
                segment.index);

            if (segment.wallCandidateIndex >= 0 &&
                segment.wallCandidateIndex < static_cast<int>(result.wallCandidates.size()))
            {
                const auto& wall = result.wallCandidates[segment.wallCandidateIndex];

                auto* wallItem = new QTreeWidgetItem(
                    segmentItem,
                    QStringList{
                        QString("Wall[%1]")
                        .arg(wall.index)
                    });

                setCandidateData(wallItem, SelectType::Wall, wall.index);
            }

            for (int endIndex : segment.endCandidateIndices)
            {
                if (endIndex < 0 ||
                    endIndex >= static_cast<int>(result.endCandidates.size()))
                {
                    continue;
                }

                const auto& end = result.endCandidates[endIndex];

                auto* endItem = new QTreeWidgetItem(
                    segmentItem,
                    QStringList{
                        QString("End[%1] %2")
                        .arg(end.index)
                        .arg(OccQtCore::Feature::holeEndCandidateTypeDisplayName(end.type))
                    });

                setCandidateData(endItem, SelectType::End, end.index);
            }
        }

        ui->treeCandidates->expandAll();
    }
}

OccQtCore::Debug::HoleDebugSelectedCandidate HoleDebugPanel::selectedCandidateFromItem(QTreeWidgetItem* item) const
{
    OccQtCore::Debug::HoleDebugSelectedCandidate selected;

    if (item == nullptr)
    {
        return selected;
    }

    const QVariant typeValue = item->data(0, CandidateTypeRole);
    const QVariant indexValue = item->data(0, CandidateIndexRole);

    if (!typeValue.isValid() ||
        !indexValue.isValid())
    {
        return selected;
    }

    selected.type = static_cast<OccQtCore::Debug::HoleDebugSelectedCandidate::Type>(typeValue.toInt());

    selected.index = indexValue.toInt();

    return selected;
}

void HoleDebugPanel::updateSelectionDetail(QTreeWidgetItem* item)
{
    const auto selected = selectedCandidateFromItem(item);

    if (!m_hasRexult)
    {
        ui->editSelectedDetail->setPlainText("穴認識結果がありません。");
        return;
    }

    QString typeText;

    using Type = OccQtCore::Debug::HoleDebugSelectedCandidate::Type;

    switch (selected.type)
    {
    case Type::Hole:
        ui->editSelectedDetail->setPlainText(buildHoleDetailText(selected.index));
        break;

    case Type::Segment:
        ui->editSelectedDetail->setPlainText(buildSegmentDetailText(selected.index));
        break;

    case Type::Wall:
        ui->editSelectedDetail->setPlainText(buildWallDetailText(selected.index));
        break;

    case Type::End:
        ui->editSelectedDetail->setPlainText(buildEndDetailText(selected.index));
        break;

    case Type::None:
    default:
        ui->editSelectedDetail->setPlainText("候補一覧から候補を選択してください。");
        return;
    }
}

QString HoleDebugPanel::buildHoleDetailText(int index) const
{
    if (index < 0 ||
        index >= static_cast<int>(m_result.holeCandidates.size()))
    {
        return QString("HoleCandidate[%1] は存在しません。").arg(index);
    }

    const auto& hole =
        m_result.holeCandidates[index];

    QString text;

    text += "種別: 穴候補\n";
    text += QString("Index: %1\n").arg(hole.index);
    text += QString("Type: %1\n")
                .arg(static_cast<int>(hole.type));

    text += "\n";
    text += QString("Segments: %1\n")
                .arg(formatIndexList(hole.segmentCandidateIndices));

    return text;
}

QString HoleDebugPanel::buildSegmentDetailText(int index) const
{
    if (index < 0 ||
        index >= static_cast<int>(m_result.segmentCandidates.size()))
    {
        return QString("Segment[%1] は存在しません。").arg(index);
    }

    const auto& segment =
        m_result.segmentCandidates[index];

    QString text;

    text += "種別: 穴区間\n";
    text += QString("Index: %1\n").arg(segment.index);
    text += QString("Type: %1\n")
                .arg(static_cast<int>(segment.type));

    text += "\n";
    text += QString("Wall: %1\n")
                .arg(segment.wallCandidateIndex);

    text += QString("Ends: %1\n")
                .arg(formatIndexList(segment.endCandidateIndices));

    return text;
}

QString HoleDebugPanel::buildWallDetailText(int index) const
{
    if (index < 0 ||
        index >= static_cast<int>(m_result.wallCandidates.size()))
    {
        return QString("Wall[%1] は存在しません。").arg(index);
    }

    const auto& wall =
        m_result.wallCandidates[index];

    QString text;

    text += "種別: 穴壁面\n";
    text += QString("Index: %1\n").arg(wall.index);
    text += QString("Radius: %1\n").arg(wall.radius, 0, 'f', 4);
    text += QString("Depth: %1\n").arg(wall.depth, 0, 'f', 4);

    text += "\n";
    text += QString("Center: %1\n")
                .arg(formatPoint(wall.center));
    text += QString("AxisDirection: %1\n")
                .arg(formatDirection(wall.axisDirection));

    text += "\n";
    text += formatGeometryRefs(wall.geometryRefs);

    return text;
}

QString HoleDebugPanel::buildEndDetailText(int index) const
{
    if (index < 0 ||
        index >= static_cast<int>(m_result.endCandidates.size()))
    {
        return QString("End[%1] は存在しません。").arg(index);
    }

    const auto& end =
        m_result.endCandidates[index];

    QString text;

    text += "種別: 穴端部\n";
    text += QString("Index: %1\n").arg(end.index);
    text += QString("Type: %1\n")
                .arg(OccQtCore::Feature::holeEndCandidateTypeDisplayName(end.type));
    text += QString("Wall: %1\n")
                .arg(end.wallCandidateIndex);
    text += QString("Radius: %1\n")
                .arg(end.radius, 0, 'f', 4);

    text += "\n";
    text += QString("Center: %1\n")
                .arg(formatPoint(end.center));
    text += QString("AxisDirection: %1\n")
                .arg(formatDirection(end.axisDirection));
    text += QString("NormalDirection: %1\n")
                .arg(formatDirection(end.normalDirection));

    text += "\n";
    text += "AxialPosition: ";
    if (end.hasAxialPosition)
    {
        text += QString::number(end.axialPosition, 'f', 4);
    }
    else
    {
        text += "N/A";
    }
    text += "\n";

    text += "\n";
    text += formatGeometryRefs(end.geometryRefs);

    return text;
}




