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
    connect(ui->actionDetectHoleWallCandidates,
            &QAction::triggered,
            this,
            &MainWindow::detectHoleWallCandidates);

    connect(ui->actionBuildHoleWallComponents,
            &QAction::triggered,
            this,
            &MainWindow::buildHoleWallComponents);

    connect(ui->actionBuildHoleEndComponents,
            &QAction::triggered,
            this,
            &MainWindow::buildHoleEndComponentsFromWallComponents);

    connect(ui->actionBuildHoleElements,
            &QAction::triggered,
            this,
            &MainWindow::buildHoleElements);

    connect(ui->actionBuildHoleCandidates,
            &QAction::triggered,
            this,
            &MainWindow::buildHoleCandidates);

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
    m_logReporter->logGeometryAnalysisReport(m_document.geometryModel());
    m_logReporter->logActionFinished("形状解析ログ出力");
}

void MainWindow::dumpGeometryDetailDiagnosticsLog()
{
    if (!m_logReporter)
    {
        return;
    }

    m_logReporter->logActionStarted("形状詳細診断ログ出力");
    m_logReporter->logGeometryDetailDiagnostics(m_document.geometryModel());
    m_logReporter->logActionFinished("形状詳細診断ログ出力");
}

void MainWindow::detectHoleWallCandidates()
{
    const auto& model = m_document.geometryModel();

    m_logReporter->logActionStarted("穴壁候補生成");

    OccQtCore::Feature::HoleFeatureRecognizer recognizer;
    const auto candidates = recognizer.detectWallCandidates(model);

    m_logReporter->logHoleWallCandidates(model, candidates);

    m_occView->clearLayer(OccQtCore::DisplayLayer::Analysis);
    displayHoleWallCandidates(candidates);

    m_logReporter->logActionFinished("穴壁候補生成");
}

void MainWindow::buildHoleEndComponentsFromWallComponents()
{
    if (!m_logReporter)
    {
        return;
    }

    const auto& model = m_document.geometryModel();

    m_logReporter->logActionStarted("穴端コンポーネント生成");

    OccQtCore::Feature::HoleFeatureRecognizer recognizer;

    const auto wallCandidates = recognizer.detectWallCandidates(model);
    const auto wallComponents = recognizer.buildWallComponents(model, wallCandidates);
    const auto endComponents =
        recognizer.buildEndComponentsFromWallComponents(model, wallComponents);

    m_logReporter->logHoleWallCandidates(model, wallCandidates);
    m_logReporter->logHoleWallComponents(model, wallComponents);
    m_logReporter->logHoleEndComponents(model, endComponents);

    m_occView->clearLayer(OccQtCore::DisplayLayer::Analysis);
    displayHoleEndComponents(endComponents);

    m_logReporter->logActionFinished("穴端コンポーネント生成");
}

void MainWindow::buildHoleWallComponents()
{
    const auto& model = m_document.geometryModel();

    m_logReporter->logActionStarted("穴壁コンポーネント生成");

    OccQtCore::Feature::HoleFeatureRecognizer recognizer;

    const auto candidates = recognizer.detectWallCandidates(model);
    const auto components = recognizer.buildWallComponents(model, candidates);

    m_logReporter->logHoleWallCandidates(model, candidates);
    m_logReporter->logHoleWallComponents(model, components);

    m_occView->clearLayer(OccQtCore::DisplayLayer::Analysis);
    displayHoleWallComponents(components);

    m_logReporter->logActionFinished("穴壁コンポーネント生成");
}

void MainWindow::buildHoleElements()
{
    if (!m_logReporter)
    {
        return;
    }

    const auto& model = m_document.geometryModel();

    m_logReporter->logActionStarted("穴要素生成");

    OccQtCore::Feature::HoleFeatureRecognizer recognizer;

    const auto wallCandidates = recognizer.detectWallCandidates(model);
    const auto wallComponents = recognizer.buildWallComponents(model, wallCandidates);
    const auto endComponents =
        recognizer.buildEndComponentsFromWallComponents(model, wallComponents);
    const auto holeElements =
        recognizer.buildHoleElements(model, wallComponents, endComponents);

    m_logReporter->logHoleWallCandidates(model, wallCandidates);
    m_logReporter->logHoleWallComponents(model, wallComponents);
    m_logReporter->logHoleEndComponents(model, endComponents);
    m_logReporter->logHoleElements(model, holeElements, endComponents);

    m_occView->clearLayer(OccQtCore::DisplayLayer::Analysis);
    displayHoleElements(holeElements, wallComponents, endComponents);

    m_logReporter->logActionFinished("穴要素生成");
}

