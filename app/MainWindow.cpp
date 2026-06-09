#include <QLabel>
#include <QVBoxLayout>
#include <QSizePolicy>
#include <QTimer>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>

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

namespace
{
    struct HoleDebugShapeSet
    {
        std::vector<TopoDS_Shape> wallFaces;

        std::vector<TopoDS_Shape> openFaces;
        std::vector<TopoDS_Shape> openEdges;

        std::vector<TopoDS_Shape> bottomFaces;
        std::vector<TopoDS_Shape> bottomEdges;

        std::vector<TopoDS_Shape> connectionFaces;
        std::vector<TopoDS_Shape> connectionEdges;
    };
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

    // メニュー フィーチャ認識 > 穴
    connect(
        ui->actionHoleDebug,
        &QAction::triggered,
        this,
        &MainWindow::showHoleDebugPanel);

    // メニュー解析
    connect(ui->actionDumpGeometryAnalysisLog,
            &QAction::triggered,
            this,
            &MainWindow::dumpGeometryAnalysisLog);

    connect(ui->actionDumpGeometryDetailDiagnosticsLog,
            &QAction::triggered,
            this,
            &MainWindow::dumpGeometryDetailDiagnosticsLog);

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
    m_occView->clearLayer(OccQtCore::DisplayLayer::Shape);
    m_occView->displayShape(m_document.shape(), OccQtCore::DisplayLayer::Shape);
    m_occView->fitAll();

    // ログ
    m_logReporter->logStepLoaded(filePath);
}

