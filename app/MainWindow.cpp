#include <algorithm>
#include <vector>

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
#include "Log/LogPanel.h"
#include "IO/StepLoader.h"

namespace
{
    QString pickedShapeTypeToString(OccQtCore::PickedShapeType type)
    {
        switch (type)
        {
        case OccQtCore::PickedShapeType::Vertex:
            return "Vertex";
        case OccQtCore::PickedShapeType::Edge:
            return "Edge";
        case OccQtCore::PickedShapeType::Face:
            return "Face";
        case OccQtCore::PickedShapeType::Solid:
            return "Solid";
        case OccQtCore::PickedShapeType::Unknown:
        default:
            return "Unknown";
        }
    }
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_lastOpenDirectory("C:/work/OccQtCore/model")
{
    ui->setupUi(this);

    setupWindow();
    setupLayout();
    setupViewArea();
    setupLogPanel();
    setupConnections();

    m_logger->info("Application started");
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
    m_logger = new OccQtCore::AppLogger(this);

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
        m_logger->error(result.errorMessage);
        return;
    }

    m_document.clear();
    m_document.setFilePath(filePath);
    m_document.setShape(result.shape);

    const auto& model = m_document.geometryModel();

    m_logger->info(QString("GeometryModel built: Faces=%1, Edges=%2, Vertices=%3")
                       .arg(model.faceCount())
                       .arg(model.edgeCount())
                       .arg(model.vertexCount()));

    m_logger->info(QString("GeometryGraph built: Faces=%1, Edges=%2, Vertices=%3")
                       .arg(model.graph().faceCount())
                       .arg(model.graph().edgeCount())
                       .arg(model.graph().vertexCount()));

    if (model.faceCount() > 0)
    {
        m_logger->info(QString("Graph Face[0]: connected edges=%1")
                           .arg(model.graph().edgesOfFace(0).size()));
    }

    if (model.edgeCount() > 0)
    {
        m_logger->info(QString("Graph Edge[0]: connected faces=%1, vertices=%2")
                           .arg(model.graph().facesOfEdge(0).size())
                           .arg(model.graph().verticesOfEdge(0).size()));
    }

    int zeroEdgeFaceCount = 0;
    int nonTwoVertexEdgeCount = 0;

    for (int i = 0; i < model.edgeCount(); ++i)
    {
        if (model.graph().facesOfEdge(i).empty())
        {
            ++zeroEdgeFaceCount;
        }

        if (model.graph().verticesOfEdge(i).size() != 2)
        {
            ++nonTwoVertexEdgeCount;
        }
    }

    m_logger->info(QString("Graph check: edges without faces=%1, edges with non-2 vertices=%2")
                       .arg(zeroEdgeFaceCount)
                       .arg(nonTwoVertexEdgeCount));

    m_occView->clearLayer(OccQtCore::DisplayLayer::Shape);
    m_occView->displayShape(m_document.shape(), OccQtCore::DisplayLayer::Shape);
    m_occView->fitAll();

    const QFileInfo fileInfo(m_document.filePath());
    setWindowTitle(QString("OccQtCore - %1").arg(fileInfo.fileName()));

    updateLastOpenDirectory(filePath);

    m_logger->info(QString("STEP file loaded: %1").arg(filePath));
}

void MainWindow::onShapePicked(const OccQtCore::PickResult& result)
{
    m_selectionInfo = OccQtCore::SelectionInfo{};

    if (!result.hasShape)
    {
        m_logger->info("Selected: none");
        m_occView->clearLayer(OccQtCore::DisplayLayer::PickHighlight);
        return;
    }

    const auto& geometryModel = m_document.geometryModel();
    const int elementIndex = geometryModel.findElementIndex(result.shape, result.type);

    m_selectionInfo.isValid = true;
    m_selectionInfo.type = result.type;
    m_selectionInfo.shape = result.shape;
    m_selectionInfo.elementIndex = elementIndex;
    m_selectionInfo.sourceDisplayObjectId = result.sourceDisplayObjectId;

    m_logger->info(
        QString("Selected: %1, Index=%2, SourceDisplayObjectId=%3")
            .arg(pickedShapeTypeToString(m_selectionInfo.type))
            .arg(m_selectionInfo.elementIndex)
            .arg(m_selectionInfo.sourceDisplayObjectId));

    m_occView->clearLayer(OccQtCore::DisplayLayer::PickHighlight);

    if (elementIndex < 0)
    {
        m_logger->warn("Graph log skipped: selected shape index was not found.");
        return;
    }

    const auto& graph = geometryModel.graph();

    if (result.type == OccQtCore::PickedShapeType::Face)
    {
        const OccQtCore::DisplayStyle selectedFaceStyle{
            Quantity_Color(Quantity_NOC_ORANGE),
            0.2,
            AIS_Shaded
        };

        const OccQtCore::DisplayStyle adjacentFaceStyle{
            Quantity_Color(Quantity_NOC_CYAN1),
            0.55,
            AIS_Shaded
        };

        m_occView->displayShape(
            TopoDS::Face(result.shape),
            OccQtCore::DisplayLayer::PickHighlight,
            selectedFaceStyle);

        const auto& edgeIndices = graph.edgesOfFace(elementIndex);

        std::vector<int> neighborFaceIndices;

        m_logger->info(
            QString("Graph Face[%1]: connected edges=%2")
                .arg(elementIndex)
                .arg(edgeIndices.size()));

        for (int edgeIndex : edgeIndices)
        {
            const auto& connectedFaceIndices = graph.facesOfEdge(edgeIndex);

            QString connectedFacesText;
            QString neighborFacesText;

            for (int faceIndex : connectedFaceIndices)
            {
                if (!connectedFacesText.isEmpty())
                {
                    connectedFacesText += ", ";
                }
                connectedFacesText += QString::number(faceIndex);

                if (faceIndex == elementIndex)
                {
                    continue;
                }

                if (!neighborFacesText.isEmpty())
                {
                    neighborFacesText += ", ";
                }
                neighborFacesText += QString::number(faceIndex);

                if (std::find(
                        neighborFaceIndices.begin(),
                        neighborFaceIndices.end(),
                        faceIndex) == neighborFaceIndices.end())
                {
                    neighborFaceIndices.push_back(faceIndex);
                }
            }

            m_logger->info(
                QString("  Edge[%1]: connected faces=[%2], neighbor faces=[%3]")
                    .arg(edgeIndex)
                    .arg(connectedFacesText)
                    .arg(neighborFacesText));
        }

        for (int neighborFaceIndex : neighborFaceIndices)
        {
            const auto& faceData = geometryModel.faces().at(neighborFaceIndex);

            m_occView->displayShape(
                TopoDS::Face(faceData.shape),
                OccQtCore::DisplayLayer::PickHighlight,
                adjacentFaceStyle);
        }
    }
    else if (result.type == OccQtCore::PickedShapeType::Edge)
    {
        const auto& faceIndices = graph.facesOfEdge(elementIndex);
        const auto& vertexIndices = graph.verticesOfEdge(elementIndex);

        m_logger->info(
            QString("Graph Edge[%1]: connected faces=%2, vertices=%3")
                .arg(elementIndex)
                .arg(faceIndices.size())
                .arg(vertexIndices.size()));
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


