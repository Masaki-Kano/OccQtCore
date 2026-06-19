#ifndef OCCCAMERACONTROLLER_H
#define OCCCAMERACONTROLLER_H

#include <QPoint>

#include <V3d_View.hxx>

namespace OccQtCore
{
    class OccCameraController
    {
    public:
        explicit OccCameraController(const Handle(V3d_View)& view);

        void beginRotate(const QPoint& pos);

        void rotateTo(const QPoint& pos);

        void panBy(const QPoint& pos);

        void zoomBy(double factor);

        void fitAll();

        void viewX();
        void viewY();
        void viewZ();
        void viewIso();

    private:
        Handle(V3d_View) m_view;
    };
}

#endif // OCCCAMERACONTROLLER_H
