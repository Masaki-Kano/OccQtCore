#include "View/OccCameraController.h"

#include <V3d_TypeOfOrientation.hxx>

namespace OccQtCore
{
    OccCameraController::OccCameraController(
        const Handle(V3d_View)& view)
        : m_view(view)
    {
    }

    void OccCameraController::beginRotate(const QPoint& pos)
    {
        if (m_view.IsNull())
        {
            return;
        }

        m_view->StartRotation(pos.x(), pos.y());
    }

    void OccCameraController::rotateTo(
        const QPoint& pos)
    {
        if (m_view.IsNull())
        {
            return;
        }

        m_view->Rotation(
            pos.x(),
            pos.y());
    }

    void OccCameraController::panBy(const QPoint& delta)
    {
        if (m_view.IsNull())
        {
            return;
        }

        m_view->Pan(delta.x(), -delta.y());
    }

    void OccCameraController::zoomBy(
        double factor)
    {
        if (m_view.IsNull())
        {
            return;
        }

        m_view->SetZoom(factor);
    }

    void OccCameraController::fitAll()
    {
        if (m_view.IsNull())
        {
            return;
        }

        m_view->MustBeResized();
        m_view->FitAll();
        m_view->ZFitAll();
    }

    void OccCameraController::viewX()
    {
        if (m_view.IsNull())
        {
            return;
        }

        m_view->SetProj(V3d_Xpos);
        fitAll();
    }

    void OccCameraController::viewY()
    {
        if (m_view.IsNull())
        {
            return;
        }

        m_view->SetProj(V3d_Ypos);
        fitAll();
    }

    void OccCameraController::viewZ()
    {
        if (m_view.IsNull())
        {
            return;
        }

        m_view->SetProj(V3d_Zpos);
        fitAll();
    }

    void OccCameraController::viewIso()
    {
        if (m_view.IsNull())
        {
            return;
        }

        m_view->SetProj(V3d_XposYnegZpos);
        fitAll();
    }
}
