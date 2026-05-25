#include "OccView.h"

#include <algorithm>

#include <QPaintEngine>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QMouseEvent>
#include <QWheelEvent>

#include <AIS_Shape.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <Graphic3d_GraphicDriver.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <Quantity_Color.hxx>
#include <V3d_View.hxx>
#include <V3d_TypeOfOrientation.hxx>
#include <WNT_Window.hxx>

namespace OccQtCore
{

    namespace
    {
        constexpr double ZoomStepFactor = 1.1;
    }

    OccView::OccView(QWidget* parent)
        : QWidget(parent)
    {
        setAttribute(Qt::WA_NativeWindow);
        setAttribute(Qt::WA_PaintOnScreen);
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_OpaquePaintEvent);

        setMouseTracking(true);
        setFocusPolicy(Qt::StrongFocus);
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

    void OccView::mousePressEvent(QMouseEvent* event)
    {
        if (!isInitialized())
        {
            QWidget::mousePressEvent(event);
            return;
        }

        const QPoint pos = event->position().toPoint();

        m_mouseState.pressPos = pos;
        m_mouseState.lastPos = pos;

        if (event->button() == Qt::RightButton)
        {
            beginRotate(pos);
            event->accept();
            return;
        }

        if (event->button() == Qt::MiddleButton)
        {
            beginPan(pos);
            event->accept();
            return;
        }

        QWidget::mousePressEvent(event);
    }

    void OccView::mouseMoveEvent(QMouseEvent* event)
    {
        if (!isInitialized())
        {
            QWidget::mouseMoveEvent(event);
            return;
        }

        const QPoint pos = event->position().toPoint();

        switch (m_mouseState.mode)
        {
        case MouseMode::Rotate:
            updateRotate(pos);
            event->accept();
            break;

        case MouseMode::Pan:
            updatePan(pos);
            event->accept();
            break;

        case MouseMode::None:
        default:
            QWidget::mouseMoveEvent(event);
            break;
        }

        m_mouseState.lastPos = pos;
    }

    void OccView::mouseReleaseEvent(QMouseEvent* event)
    {
        const QPoint pos = event->position().toPoint();

        if (event->button() == Qt::LeftButton)
        {
            if (isClickOperation(pos))
            {
                pickAt(pos);
            }

            event->accept();
            return;
        }

        if (event->button() == Qt::RightButton ||
            event->button() == Qt::MiddleButton)
        {
            endMouseOperation();
            event->accept();
            return;
        }

        QWidget::mouseReleaseEvent(event);
    }

