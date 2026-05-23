#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "AppLogger.h"
#include "LogPanel.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QSizePolicy>

#include <BRepPrimAPI_MakeBox.hxx>
#include <TopoDS_Shape.hxx>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setupWindow();
    setupLayout();
    setupViewArea();
    setupLogPanel();

    m_logger->info("Application started");

    runOccRuntimeCheck();
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
    m_viewLabel = new QLabel("OccView area", this);
    m_viewLabel->setAlignment(Qt::AlignCenter);

    auto* viewLayout = new QVBoxLayout(ui->viewContainer);
    viewLayout->setContentsMargins(0, 0, 0, 0);
    viewLayout->setSpacing(0);
    viewLayout->addWidget(m_viewLabel);
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

void MainWindow::runOccRuntimeCheck()
{
    TopoDS_Shape testShape = BRepPrimAPI_MakeBox(10.0, 20.0, 30.0).Shape();
    (void)testShape;

    if (m_logger)
    {
        m_logger->info("OCC runtime check OK");
    }
}
