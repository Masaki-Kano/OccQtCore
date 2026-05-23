#include "OccView.h"

#include <QLabel>
#include <QVBoxLayout>

namespace OccQtCore
{
    OccView::OccView(QWidget* parent)
        : QWidget(parent)
    {
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        auto* label = new QLabel("OccView", this);
        label->setAlignment(Qt::AlignCenter);

        layout->addWidget(label);
    }
}