void MainWindow::buildHoleCandidates()
{
    if (!m_logReporter)
    {
        return;
    }

    const auto& model = m_document.geometryModel();

    m_logReporter->logActionStarted("穴候補生成");

    OccQtCore::Feature::HoleFeatureRecognizer recognizer;

    const auto wallCandidates =
        recognizer.detectWallCandidates(model);

    const auto wallComponents =
        recognizer.buildWallComponents(
            model,
            wallCandidates);

    const auto endComponents =
        recognizer.buildEndComponentsFromWallComponents(
            model,
            wallComponents);

    const auto holeElements =
        recognizer.buildHoleElements(
            model,
            wallComponents,
            endComponents);

    const auto holeCandidates =
        recognizer.buildHoleCandidatesFromElements(
            wallComponents,
            holeElements);

    const auto stepConnections =
        recognizer.buildStepConnectionsInHoleCandidates(
            model,
            holeCandidates,
            holeElements,
            wallComponents,
            endComponents);

    m_logReporter->logHoleWallCandidates(model, wallCandidates);
    m_logReporter->logHoleWallComponents(model, wallComponents);
    m_logReporter->logHoleEndComponents(model, endComponents);
    m_logReporter->logHoleElements(model, holeElements, endComponents);
    m_logReporter->logHoleCandidates(
        model,
        holeCandidates,
        holeElements,
        wallComponents,
        endComponents);

    m_logReporter->logHoleElementStepConnections(stepConnections);

    m_occView->clearLayer(OccQtCore::DisplayLayer::Analysis);
    displayHoleCandidates(
        holeCandidates,
        holeElements,
        wallComponents,
        endComponents);

    m_logReporter->logActionFinished("穴候補生成");
}

void MainWindow::displayHoleWallCandidates(
    const std::vector<OccQtCore::Feature::HoleWallCandidate>& candidates)
{
    const auto& model = m_document.geometryModel();

    std::vector<TopoDS_Shape> faceShapes;

    for (const auto& candidate : candidates)
    {
        const auto* faceData = model.faceAt(candidate.faceIndex);
        if (faceData == nullptr)
        {
            continue;
        }

        faceShapes.push_back(faceData->shape);
    }

    if (faceShapes.empty())
    {
        return;
    }

    m_occView->displayShapes(
        faceShapes,
        OccQtCore::DisplayLayer::Analysis,
        OccQtCore::DisplayStyle::analysisAdjacentFace());
}

void MainWindow::displayHoleEndComponents(
    const std::vector<OccQtCore::Feature::HoleEndComponent>& components)
{
    const auto& model = m_document.geometryModel();

    std::vector<TopoDS_Shape> edgeShapes;
    std::vector<TopoDS_Shape> faceShapes;

    for (const auto& component : components)
    {
        for (int edgeIndex : component.geometryRefs.edgeIndices)
        {
            const auto* edgeData = model.edgeAt(edgeIndex);
            if (edgeData == nullptr)
            {
                continue;
            }

            edgeShapes.push_back(edgeData->shape);
        }

        for (int faceIndex : component.geometryRefs.faceIndices)
        {
            const auto* faceData = model.faceAt(faceIndex);
            if (faceData == nullptr)
            {
                continue;
            }

            faceShapes.push_back(faceData->shape);
        }
    }

    if (!edgeShapes.empty())
    {
        m_occView->displayShapes(
            edgeShapes,
            OccQtCore::DisplayLayer::Analysis,
            OccQtCore::DisplayStyle::analysisComponentEdge());
    }

    if (!faceShapes.empty())
    {
        m_occView->displayShapes(
            faceShapes,
            OccQtCore::DisplayLayer::Analysis,
            OccQtCore::DisplayStyle::analysisAdjacentFace());
    }
}

void MainWindow::displayHoleWallComponents(
    const std::vector<OccQtCore::Feature::HoleWallComponent>& components)
{
    const auto& model = m_document.geometryModel();

    std::vector<TopoDS_Shape> faceShapes;

    for (const auto& component : components)
    {
        for (int faceIndex : component.geometryRefs.faceIndices)
        {
            const auto* faceData = model.faceAt(faceIndex);
            if (faceData == nullptr)
            {
                continue;
            }

            faceShapes.push_back(faceData->shape);
        }
    }

    if (faceShapes.empty())
    {
        return;
    }

    m_occView->displayShapes(
        faceShapes,
        OccQtCore::DisplayLayer::Analysis,
        OccQtCore::DisplayStyle::analysisAdjacentFace());
}

