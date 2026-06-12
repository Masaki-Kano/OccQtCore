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
        m_holeDebugPanel->resize(900, 650);

        m_holeDebugPanel->setDisplayOptions(
            m_holeDebugDisplayOptions);

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
        &HoleDebugPanel::displayOptionsChanged,
        this,
        &MainWindow::applyHoleDebugDisplayOptions);

    connect(
        m_holeDebugPanel,
        &HoleDebugPanel::buildRequested,
        this,
        &MainWindow::buildHoleDebugData);

    connect(
        m_holeDebugPanel,
        &HoleDebugPanel::refreshDisplayRequested,
        this,
        &MainWindow::refreshHoleDebugDisplay);

    connect(
        m_holeDebugPanel,
        &HoleDebugPanel::clearDisplayRequested,
        this,
        &MainWindow::clearHoleDebugDisplay);

    connect(
        m_holeDebugPanel,
        &HoleDebugPanel::exportDetailLogRequested,
        this,
        &MainWindow::logHoleDebugInfo);

    connect(
        m_holeDebugPanel,
        &HoleDebugPanel::selectedItemChanged,
        this,
        &MainWindow::applyHoleDebugSelectedItem);
}

void MainWindow::buildHoleDebugData()
{
    m_hasHoleRecognitionResult = false;
    m_holeRecognitionResult = {};

    const auto& model = m_document.geometryModel();

    OccQtCore::Feature::HoleFeatureRecognizer recognizer;

    m_holeRecognitionResult = recognizer.recognizeCandidates(model);

    m_hasHoleRecognitionResult = true;

    if (m_holeDebugPanel != nullptr)
    {
        m_holeDebugPanel->setStatusText(
            QString("状態: 生成済み  Wall=%1  End=%2  Segment=%3  Hole=%4")
                .arg(m_holeRecognitionResult.wallCandidates.size())
                .arg(m_holeRecognitionResult.endCandidates.size())
                .arg(m_holeRecognitionResult.segmentCandidates.size())
                .arg(m_holeRecognitionResult.holeCandidates.size()));

        m_holeDebugPanel->setRecognitionResult(m_holeRecognitionResult);
    }

    refreshHoleDebugDisplay();
}

void MainWindow::logHoleDebugInfo()
{
    if (!m_logReporter)
    {
        return;
    }

    if (!m_hasHoleRecognitionResult)
    {
        m_logReporter->logActionFailed(
            "穴認識デバックログ出力",
            "穴認識結果が生成されていません");

        return;
    }

    const auto& model = m_document.geometryModel();

    m_logReporter->logActionStarted("穴認識デバックログ出力");

    OccQtCore::HoleRecognitionLogReport report{
        model,
        m_holeRecognitionResult
    };

    // TODO: あとでログをそろえる

    m_logReporter->logHoleRecognition(report);

    m_logReporter->logActionFinished("穴認識デバッグログ出力");
}

void MainWindow::applyHoleDebugDisplayOptions(const OccQtCore::Debug::HoleDebugDisplayOptions& options)
{
    m_holeDebugDisplayOptions = options;

    if (m_hasHoleRecognitionResult)
    {
        refreshHoleDebugDisplay();
    }
}

void MainWindow::applyHoleDebugSelectedItem(
    const OccQtCore::Debug::HoleDebugSelectedItem& selected)
{
    m_holeDebugDisplayOptions.selectedItem = selected;

    if (m_hasHoleRecognitionResult)
    {
        refreshHoleDebugDisplay();
    }
}

void MainWindow::refreshHoleDebugDisplay()
{
    clearHoleDebugDisplay();

    if (!m_hasHoleRecognitionResult)
    {
        return;
    }

    displayHoleDebugData();
}

