#include <QLabel>
#include <QVBoxLayout>
#include <QSizePolicy>
#include <QTimer>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>

#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <AIS_DisplayMode.hxx>
#include <Quantity_Color.hxx>
#include <Quantity_NameOfColor.hxx>

#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "View/OccView.h"
#include "Log/AppLogger.h"
#include "Log/AppLogReporter.h"
#include "Log/LogPanel.h"
#include "IO/StepLoader.h"

#include "Debug/HoleDebugPanel.h"
#include "Feature/HoleFeatureRecognizer.h"
#include "Feature/HoleContextQuery.h"

namespace
{
    namespace HoleContextQuery = OccQtCore::Feature::HoleContextQuery;
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_lastOpenDirectory("C:/work/OccQtCore/model")
{
    ui->setupUi(this);

    m_logger = new OccQtCore::AppLogger(this);
    m_logReporter = std::make_unique<OccQtCore::AppLogReporter>(m_logger);

    setupWindow();
    setupLayout();
    setupViewArea();
    setupLogPanel();
    setupConnections();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupWindow()
{
    setWindowTitle("OccQtCore");

    resize(1000, 700);
    setMinimumSize(800, 500);
}

void MainWindow::setupLayout()
{
    // centralwidget 自体は QMainWindow が広げる。
    // ここでは中身の余白と比率だけ調整する。
    ui->centralwidget->setContentsMargins(0, 0, 0, 0);

    ui->verticalLayout->setContentsMargins(2, 2, 2, 2);
    ui->verticalLayout->setSpacing(2);

    // .ui 側の順番が
    // 0: viewContainer
    // 1: logContainer
    // である前提。
    ui->verticalLayout->setStretch(0, 4);
    ui->verticalLayout->setStretch(1, 1);

    ui->viewContainer->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Expanding
        );

    ui->logContainer->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Preferred
        );

    // ログは最低限の高さだけ確保。
    // 最大高さは固定しないので、ウィンドウ拡大時に伸びられる。
    ui->logContainer->setMinimumHeight(120);
}

void MainWindow::setupViewArea()
{
    m_occView = new OccQtCore::OccView(this);

    auto* viewLayout = new QVBoxLayout(ui->viewContainer);
    viewLayout->setContentsMargins(0, 0, 0, 0);
    viewLayout->setSpacing(0);
    viewLayout->addWidget(m_occView);
}

void MainWindow::setupLogPanel()
{
    m_logPanel = new OccQtCore::LogPanel(this);
    m_logPanel->setLogger(m_logger);

    auto* logLayout = new QVBoxLayout(ui->logContainer);
    logLayout->setContentsMargins(0, 0, 0, 0);
    logLayout->setSpacing(0);
    logLayout->addWidget(m_logPanel);
}

void MainWindow::setupConnections()
{
    // メニュー ファイル
    connect(ui->actionOpen,
            &QAction::triggered,
            this,
            &MainWindow::openStepFileDialog);

    // メニュー 表示
    connect(ui->actionFitAll,
            &QAction::triggered,
            m_occView,
            &OccQtCore::OccView::fitAll);

    connect(ui->actionViewX,
            &QAction::triggered,
            m_occView,
            &OccQtCore::OccView::viewX);

    connect(ui->actionViewY,
            &QAction::triggered,
            m_occView,
            &OccQtCore::OccView::viewY);

    connect(ui->actionViewZ,
            &QAction::triggered,
            m_occView,
            &OccQtCore::OccView::viewZ);

    connect(ui->actionViewIso,
            &QAction::triggered,
            m_occView,
            &OccQtCore::OccView::viewIso);

    connect(ui->actionDisplayShaded,
            &QAction::triggered,
            m_occView,
            &OccQtCore::OccView::setShadedMode);

    connect(ui->actionDisplayWireframe,
            &QAction::triggered,
            m_occView,
            &OccQtCore::OccView::setWireframeMode);

    // メニュー解析
    connect(ui->actionDumpGeometryAnalysisLog,
            &QAction::triggered,
            this,
            &MainWindow::dumpGeometryAnalysisLog);

    connect(ui->actionDumpGeometryDetailDiagnosticsLog,
            &QAction::triggered,
            this,
            &MainWindow::dumpGeometryDetailDiagnosticsLog);

    // メニューツール
    connect(ui->actionHoleDebug, &QAction::triggered, this, &MainWindow::showHoleDebugPanel);

    // ピック
    connect(m_occView,
            &OccQtCore::OccView::shapePicked,
            this,
            &MainWindow::onShapePicked);
}

