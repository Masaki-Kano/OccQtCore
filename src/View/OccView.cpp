#include "OccView.h"

#include <AIS_Shape.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <WNT_Window.hxx>

#include <QResizeEvent>

namespace OccQtCore
{
    OccView::OccView(QWidget* parent)
        : QWidget(parent)
    {
        setAttribute(Qt::WA_NativeWindow);
        setAttribute(Qt::WA_PaintOnScreen);
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_OpaquePaintEvent);

        setMouseTracking(true);
    }

    QPaintEngine* OccView::paintEngine() const
    {
        return nullptr;
    }

    void OccView::resizeEvent(QResizeEvent* event)
    {
        QWidget::resizeEvent(event);

        if (!m_initialized)
        {
            initializeOcc();
        }

        if (!m_view.IsNull())
        {
            m_view->MustBeResized();
            m_view->Redraw();
        }
    }

    void OccView::paintEvent(QPaintEvent* event)
    {
        Q_UNUSED(event);

        if (!m_initialized)
        {
            initializeOcc();
        }

        if (!m_view.IsNull())
        {
            m_view->Redraw();
        }
    }

    void OccView::initializeOcc()
    {
        if (m_initialized)
        {
            return;
        }

        m_displayConnection = new Aspect_DisplayConnection();
        m_graphicDriver = new OpenGl_GraphicDriver(m_displayConnection);

        m_viewer = new V3d_Viewer(m_graphicDriver);
        m_viewer->SetDefaultLights();
        m_viewer->SetLightOn();

        m_context = new AIS_InteractiveContext(m_viewer);

        m_view = m_viewer->CreateView();

        Handle(WNT_Window) window = new WNT_Window(reinterpret_cast<Aspect_Handle>(winId()));
        m_view->SetWindow(window);

        if (!window->IsMapped())
        {
            window->Map();
        }

        m_view->SetBackgroundColor(Quantity_NOC_BLACK);
        m_view->TriedronDisplay(
            Aspect_TOTP_LEFT_LOWER,
            Quantity_NOC_WHITE,
            0.08,
            V3d_ZBUFFER
        );

        m_view->MustBeResized();

        m_initialized = true;
    }

    void OccView::displayTestBox()
    {
        if (!m_initialized)
        {
            initializeOcc();
        }

        if (m_context.IsNull())
        {
            return;
        }

        TopoDS_Shape box = BRepPrimAPI_MakeBox(100.0, 80.0, 60.0).Shape();
        Handle(AIS_Shape) aisShape = new AIS_Shape(box);

        m_context->Display(aisShape, Standard_False);
        m_context->SetDisplayMode(aisShape, AIS_Shaded, Standard_False);
        m_context->UpdateCurrentViewer();

        if (!m_view.IsNull())
        {
            m_view->MustBeResized();
            m_view->FitAll();
            m_view->ZFitAll();
            m_view->Redraw();
        }
    }
}
