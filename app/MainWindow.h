#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <vector>
#include <memory>

#include <QLabel>
#include <QMainWindow>

#include "Core/DocumentData.h"
#include "Core/SelectionInfo.h"

// 一時デバック用のインクルード
#include "Feature/HoleFeatureRecognizer.h"
#include "Debug/HoleDebugOptions.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class HoleDebugPanel;

namespace OccQtCore {
class AppLogger;
class AppLogReporter;
class LogPanel;
class OccView;
struct PickResult;
}

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

    // 穴デバッグウィンドウ
    void showHoleDebugPanel();
    void setupHoleDebugPanelConnections();

    // 穴認識デバッグ制御
    void setHoleDebugEnabled(bool enabled);
    void buildHoleDebugData();
    void rebuildHoleDebugData();
    void logHoleDebugInfo();

    void applyHoleDebugDisplayOptions(
        const OccQtCore::Debug::HoleDebugDisplayOptions& options);

    void refreshHoleDebugDisplay();
    void clearHoleDebugDisplay();
    void displayHoleDebugData();


private:
    Ui::MainWindow* ui = nullptr;

    OccQtCore::AppLogger* m_logger = nullptr;
    std::unique_ptr<OccQtCore::AppLogReporter> m_logReporter;
    OccQtCore::LogPanel* m_logPanel = nullptr;
    OccQtCore::OccView* m_occView = nullptr;

    HoleDebugPanel* m_holeDebugPanel = nullptr;

    OccQtCore::DocumentData m_document;
    OccQtCore::SelectionInfo m_selectionInfo;
    QString m_lastOpenDirectory;

    bool m_isHoleDebugEnabled = false;
    bool m_hasHoleDebugData = false;

    std::vector<OccQtCore::Feature::HoleWallCandidate> m_holeWallCandidates;
    std::vector<OccQtCore::Feature::HoleEndCandidate> m_holeEndCandidates;
    std::vector<OccQtCore::Feature::HoleSegmentCandidate> m_holeSegmentCandidates;

    OccQtCore::Debug::HoleDebugDisplayOptions m_holeDebugDisplayOptions;
    OccQtCore::Debug::HoleDebugLogOptions m_holeDebugLogOptions;
};
#endif // MAINWINDOW_H