void MainWindow::openStepFileDialog()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("Open STEP File"),
        defaultOpenDirectory(),
        tr("STEP Files (*.step *.stp *.STEP *.STP);;All Files (*.*)"));

    if (filePath.isEmpty())
    {
        return;
    }

    openStepFile(filePath);
}

void MainWindow::openStepFile(const QString& filePath)
{
    const auto result = OccQtCore::StepLoader::load(filePath);

    if (!result.success)
    {
        m_logReporter->logStepLoadFailed(result.errorMessage);
        return;
    }

    // ドキュメントに保持
    m_document.clear();
    m_document.setFilePath(filePath);
    m_document.setShape(result.shape);

    // ビュー処理
    m_occView->clearLayer(
        OccQtCore::DisplayLayer::Model);
    m_occView->displayShape(
        m_document.shape(),
        OccQtCore::DisplayLayer::Model,
        OccQtCore::DisplayStyle::preset(
            OccQtCore::DisplayStyle::Preset::DefaultShape),
        OccQtCore::DisplayObjectSourceKind::Model,
        -1);
    m_occView->fitAll();

    // ログ
    m_logReporter->logStepLoaded(filePath);
}

void MainWindow::onShapePicked(const OccQtCore::PickResult& result)
{
    m_selectionInfo = OccQtCore::SelectionInfo{};

    m_occView->clearLayer(
        OccQtCore::DisplayLayer::PickOverlay);

    if (!result.hasShape)
    {
        m_logReporter->logSelection(m_selectionInfo);

        if (m_holeDebugPanel)
        {
            m_holeDebugPanel->clearPickedGeometryDetail();
        }

        return;
    }

    const auto& geometryModel = m_document.geometryModel();

    const int elementIndex =
        geometryModel.findElementIndex(result.shape, result.type);

    m_selectionInfo.isValid = true;
    m_selectionInfo.type = result.type;
    m_selectionInfo.shape = result.shape;
    m_selectionInfo.elementIndex = elementIndex;
    m_selectionInfo.sourceDisplayObjectId = result.sourceDisplayObjectId;

    m_occView->displayShape(
        m_selectionInfo.shape,
        OccQtCore::DisplayLayer::PickOverlay,
        OccQtCore::DisplayStyle::preset(
            OccQtCore::DisplayStyle::Preset::PickHighlightFace),
        OccQtCore::DisplayObjectSourceKind::Pick,
        m_selectionInfo.elementIndex);

    m_logReporter->logSelection(m_selectionInfo);

    if (m_holeDebugPanel)
    {
        if (m_selectionInfo.type == OccQtCore::PickedShapeType::Face &&
            m_selectionInfo.elementIndex >= 0)
        {
            m_holeDebugPanel->inspectPickedFace(
                m_selectionInfo.elementIndex);
        }
        else
        {
            m_holeDebugPanel->clearPickedGeometryDetail();
        }
    }
}

QString MainWindow::defaultOpenDirectory() const
{
    if (!m_lastOpenDirectory.isEmpty())
    {
        return m_lastOpenDirectory;
    }

    return QDir::homePath();
}

void MainWindow::updateLastOpenDirectory(const QString& filePath)
{
    const QFileInfo fileInfo(filePath);

    if (fileInfo.exists())
    {
        m_lastOpenDirectory = fileInfo.absolutePath();
    }
}