void MainWindow::clearHoleDebugDisplay()
{
    if (m_occView == nullptr)
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

    if (!m_hasHoleRecognitionResult)
    {
        return;
    }

    const auto& model = m_document.geometryModel();
    const auto& result = m_holeRecognitionResult;
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

    auto appendGeometryRefs =
        [&](const OccQtCore::Feature::GeometryRefs& refs,
            std::vector<TopoDS_Shape>& targetFaces,
            std::vector<TopoDS_Shape>& targetEdges)
    {
        for (int faceIndex : refs.faceIndices)
        {
            appendFaceShape(targetFaces, faceIndex);
        }

        for (int edgeIndex : refs.edgeIndices)
        {
            appendEdgeShape(targetEdges, edgeIndex);
        }
    };

    auto appendWall =
        [&](const OccQtCore::Feature::Hole::Wall& wall)
    {
        for (int faceIndex : wall.geometryRefs.faceIndices)
        {
            appendFaceShape(shapes.wallFaces, faceIndex);
        }
    };

    auto appendEnd =
        [&](const OccQtCore::Feature::Hole::End& end)
    {
        std::vector<TopoDS_Shape>* targetFaces = nullptr;
        std::vector<TopoDS_Shape>* targetEdges = nullptr;

        switch (end.endType)
        {
        case OccQtCore::Feature::Hole::EndType::Open:
            targetFaces = &shapes.openFaces;
            targetEdges = &shapes.openEdges;
            break;

        case OccQtCore::Feature::Hole::EndType::Bottom:
            targetFaces = &shapes.bottomFaces;
            targetEdges = &shapes.bottomEdges;
            break;

        case OccQtCore::Feature::Hole::EndType::Connected:
            targetFaces = &shapes.connectionFaces;
            targetEdges = &shapes.connectionEdges;
            break;

        case OccQtCore::Feature::Hole::EndType::Unknown:
        default:
            return;
        }

        appendGeometryRefs(
            end.geometryRefs,
            *targetFaces,
            *targetEdges);
    };

    auto appendConnection =
        [&](const OccQtCore::Feature::Hole::ElementConnection& connection)
    {
        appendGeometryRefs(
            connection.geometryRefs,
            shapes.connectionFaces,
            shapes.connectionEdges);
    };

    auto appendElement =
        [&](const OccQtCore::Feature::Hole::Element& element)
    {
        appendWall(element.wall);

        for (const auto& end : element.ends)
        {
            appendEnd(end);
        }
    };

    auto appendHole =
        [&](const OccQtCore::Feature::Hole::Data& hole)
    {
        for (const auto& element : hole.elements)
        {
            appendElement(element);
        }

        for (const auto& connection : hole.elementConnections)
        {
            appendConnection(connection);
        }
    };

    auto holeAt =
        [&](int holeIndex) -> const OccQtCore::Feature::Hole::Data*
    {
        if (holeIndex < 0 ||
            holeIndex >= static_cast<int>(result.holes.size()))
        {
            return nullptr;
        }

        return &result.holes[holeIndex];
    };

    auto elementAt =
        [&](int holeIndex,
            int elementIndex) -> const OccQtCore::Feature::Hole::Element*
    {
        const auto* hole = holeAt(holeIndex);
        if (hole == nullptr)
        {
            return nullptr;
        }

        if (elementIndex < 0 ||
            elementIndex >= static_cast<int>(hole->elements.size()))
        {
            return nullptr;
        }

        return &hole->elements[elementIndex];
    };

    auto endAt =
        [&](int holeIndex,
            int elementIndex,
            int endIndex) -> const OccQtCore::Feature::Hole::End*
    {
        const auto* element =
            elementAt(
                holeIndex,
                elementIndex);

        if (element == nullptr)
        {
            return nullptr;
        }

        if (endIndex < 0 ||
            endIndex >= static_cast<int>(element->ends.size()))
        {
            return nullptr;
        }

        return &element->ends[endIndex];
    };

    auto connectionAt =
        [&](int holeIndex,
            int connectionIndex) -> const OccQtCore::Feature::Hole::ElementConnection*
    {
        const auto* hole = holeAt(holeIndex);
        if (hole == nullptr)
        {
            return nullptr;
        }

        if (connectionIndex < 0 ||
            connectionIndex >= static_cast<int>(hole->elementConnections.size()))
        {
            return nullptr;
        }

        return &hole->elementConnections[connectionIndex];
    };

    using Scope = OccQtCore::Debug::HoleDebugDisplayScope;
    using SelectedType = OccQtCore::Debug::HoleDebugSelectedItem::Type;

    const auto& selected =
        options.selectedItem;

    if (options.scope == Scope::All)
    {
        for (const auto& hole : result.holes)
        {
            appendHole(hole);
        }
    }
    else
    {
        switch (selected.type)
        {
        case SelectedType::Hole:
        {
            const auto* hole =
                holeAt(selected.holeIndex);

            if (hole != nullptr)
            {
                appendHole(*hole);
            }

            break;
        }

        case SelectedType::Element:
        {
            const auto* element =
                elementAt(
                    selected.holeIndex,
                    selected.elementIndex);

            if (element != nullptr)
            {
                appendElement(*element);
            }

            break;
        }

        case SelectedType::Wall:
        {
            const auto* element =
                elementAt(
                    selected.holeIndex,
                    selected.elementIndex);

            if (element != nullptr)
            {
                appendWall(element->wall);
            }

            break;
        }

        case SelectedType::End:
        {
            const auto* end =
                endAt(
                    selected.holeIndex,
                    selected.elementIndex,
                    selected.childIndex);

            if (end != nullptr)
            {
                appendEnd(*end);
            }

            break;
        }

        case SelectedType::None:
        default:
            break;
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

