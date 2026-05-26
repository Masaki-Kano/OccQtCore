#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>

#include <QLabel>
#include <QMainWindow>

#include "Core/DocumentData.h"
#include "Core/SelectionInfo.h"

// 一時デバック用のインクルード
#include "Feature/HoleFeatureRecognizer.h"

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

    // 穴関連一時デバック関数群
    void analyzeHoleEnds();
    void showHoleEndCandidates(
        const std::vector<OccQtCore::Feature::HoleEndCandidate>& candidates);
    void showHoleEndComponents(
        const std::vector<OccQtCore::Feature::HoleEndComponent>& components);

private:
    Ui::MainWindow* ui = nullptr;

    OccQtCore::AppLogger* m_logger = nullptr;
    std::unique_ptr<OccQtCore::AppLogReporter> m_logReporter;
    OccQtCore::LogPanel* m_logPanel = nullptr;
    OccQtCore::OccView* m_occView = nullptr;

    OccQtCore::DocumentData m_document;
    OccQtCore::SelectionInfo m_selectionInfo;
    QString m_lastOpenDirectory;
};
#endif // MAINWINDOW_H