void MainWindow::dumpGeometryAnalysisLog()
{
    if (!m_logReporter)
    {
        return;
    }

    m_logReporter->logActionStarted("形状解析ログ出力");

    OccQtCore::GeometryLogReport report{
        m_document.geometryModel()
    };

    report.outputDetailDiagnostics = false;

    m_logReporter->logGeometry(report);

    m_logReporter->logActionFinished("形状解析ログ出力");
}

void MainWindow::dumpGeometryDetailDiagnosticsLog()
{
    if (!m_logReporter)
    {
        return;
    }

    m_logReporter->logActionStarted("形状詳細診断ログ出力");

    OccQtCore::GeometryLogReport report{
        m_document.geometryModel()
    };

    report.outputSummary = false;
    report.outputTopologySummary = false;
    report.outputComplexGeometrySummary = false;
    report.outputDetailDiagnostics = true;

    m_logReporter->logGeometry(report);

    m_logReporter->logActionFinished("形状詳細診断ログ出力");
}

void MainWindow::showHoleDebugPanel()
{
    if (!m_holeDebugPanel)
    {
        m_holeDebugPanel = new HoleDebugPanel(this);
        m_holeDebugPanel->setWindowFlag(Qt::Window, true);

        connect(m_holeDebugPanel,
                &HoleDebugPanel::buildRequested,
                this,
                &MainWindow::buildHoleDebugData);

        connect(m_holeDebugPanel,
                &HoleDebugPanel::clearRequested,
                this,
                &MainWindow::clearHoleDebugDisplay);

        connect(m_holeDebugPanel,
                &HoleDebugPanel::exportLogRequested,
                this,
                &MainWindow::exportHoleDebugLog);

        connect(m_holeDebugPanel,
                &HoleDebugPanel::groupSelected,
                this,
                &MainWindow::applyHoleContextGroupSelection);

        connect(m_holeDebugPanel,
                &HoleDebugPanel::traceStepSelected,
                this,
                &MainWindow::applyHoleTraceStepSelection);

        connect(m_holeDebugPanel,
                &HoleDebugPanel::selectionCleared,
                this,
                &MainWindow::clearHoleDebugSelection);
    }

    m_holeDebugPanel->show();
    m_holeDebugPanel->raise();
    m_holeDebugPanel->activateWindow();
}

void MainWindow::buildHoleDebugData()
{
    const auto& geometryModel = m_document.geometryModel();

    OccQtCore::Feature::HoleFeatureRecognizer recognizer;

    m_holeDebugResult =
        recognizer.recognizeCandidates(
            geometryModel,
            &m_holeDebugWorkingData);

    if (m_holeDebugPanel)
    {
        m_holeDebugPanel->setRecognitionResult(m_holeDebugResult);
    }
}

void MainWindow::clearHoleDebugDisplay()
{
    // まず空でOK。あとで3D表示クリアを入れる。
}

void MainWindow::exportHoleDebugLog()
{
    const QString defaultPath =
        QDir(defaultOpenDirectory()).filePath("HoleRecognitionDebug.log");

    const QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("Export Hole Debug Log"),
        defaultPath,
        tr("Log Files (*.log);;Text Files (*.txt);;All Files (*.*)"));

    if (filePath.isEmpty())
    {
        return;
    }

    OccQtCore::HoleRecognitionLogReport report{
        m_document.geometryModel(),
        m_holeDebugResult
    };

    OccQtCore::HoleRecognitionLogReporter reporter(m_logger);
    const QString text = reporter.formatHoleRecognition(report);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        if (m_logger)
        {
            m_logger->info(
                QString("穴認識デバッグログ出力失敗: %1").arg(filePath));
        }
        return;
    }

    QTextStream out(&file);
    out << text;

    file.close();

    if (m_logger)
    {
        m_logger->info(
            QString("穴認識デバッグログ出力完了: %1").arg(filePath));
    }
}

