#include "OccView.h"

#include <algorithm>

#include <QPaintEngine>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QShowEvent>

#include <AIS_Shape.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <Graphic3d_GraphicDriver.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <Quantity_Color.hxx>
#include <V3d_View.hxx>
#include <WNT_Window.hxx>

namespace OccQtCore
{
    OccView::OccView(QWidget* parent)
        : QWidget(parent)
    {
        setAttribute(Qt::WA_NativeWindow);
        setAttribute(Qt::WA_PaintOnScreen);
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_OpaquePaintEvent);
    }

    QPaintEngine* OccView::paintEngine() const
    {
        return nullptr;
    }

    void OccView::showEvent(QShowEvent* event)
    {
        QWidget::showEvent(event);

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

    void OccView::resizeEvent(QResizeEvent* event)
    {
        QWidget::resizeEvent(event);

        if (!m_view.IsNull())
        {
            m_view->MustBeResized();
            m_view->Redraw();
        }
    }

    void OccView::paintEvent(QPaintEvent* event)
    {
        Q_UNUSED(event);

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
            V3d_ZBUFFER);

        m_view->MustBeResized();
        m_view->Redraw();

        m_initialized = true;
    }

    bool OccView::isInitialized() const
    {
        return m_initialized
               && !m_context.IsNull()
               && !m_view.IsNull();
    }

    DisplayObjectId OccView::displayObject(
        const Handle(AIS_InteractiveObject)& object,
        DisplayLayer layer)
    {
        if (object.IsNull())
        {
            return -1;
        }

        if (!isInitialized())
        {
            initializeOcc();
        }

        if (!isInitialized())
        {
            return -1;
        }

        const DisplayObjectId id = m_nextDisplayObjectId++;

        DisplayObject displayObject;
        displayObject.id = id;
        displayObject.layer = layer;
        displayObject.object = object;

        m_displayObjects.push_back(displayObject);

        m_context->Display(object, Standard_False);
        m_context->UpdateCurrentViewer();

        redraw();

        return id;
    }

    DisplayObjectId OccView::displayShape(
        const TopoDS_Shape& shape,
        DisplayLayer layer)
    {
        if (shape.IsNull())
        {
            return -1;
        }

        Handle(AIS_Shape) aisShape = new AIS_Shape(shape);
        return displayObject(aisShape, layer);
    }

    void OccView::removeObject(DisplayObjectId id)
    {
        if (!isInitialized())
        {
            return;
        }

        auto it = std::find_if(
            m_displayObjects.begin(),
            m_displayObjects.end(),
            [id](const DisplayObject& displayObject)
            {
                return displayObject.id == id;
            });

        if (it == m_displayObjects.end())
        {
            return;
        }

        if (!it->object.IsNull())
        {
            m_context->Remove(it->object, Standard_False);
        }

        m_displayObjects.erase(it);

        m_context->UpdateCurrentViewer();
        redraw();
    }

    void OccView::clearLayer(DisplayLayer layer)
    {
        if (!isInitialized())
        {
            return;
        }

        auto it = m_displayObjects.begin();

        while (it != m_displayObjects.end())
        {
            if (it->layer == layer)
            {
                if (!it->object.IsNull())
                {
                    m_context->Remove(it->object, Standard_False);
                }

                it = m_displayObjects.erase(it);
            }
            else
            {
                ++it;
            }
        }

        m_context->UpdateCurrentViewer();
        redraw();
    }

    void OccView::clearAll()
    {
        if (!isInitialized())
        {
            m_displayObjects.clear();
            return;
        }

        for (const DisplayObject& displayObject : m_displayObjects)
        {
            if (!displayObject.object.IsNull())
            {
                m_context->Remove(displayObject.object, Standard_False);
            }
        }

        m_displayObjects.clear();

        m_context->UpdateCurrentViewer();
        redraw();
    }

    void OccView::fitAll()
    {
        if (m_view.IsNull())
        {
            return;
        }

        m_view->MustBeResized();
        m_view->FitAll();
        m_view->ZFitAll();
        m_view->Redraw();
    }

    void OccView::redraw()
    {
        if (m_view.IsNull())
        {
            return;
        }

        m_view->Redraw();
    }

    void OccView::displayTestBox()
    {
        TopoDS_Shape box = BRepPrimAPI_MakeBox(100.0, 80.0, 60.0).Shape();

        displayShape(box, DisplayLayer::Shape);

        fitAll();
    }
}