void MainWindow::onShapePicked(const OccQtCore::PickResult& result)
{
    m_selectionInfo = OccQtCore::SelectionInfo{};

    m_occView->clearLayer(OccQtCore::DisplayLayer::PickHighlight);

    if (!result.hasShape)
    {
        m_logReporter->logSelection(m_selectionInfo);
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

    m_logReporter->logSelection(m_selectionInfo);

    return;
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
    if (m_holeDebugPanel == nullptr)
    {
        m_holeDebugPanel = new HoleDebugPanel(this);

        m_holeDebugPanel->setWindowFlag(Qt::Tool, true);
        m_holeDebugPanel->setWindowTitle("穴認識デバッグ");
        m_holeDebugPanel->resize(360, 420);

        m_holeDebugPanel->setDisplayOptions(
            m_holeDebugDisplayOptions);

        m_holeDebugPanel->setDebugEnabled(
            m_isHoleDebugEnabled);

        setupHoleDebugPanelConnections();
    }

    m_holeDebugPanel->show();
    m_holeDebugPanel->raise();
    m_holeDebugPanel->activateWindow();
}

void MainWindow::setupHoleDebugPanelConnections()
{
    if (m_holeDebugPanel == nullptr)
    {
        return;
    }

    connect(
        m_holeDebugPanel,
        &HoleDebugPanel::debugEnabledChanged,
        this,
        &MainWindow::setHoleDebugEnabled);

    connect(
        m_holeDebugPanel,
        &HoleDebugPanel::displayOptionsChanged,
        this,
        &MainWindow::applyHoleDebugDisplayOptions);

    connect(
        m_holeDebugPanel,
        &HoleDebugPanel::refreshRequested,
        this,
        &MainWindow::refreshHoleDebugDisplay);

    connect(
        m_holeDebugPanel,
        &HoleDebugPanel::rebuildRequested,
        this,
        &MainWindow::rebuildHoleDebugData);
}

void MainWindow::setHoleDebugEnabled(bool enabled)
{
    if (m_isHoleDebugEnabled == enabled)
    {
        return;
    }

    m_isHoleDebugEnabled = enabled;

    if (!m_isHoleDebugEnabled)
    {
        clearHoleDebugDisplay();
        return;
    }

    if (!m_hasHoleDebugData)
    {
        buildHoleDebugData();
        logHoleDebugInfo();
    }

    refreshHoleDebugDisplay();
}

void MainWindow::buildHoleDebugData()
{
    const auto& model = m_document.geometryModel();

    OccQtCore::Feature::HoleFeatureRecognizer recognizer;

    m_holeWallCandidates =
        recognizer.detectWallCandidates(model);

    m_holeEndCandidates =
        recognizer.detectEndCandidates(
            model,
            m_holeWallCandidates
        );

    m_holeSegmentCandidates =
        recognizer.buildSegmentCandidates(
            model,
            m_holeWallCandidates,
            m_holeEndCandidates);

    m_holeCandidates = recognizer.buildHoleCandidatesFromSegments(
        m_holeWallCandidates,
        m_holeEndCandidates,
        m_holeSegmentCandidates);

    m_hasHoleDebugData = true;
}

void MainWindow::rebuildHoleDebugData()
{
    m_hasHoleDebugData = false;

    m_holeWallCandidates.clear();
    m_holeEndCandidates.clear();
    m_holeSegmentCandidates.clear();

    buildHoleDebugData();
    logHoleDebugInfo();

    if (m_isHoleDebugEnabled)
    {
        refreshHoleDebugDisplay();
    }
}

void MainWindow::logHoleDebugInfo()
{
    if (!m_logReporter)
    {
        return;
    }

    const auto& model = m_document.geometryModel();

    m_logReporter->logActionStarted("穴認識デバッグ生成");

    OccQtCore::Feature::HoleRecognitionResult result;
    result.wallCandidates = m_holeWallCandidates;
    result.endCandidates = m_holeEndCandidates;
    result.segmentCandidates = m_holeSegmentCandidates;
    result.holeCandidates = m_holeCandidates;

    OccQtCore::HoleRecognitionLogReport report{model, result};

    report.outputSummary = true;
    report.outputWallCandidates = m_holeDebugLogOptions.logWalls;
    report.outputEndCandidates = m_holeDebugLogOptions.logEnds;
    report.outputSegmentCandidates = m_holeDebugLogOptions.logSegments;
    report.outputHoleCandidates = m_holeDebugLogOptions.logCandidates;

    m_logReporter->logHoleRecognition(report);

    m_logReporter->logActionFinished("穴認識デバッグ生成");
}

void MainWindow::applyHoleDebugDisplayOptions(
    const OccQtCore::Debug::HoleDebugDisplayOptions& options)
{
    m_holeDebugDisplayOptions = options;

    if (m_isHoleDebugEnabled)
    {
        refreshHoleDebugDisplay();
    }
}

void MainWindow::refreshHoleDebugDisplay()
{
    clearHoleDebugDisplay();

    if (!m_isHoleDebugEnabled ||
        !m_hasHoleDebugData)
    {
        return;
    }

    displayHoleDebugData();
}

void MainWindow::clearHoleDebugDisplay()
{
    if (!m_occView)
    {
        return;
    }

    m_occView->clearHoleAnalysisLayers();
}

void MainWindow::displayHoleDebugData()
{
    if (m_occView == nullptr)
    {
        return;
    }

    if (!m_hasHoleDebugData)
    {
        return;
    }

    const auto& model = m_document.geometryModel();
    const auto& options = m_holeDebugDisplayOptions;

    HoleDebugShapeSet shapes;

    auto appendFaceShape =
        [&](std::vector<TopoDS_Shape>& targetShapes, int faceIndex)
    {
        const auto* faceData = model.faceAt(faceIndex);
        if (faceData == nullptr)
        {
            return;
        }

        targetShapes.push_back(faceData->shape);
    };

    auto appendEdgeShape =
        [&](std::vector<TopoDS_Shape>& targetShapes, int edgeIndex)
    {
        const auto* edgeData = model.edgeAt(edgeIndex);
        if (edgeData == nullptr)
        {
            return;
        }

        targetShapes.push_back(edgeData->shape);
    };

    auto appendEndShapes =
        [&](const OccQtCore::Feature::HoleEndCandidate& end)
    {
        std::vector<TopoDS_Shape>* targetFaces = nullptr;
        std::vector<TopoDS_Shape>* targetEdges = nullptr;

        if (end.type == OccQtCore::Feature::HoleEndCandidateType::Open)
        {
            if (!options.showOpenEnds)
            {
                return;
            }

            targetFaces = &shapes.openFaces;
            targetEdges = &shapes.openEdges;
        }
        else if (end.type == OccQtCore::Feature::HoleEndCandidateType::Bottom)
        {
            if (!options.showBottomEnds)
            {
                return;
            }

            targetFaces = &shapes.bottomFaces;
            targetEdges = &shapes.bottomEdges;
        }
        else if (end.type == OccQtCore::Feature::HoleEndCandidateType::WallConnection)
        {
            if (!options.showWallConnectionEnds)
            {
                return;
            }

            targetFaces = &shapes.connectionFaces;
            targetEdges = &shapes.connectionEdges;
        }
        else
        {
            return;
        }

        for (int faceIndex : end.geometryRefs.faceIndices)
        {
            appendFaceShape(*targetFaces, faceIndex);
        }

        for (int edgeIndex : end.geometryRefs.edgeIndices)
        {
            appendEdgeShape(*targetEdges, edgeIndex);
        }
    };

    for (const auto& segment : m_holeSegmentCandidates)
    {
        if (options.targetSegmentIndex >= 0 &&
            segment.index != options.targetSegmentIndex)
        {
            continue;
        }

        if (options.targetWallCandidateIndex >= 0 &&
            segment.wallCandidateIndex != options.targetWallCandidateIndex)
        {
            continue;
        }

        if (options.showWallFaces &&
            segment.wallCandidateIndex >= 0 &&
            segment.wallCandidateIndex < static_cast<int>(m_holeWallCandidates.size()))
        {
            const auto& wall =
                m_holeWallCandidates[segment.wallCandidateIndex];

            for (int faceIndex : wall.geometryRefs.faceIndices)
            {
                appendFaceShape(shapes.wallFaces, faceIndex);
            }
        }

        // Raw End 表示は代表End表示の上位モードとして扱う。
        // showRawEnds=true のときは、代表化前のEndCandidateを全部見る。
        if (options.showRawEnds)
        {
            for (const auto& end : m_holeEndCandidates)
            {
                if (end.wallCandidateIndex != segment.wallCandidateIndex)
                {
                    continue;
                }

                appendEndShapes(end);
            }
        }
        else if (options.showRepresentativeEnds)
        {
            for (int endIndex : segment.endCandidateIndices)
            {
                if (endIndex < 0 ||
                    endIndex >= static_cast<int>(m_holeEndCandidates.size()))
                {
                    continue;
                }

                appendEndShapes(m_holeEndCandidates[endIndex]);
            }
        }
    }

    using DisplayPreset = OccQtCore::DisplayStyle::Preset;

    if (!shapes.wallFaces.empty())
    {
        m_occView->displayShapes(
            shapes.wallFaces,
            OccQtCore::DisplayLayer::AnalysisHoleWall,
            OccQtCore::DisplayStyle::preset(DisplayPreset::HoleWallFace));
    }

    if (!shapes.openFaces.empty())
    {
        m_occView->displayShapes(
            shapes.openFaces,
            OccQtCore::DisplayLayer::AnalysisHoleOpen,
            OccQtCore::DisplayStyle::preset(DisplayPreset::HoleOpenFace));
    }

    if (!shapes.openEdges.empty())
    {
        m_occView->displayShapes(
            shapes.openEdges,
            OccQtCore::DisplayLayer::AnalysisHoleOpen,
            OccQtCore::DisplayStyle::preset(DisplayPreset::HoleOpenEdge));
    }

    if (!shapes.bottomFaces.empty())
    {
        m_occView->displayShapes(
            shapes.bottomFaces,
            OccQtCore::DisplayLayer::AnalysisHoleBottom,
            OccQtCore::DisplayStyle::preset(DisplayPreset::HoleBottomFace));
    }

    if (!shapes.bottomEdges.empty())
    {
        m_occView->displayShapes(
            shapes.bottomEdges,
            OccQtCore::DisplayLayer::AnalysisHoleBottom,
            OccQtCore::DisplayStyle::preset(DisplayPreset::HoleBottomEdge));
    }

    if (!shapes.connectionFaces.empty())
    {
        m_occView->displayShapes(
            shapes.connectionFaces,
            OccQtCore::DisplayLayer::AnalysisHoleConnection,
            OccQtCore::DisplayStyle::preset(DisplayPreset::HoleConnectionFace));
    }

    if (!shapes.connectionEdges.empty())
    {
        m_occView->displayShapes(
            shapes.connectionEdges,
            OccQtCore::DisplayLayer::AnalysisHoleConnection,
            OccQtCore::DisplayStyle::preset(DisplayPreset::HoleConnectionEdge));
    }
}

