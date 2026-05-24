#include <QLabel>
#include <QVBoxLayout>
#include <QSizePolicy>
#include <QTimer>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>

#include <BRepPrimAPI_MakeBox.hxx>
#include <TopoDS_Shape.hxx>

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
    updateLastOpenDirectory(filePath);

    // 一時デバック
    const auto& model = m_document.geometryModel();

    m_logger->info(QString("GeometryModel built: Faces=%1, Edges=%2, Vertices=%3")
                       .arg(model.faceCount())
                       .arg(model.edgeCount())
                       .arg(model.vertexCount()));

    const int faceLogCount = std::min(model.faceCount(), 5);
    const int edgeLogCount = std::min(model.edgeCount(), 10);

    for (int i = 0; i < model.faceCount(); ++i)
    {
        const auto* face = model.faceAt(i);
        if (!face)
        {
            continue;
        }

        m_logger->info(QString("Face[%1]: kind=%2, area=%3")
                           .arg(face->index)
                           .arg(OccQtCore::surfaceKindDisplayName(face->info.kind))
                           .arg(face->info.area));

        if (face->info.cylinder)
        {
            const auto& axisDir = face->info.cylinder->axis.Direction();

            m_logger->info(QString("  円筒面 半径=%1, 軸方向=(%2, %3, %4)")
                               .arg(face->info.cylinder->radius)
                               .arg(axisDir.X())
                               .arg(axisDir.Y())
                               .arg(axisDir.Z()));
        }
    }

    for (int i = 0; i < model.edgeCount(); ++i)
    {
        const auto* edge = model.edgeAt(i);
        if (!edge)
        {
            continue;
        }

        if (!edge->info.circle)
        {
            continue;
        }

        const auto& center = edge->info.circle->center;
        const auto& axisDir = edge->info.circle->axis.Direction();

        m_logger->info(QString("Circle Edge[%1]: center=(%2, %3, %4), radius=%5, axis=(%6, %7, %8), length=%9")
                           .arg(edge->index)
                           .arg(center.X())
                           .arg(center.Y())
                           .arg(center.Z())
                           .arg(edge->info.circle->radius)
                           .arg(axisDir.X())
                           .arg(axisDir.Y())
                           .arg(axisDir.Z())
                           .arg(edge->info.length));
    }

    // ビュー同期
    m_occView->clearLayer(OccQtCore::DisplayLayer::Shape);
    m_occView->displayShape(m_document.shape(), OccQtCore::DisplayLayer::Shape);
    m_occView->fitAll();

    const QFileInfo fileInfo(m_document.filePath());
    setWindowTitle(QString("OccQtCore - %1").arg(fileInfo.fileName()));

    m_logger->info(QString("STEP file loaded: %1").arg(filePath));
}

void MainWindow::onShapePicked(const OccQtCore::PickResult& result)
{
    m_selectionInfo = OccQtCore::SelectionInfo{};

    if (!result.hasShape)
    {
        m_logger->info("Selected: none");
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