void MainWindow::displayHoleElements(
    const std::vector<OccQtCore::Feature::HoleElement>& elements,
    const std::vector<OccQtCore::Feature::HoleWallComponent>& wallComponents,
    const std::vector<OccQtCore::Feature::HoleEndComponent>& endComponents)
{
    const auto& model = m_document.geometryModel();

    std::vector<TopoDS_Shape> faceShapes;
    std::vector<TopoDS_Shape> edgeShapes;

    for (const auto& element : elements)
    {
        if (element.wallComponentIndex >= 0 &&
            element.wallComponentIndex < static_cast<int>(wallComponents.size()))
        {
            const auto& wall = wallComponents[element.wallComponentIndex];

            for (int faceIndex : wall.geometryRefs.faceIndices)
            {
                const auto* faceData = model.faceAt(faceIndex);
                if (faceData == nullptr)
                {
                    continue;
                }

                faceShapes.push_back(faceData->shape);
            }
        }

        for (int endIndex : element.endComponentIndices)
        {
            if (endIndex < 0 ||
                endIndex >= static_cast<int>(endComponents.size()))
            {
                continue;
            }

            const auto& end = endComponents[endIndex];

            for (int faceIndex : end.geometryRefs.faceIndices)
            {
                const auto* faceData = model.faceAt(faceIndex);
                if (faceData == nullptr)
                {
                    continue;
                }

                faceShapes.push_back(faceData->shape);
            }

            for (int edgeIndex : end.geometryRefs.edgeIndices)
            {
                const auto* edgeData = model.edgeAt(edgeIndex);
                if (edgeData == nullptr)
                {
                    continue;
                }

                edgeShapes.push_back(edgeData->shape);
            }
        }
    }

    if (!faceShapes.empty())
    {
        m_occView->displayShapes(
            faceShapes,
            OccQtCore::DisplayLayer::Analysis,
            OccQtCore::DisplayStyle::analysisAdjacentFace());
    }

    if (!edgeShapes.empty())
    {
        m_occView->displayShapes(
            edgeShapes,
            OccQtCore::DisplayLayer::Analysis,
            OccQtCore::DisplayStyle::analysisComponentEdge());
    }
}

void MainWindow::displayHoleCandidates(
    const std::vector<OccQtCore::Feature::HoleCandidate>& candidates,
    const std::vector<OccQtCore::Feature::HoleElement>& elements,
    const std::vector<OccQtCore::Feature::HoleWallComponent>& wallComponents,
    const std::vector<OccQtCore::Feature::HoleEndComponent>& endComponents)
{
    const auto& model = m_document.geometryModel();

    std::vector<TopoDS_Shape> faceShapes;
    std::vector<TopoDS_Shape> edgeShapes;

    for (const auto& candidate : candidates)
    {
        for (int elementIndex : candidate.elementIndices)
        {
            if (elementIndex < 0 ||
                elementIndex >= static_cast<int>(elements.size()))
            {
                continue;
            }

            const auto& element = elements[elementIndex];

            if (element.wallComponentIndex >= 0 &&
                element.wallComponentIndex < static_cast<int>(wallComponents.size()))
            {
                const auto& wall =
                    wallComponents[element.wallComponentIndex];

                for (int faceIndex : wall.geometryRefs.faceIndices)
                {
                    const auto* faceData = model.faceAt(faceIndex);
                    if (faceData == nullptr)
                    {
                        continue;
                    }

                    faceShapes.push_back(faceData->shape);
                }
            }

            for (int endIndex : element.endComponentIndices)
            {
                if (endIndex < 0 ||
                    endIndex >= static_cast<int>(endComponents.size()))
                {
                    continue;
                }

                const auto& end = endComponents[endIndex];

                for (int faceIndex : end.geometryRefs.faceIndices)
                {
                    const auto* faceData = model.faceAt(faceIndex);
                    if (faceData == nullptr)
                    {
                        continue;
                    }

                    faceShapes.push_back(faceData->shape);
                }

                for (int edgeIndex : end.geometryRefs.edgeIndices)
                {
                    const auto* edgeData = model.edgeAt(edgeIndex);
                    if (edgeData == nullptr)
                    {
                        continue;
                    }

                    edgeShapes.push_back(edgeData->shape);
                }
            }
        }
    }

    if (!faceShapes.empty())
    {
        m_occView->displayShapes(
            faceShapes,
            OccQtCore::DisplayLayer::Analysis,
            OccQtCore::DisplayStyle::analysisAdjacentFace());
    }

    if (!edgeShapes.empty())
    {
        m_occView->displayShapes(
            edgeShapes,
            OccQtCore::DisplayLayer::Analysis,
            OccQtCore::DisplayStyle::analysisComponentEdge());
    }
}