    void OccView::wheelEvent(QWheelEvent* event)
    {
        if (!isInitialized())
        {
            QWidget::wheelEvent(event);
            return;
        }

        const int wheelDelta = event->angleDelta().y();

        if (wheelDelta == 0)
        {
            QWidget::wheelEvent(event);
            return;
        }

        const double zoomFactor =
            (wheelDelta > 0) ? ZoomStepFactor : (1.0 / ZoomStepFactor);

        zoomView(zoomFactor);
        event->accept();
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

    void OccView::beginRotate(const QPoint& pos)
    {
        m_mouseState.mode = MouseMode::Rotate;
        m_view->StartRotation(pos.x(), pos.y());
    }

    void OccView::beginPan(const QPoint& pos)
    {
        Q_UNUSED(pos);

        m_mouseState.mode = MouseMode::Pan;
    }

    void OccView::updateRotate(const QPoint& pos)
    {
        if (m_view.IsNull())
        {
            return;
        }

        m_view->Rotation(pos.x(), pos.y());
        m_view->Redraw();
    }

    void OccView::updatePan(const QPoint& pos)
    {
        if (m_view.IsNull())
        {
            return;
        }

        const QPoint delta = pos - m_mouseState.lastPos;

        m_view->Pan(delta.x(), -delta.y());
        m_view->Redraw();
    }

    void OccView::endMouseOperation()
    {
        m_mouseState.mode = MouseMode::None;
    }

    void OccView::zoomView(double factor)
    {
        if (m_view.IsNull())
        {
            return;
        }

        m_view->SetZoom(factor);
        m_view->Redraw();
    }

    void OccView::setShapeDisplayMode(AIS_DisplayMode displayMode)
    {
        if (!isInitialized())
        {
            return;
        }

        m_shapeStyle.displayMode = displayMode;

        for (const DisplayObject& displayObject : m_displayObjects)
        {
            if (displayObject.layer != DisplayLayer::Shape)
            {
                continue;
            }

            applyDisplayStyle(displayObject.object, m_shapeStyle);
        }

        m_context->UpdateCurrentViewer();
        redraw();
    }

    void OccView::applyDisplayStyle(
        const Handle(AIS_InteractiveObject)& object,
        const DisplayStyle& style)
    {
        if (!isInitialized())
        {
            return;
        }

        if (object.IsNull())
        {
            return;
        }

        m_context->SetDisplayMode(object, style.displayMode, Standard_False);
        m_context->SetColor(object, style.color, Standard_False);
        m_context->SetTransparency(object, style.transparency, Standard_False);
        m_context->Redisplay(object, Standard_False);
    }

    bool OccView::isClickOperation(const QPoint& releasePos) const
    {
        constexpr int ClickMoveThreshold = 3;

        const int moveDistance =
            (releasePos - m_mouseState.pressPos).manhattanLength();

        return moveDistance <= ClickMoveThreshold;
    }

    PickedShapeType OccView::toPickedShapeType(TopAbs_ShapeEnum shapeType)
    {
        switch (shapeType)
        {
        case TopAbs_VERTEX:
            return PickedShapeType::Vertex;

        case TopAbs_EDGE:
            return PickedShapeType::Edge;

        case TopAbs_FACE:
            return PickedShapeType::Face;

        case TopAbs_SOLID:
            return PickedShapeType::Solid;

        default:
            return PickedShapeType::Unknown;
        }
    }

    DisplayObjectId OccView::findDisplayObjectId(
        const Handle(AIS_InteractiveObject)& object) const
    {
        if (object.IsNull())
        {
            return -1;
        }

        for (const DisplayObject& displayObject : m_displayObjects)
        {
            if (displayObject.object == object)
            {
                return displayObject.id;
            }
        }

        return -1;
    }

    void OccView::pickAt(const QPoint& pos)
    {
        if (!isInitialized())
        {
            return;
        }

        PickResult result;

        m_context->MoveTo(pos.x(), pos.y(), m_view, Standard_True);

        if (!m_context->HasDetected())
        {
            emit shapePicked(result);
            return;
        }

        const Handle(AIS_InteractiveObject) pickedObject =
            m_context->DetectedInteractive();

        m_context->SelectDetected(AIS_SelectionScheme_Replace);
        m_context->InitSelected();

        if (!m_context->MoreSelected())
        {
            emit shapePicked(result);
            return;
        }

        const TopoDS_Shape pickedShape = m_context->SelectedShape();

        if (pickedShape.IsNull())
        {
            emit shapePicked(result);
            return;
        }

        result.hasShape = true;
        result.shape = pickedShape;
        result.type = toPickedShapeType(pickedShape.ShapeType());
        result.sourceDisplayObjectId = findDisplayObjectId(pickedObject);

        emit shapePicked(result);
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

        if (layer == DisplayLayer::Shape)
        {
            applyDisplayStyle(object, m_shapeStyle);
        }

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

        const DisplayObjectId id = displayObject(aisShape, layer);

        if (layer == DisplayLayer::Shape && isInitialized())
        {
            m_context->Activate(aisShape, 0);
            m_context->Activate(aisShape, AIS_Shape::SelectionMode(TopAbs_FACE));
            m_context->Activate(aisShape, AIS_Shape::SelectionMode(TopAbs_EDGE));
            m_context->Activate(aisShape, AIS_Shape::SelectionMode(TopAbs_VERTEX));

            m_context->UpdateCurrentViewer();
        }

        return id;
    }

    DisplayObjectId OccView::displayShape(
        const TopoDS_Shape& shape,
        DisplayLayer layer,
        const DisplayStyle& style)
    {
        Handle(AIS_Shape) aisShape = new AIS_Shape(shape);

        const DisplayObjectId id = displayObject(aisShape, layer);

        if (id < 0 || !isInitialized())
        {
            return id;
        }

        applyDisplayStyle(aisShape, style);

        m_context->UpdateCurrentViewer();
        redraw();

        return id;
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

    void OccView::viewX()
    {
        if (m_view.IsNull())
        {
            return;
        }

        m_view->SetProj(V3d_Xpos);
        fitAll();
    }

    void OccView::viewY()
    {
        if (m_view.IsNull())
        {
            return;
        }

        m_view->SetProj(V3d_Ypos);
        fitAll();
    }

    void OccView::viewZ()
    {
        if (m_view.IsNull())
        {
            return;
        }

        m_view->SetProj(V3d_Zpos);
        fitAll();
    }

    void OccView::viewIso()
    {
        if (m_view.IsNull())
        {
            return;
        }

        m_view->SetProj(V3d_XposYnegZpos);
        fitAll();
    }

    void OccView::setShadedMode()
    {
        setShapeDisplayMode(AIS_Shaded);
    }

    void OccView::setWireframeMode()
    {
       setShapeDisplayMode(AIS_WireFrame);
    }

    void OccView::setShapeColor(const Quantity_Color& color)
    {
        m_shapeStyle.color = color;

        if (!isInitialized())
        {
            return;
        }

        for (const DisplayObject& displayObject : m_displayObjects)
        {
            if (displayObject.layer != DisplayLayer::Shape)
            {
                continue;
            }

            applyDisplayStyle(displayObject.object, m_shapeStyle);
        }

        m_context->UpdateCurrentViewer();
        redraw();
    }

    void OccView::redraw()
    {
        if (m_view.IsNull())
        {
            return;
        }

        m_view->Redraw();
    }
}