void MainWindow::applyHoleContextGroupSelection(int groupIndex)
{
    const auto* group =
        HoleContextQuery::findGroupByIndex(
            m_holeDebugResult,
            groupIndex);

    if (group == nullptr)
    {
        clearHoleDebugSelection();
        return;
    }

    m_occView->clearLayer(
        OccQtCore::DisplayLayer::TemporaryOverlay);

    const auto& geometryModel = m_document.geometryModel();

    const auto groupStyle = OccQtCore::DisplayStyle::preset(
        OccQtCore::DisplayStyle::Preset::ContextGroupFace);

    for (const int faceIndex : group->geometryRefs.faceIndices)
    {
        const auto* face = geometryModel.faceAt(faceIndex);

        if (face == nullptr)
        {
            continue;
        }

        m_occView->displayShape(
            face->shape,
            OccQtCore::DisplayLayer::TemporaryOverlay,
            groupStyle,
            OccQtCore::DisplayObjectSourceKind::DebugContextGroup,
            groupIndex);
    }

    m_occView->redraw();
}

void MainWindow::applyHoleTracePortSelection(int portIndex)
{
    const auto* port =
        HoleContextQuery::findPortByIndex(
            m_holeDebugResult,
            portIndex);

    if (port == nullptr)
    {
        clearHoleDebugSelection();
        return;
    }

    m_occView->clearLayer(
        OccQtCore::DisplayLayer::TemporaryOverlay);

    const auto& geometryModel = m_document.geometryModel();

    const auto portStyle = OccQtCore::DisplayStyle::preset(
        OccQtCore::DisplayStyle::Preset::TracePortEdge);

    for (const int edgeIndex : port->geometryRefs.edgeIndices)
    {
        const auto* edge = geometryModel.edgeAt(edgeIndex);

        if (edge == nullptr)
        {
            continue;
        }

        m_occView->displayShape(
            edge->shape,
            OccQtCore::DisplayLayer::TemporaryOverlay,
            portStyle,
            OccQtCore::DisplayObjectSourceKind::DebugTracePort,
            portIndex);
    }

    m_occView->redraw();
}

void MainWindow::applyHoleTraceStepSelection(int stepIndex)
{
    const auto* step =
        HoleContextQuery::findStepByIndex(
            m_holeDebugResult,
            stepIndex);

    if (step == nullptr)
    {
        clearHoleDebugSelection();
        return;
    }

    m_occView->clearLayer(
        OccQtCore::DisplayLayer::TemporaryOverlay);

    const auto& geometryModel = m_document.geometryModel();

    const auto outsideFaceStyle = OccQtCore::DisplayStyle::preset(
        OccQtCore::DisplayStyle::Preset::ContextGroupFace);

    for (const int faceIndex : step->outsideGeometryRefs.faceIndices)
    {
        const auto* face = geometryModel.faceAt(faceIndex);

        if (face == nullptr)
        {
            continue;
        }

        m_occView->displayShape(
            face->shape,
            OccQtCore::DisplayLayer::TemporaryOverlay,
            outsideFaceStyle,
            OccQtCore::DisplayObjectSourceKind::DebugTraceStep,
            stepIndex);
    }

    const auto portEdgeStyle = OccQtCore::DisplayStyle::preset(
        OccQtCore::DisplayStyle::Preset::TracePortEdge);

    for (const int edgeIndex : step->portGeometryRefs.edgeIndices)
    {
        const auto* edge = geometryModel.edgeAt(edgeIndex);

        if (edge == nullptr)
        {
            continue;
        }

        m_occView->displayShape(
            edge->shape,
            OccQtCore::DisplayLayer::TemporaryOverlay,
            portEdgeStyle,
            OccQtCore::DisplayObjectSourceKind::DebugTraceStep,
            stepIndex);
    }

    m_occView->redraw();
}

void MainWindow::clearHoleDebugSelection()
{
    m_occView->clearLayer(
        OccQtCore::DisplayLayer::TemporaryOverlay);
}

