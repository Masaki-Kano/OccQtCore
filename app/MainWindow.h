#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>

#include <QLabel>
#include <QMainWindow>

#include "Core/DocumentData.h"
#include "Core/SelectionInfo.h"

#include "Feature/HoleRecognitionModel.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

namespace OccQtCore {
class AppLogger;
class AppLogReporter;
class LogPanel;
class OccView;
struct PickResult;
}

class HoleDebugPanel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void setupWindow();
    void setupLayout();
    void setupViewArea();
    void setupLogPanel();
    void setupConnections();

    void openStepFileDialog();
    void openStepFile(const QString& filePath);

    void onShapePicked(const OccQtCore::PickResult& result);

    QString defaultOpenDirectory() const;
    void updateLastOpenDirectory(const QString& filePath);

    void dumpGeometryAnalysisLog();
    void dumpGeometryDetailDiagnosticsLog();

    // 穴認識デバック
    void showHoleDebugPanel();
    void buildHoleDebugData();
    void clearHoleDebugDisplay();
    void exportHoleDebugLog();
    void applyHoleWallSelection(int wallIndex);
    void applyHoleBoundarySelection(int boundaryIndex);
    void clearHoleDebugSelection();


private:
    Ui::MainWindow* ui = nullptr;

    OccQtCore::AppLogger* m_logger = nullptr;
    std::unique_ptr<OccQtCore::AppLogReporter> m_logReporter;
    OccQtCore::LogPanel* m_logPanel = nullptr;
    OccQtCore::OccView* m_occView = nullptr;
    OccQtCore::DocumentData m_document;
    OccQtCore::SelectionInfo m_selectionInfo;
    QString m_lastOpenDirectory;

    HoleDebugPanel* m_holeDebugPanel = nullptr;
    OccQtCore::Feature::HoleRecognitionResult m_holeDebugResult;
};
#endif // MAINWINDOW_H
