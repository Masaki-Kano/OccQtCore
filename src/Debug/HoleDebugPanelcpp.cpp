#include "Debug/HoleDebugPanel.h"
#include "ui_HoleDebugPanel.h"

HoleDebugPanel::HoleDebugPanel(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::HoleDebugPanel)
{
    ui->setupUi(this);

    ui->segmentIndexSpinBox->setMinimum(-1);
    ui->segmentIndexSpinBox->setMaximum(9999);

    ui->wallIndexSpinBox->setMinimum(-1);
    ui->wallIndexSpinBox->setMaximum(9999);

    setupConnections();
}

HoleDebugPanel::~HoleDebugPanel()
{
    delete ui;
}

OccQtCore::Debug::HoleDebugDisplayOptions
HoleDebugPanel::displayOptions() const
{
    OccQtCore::Debug::HoleDebugDisplayOptions options;

    options.targetSegmentIndex =
        ui->segmentIndexSpinBox->value();

    options.targetWallCandidateIndex =
        ui->wallIndexSpinBox->value();

    options.showWallFaces =
        ui->showWallFacesCheckBox->isChecked();

    options.showRepresentativeEnds =
        ui->showRepresentativeEndsCheckBox->isChecked();

    options.showRawEnds =
        ui->showRawEndsCheckBox->isChecked();

    options.showOpenEnds =
        ui->showOpenEndsCheckBox->isChecked();

    options.showBottomEnds =
        ui->showBottomEndsCheckBox->isChecked();

    options.showWallConnectionEnds =
        ui->showConnectionEndsCheckBox->isChecked();

    return options;
}

void HoleDebugPanel::setDisplayOptions(
    const OccQtCore::Debug::HoleDebugDisplayOptions& options)
{
    ui->segmentIndexSpinBox->setValue(options.targetSegmentIndex);
    ui->wallIndexSpinBox->setValue(options.targetWallCandidateIndex);

    ui->showWallFacesCheckBox->setChecked(options.showWallFaces);

    ui->showRepresentativeEndsCheckBox->setChecked(
        options.showRepresentativeEnds);

    ui->showRawEndsCheckBox->setChecked(
        options.showRawEnds);

    ui->showOpenEndsCheckBox->setChecked(options.showOpenEnds);
    ui->showBottomEndsCheckBox->setChecked(options.showBottomEnds);

    ui->showConnectionEndsCheckBox->setChecked(
        options.showWallConnectionEnds);
}

bool HoleDebugPanel::isDebugEnabled() const
{
    return ui->enabledCheckBox->isChecked();
}

void HoleDebugPanel::setDebugEnabled(bool enabled)
{
    ui->enabledCheckBox->setChecked(enabled);
}

void HoleDebugPanel::setupConnections()
{
    connect(
        ui->enabledCheckBox,
        &QCheckBox::toggled,
        this,
        &HoleDebugPanel::debugEnabledChanged);

    connect(
        ui->refreshButton,
        &QPushButton::clicked,
        this,
        &HoleDebugPanel::refreshRequested);

    connect(
        ui->rebuildButton,
        &QPushButton::clicked,
        this,
        &HoleDebugPanel::rebuildRequested);

    const auto emitChanged =
        [this]()
    {
        emitDisplayOptionsChanged();
    };

    connect(
        ui->segmentIndexSpinBox,
        QOverload<int>::of(&QSpinBox::valueChanged),
        this,
        emitChanged);

    connect(
        ui->wallIndexSpinBox,
        QOverload<int>::of(&QSpinBox::valueChanged),
        this,
        emitChanged);

    connect(
        ui->showWallFacesCheckBox,
        &QCheckBox::toggled,
        this,
        emitChanged);

    connect(
        ui->showRepresentativeEndsCheckBox,
        &QCheckBox::toggled,
        this,
        emitChanged);

    connect(
        ui->showRawEndsCheckBox,
        &QCheckBox::toggled,
        this,
        emitChanged);

    connect(
        ui->showOpenEndsCheckBox,
        &QCheckBox::toggled,
        this,
        emitChanged);

    connect(
        ui->showBottomEndsCheckBox,
        &QCheckBox::toggled,
        this,
        emitChanged);

    connect(
        ui->showConnectionEndsCheckBox,
        &QCheckBox::toggled,
        this,
        emitChanged);
}

void HoleDebugPanel::emitDisplayOptionsChanged()
{
    emit displayOptionsChanged(displayOptions());
}
