#ifndef HOLEDEBUGPANEL_H
#define HOLEDEBUGPANEL_H

#include <QWidget>
#include <QString>

#include "Debug/HoleDebugOptions.h"
#include "Feature/HoleRecognitionTypes.h"

namespace Ui {
class HoleDebugPanel;
}

namespace OccQtCore::Feature {
struct HoleRecognitionResult;
}

class QTreeWidgetItem;

class HoleDebugPanel : public QWidget
{
    Q_OBJECT

public:
    explicit HoleDebugPanel(QWidget *parent = nullptr);
    ~HoleDebugPanel();

    OccQtCore::Debug::HoleDebugDisplayOptions displayOptions() const;
    void setDisplayOptions(const OccQtCore::Debug::HoleDebugDisplayOptions& options);

    void setStatusText(const QString& text);
    void setRecognitionResult(const OccQtCore::Feature::HoleRecognitionResult& result);

signals:
    void buildRequested();
    void refreshDisplayRequested();
    void clearDisplayRequested();
    void exportDetailLogRequested();

    void displayOptionsChanged(const OccQtCore::Debug::HoleDebugDisplayOptions& options);

    void selectedCandidateChanged(const OccQtCore::Debug::HoleDebugSelectedCandidate& selected);

private:
    void setupInitialState();
    void setupConnections();
    void emitDisplayOptionsChanged();
    void populateCandidateTree(const OccQtCore::Feature::HoleRecognitionResult& result);

    void updateSelectionDetail(QTreeWidgetItem* item);
    OccQtCore::Debug::HoleDebugSelectedCandidate selectedCandidateFromItem(QTreeWidgetItem* item) const;

    QString buildHoleDetailText(int index) const;
    QString buildSegmentDetailText(int index) const;
    QString buildWallDetailText(int index) const;
    QString buildEndDetailText(int index) const;

private:
    Ui::HoleDebugPanel *ui;
    OccQtCore::Feature::HoleRecognitionResult m_result;
    bool m_hasRexult = false;
};

#endif // HOLEDEBUGPANEL_H
