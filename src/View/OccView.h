#ifndef OCCVIEW_H
#define OCCVIEW_H

#include <QWidget>

namespace OccQtCore
{
    class OccView : public QWidget
    {
        Q_OBJECT

    public:
        explicit OccView(QWidget* parent = nullptr);
    };
}

#endif // OCCVIEW_H
