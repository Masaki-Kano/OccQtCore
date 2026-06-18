#include "OccView.h"

#include <QPaintEngine>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QMouseEvent>
#include <QWheelEvent>

#include <Aspect_DisplayConnection.hxx>
#include <Graphic3d_GraphicDriver.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <Quantity_Color.hxx>
#include <V3d_View.hxx>
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

        m_aisDisplayManager =
            std::make_unique<AisDisplayManager>(m_context);

        m_pickController =
            std::make_unique<OccPickController>(
                m_context,
                m_view,
                m_aisDisplayManager.get());

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
        redraw();
    }

    void OccView::updatePan(const QPoint& pos)
    {
        if (m_view.IsNull())
        {
            return;
        }

        const QPoint delta = pos - m_mouseState.lastPos;

        m_view->Pan(delta.x(), -delta.y());
        redraw();
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
        redraw();
    }

    void OccView::setShapeDisplayMode(AIS_DisplayMode displayMode)
    {
        m_shapeStyle.displayMode = displayMode;

        if (!isInitialized() ||
            !m_aisDisplayManager)
        {
            return;
        }

        m_aisDisplayManager->applyStyleToLayer(
            DisplayLayer::Model,
            m_shapeStyle);

        updateViewer();
    }

    bool OccView::isClickOperation(const QPoint& releasePos) const
    {
        constexpr int ClickMoveThreshold = 3;

        const int moveDistance =
            (releasePos - m_mouseState.pressPos).manhattanLength();

        return moveDistance <= ClickMoveThreshold;
    }

    void OccView::pickAt(const QPoint& pos)
    {
        if (!isInitialized() ||
            !m_pickController)
        {
            return;
        }

        const PickResult result =
            m_pickController->pickAt(pos);

        updateViewer();

        emit shapePicked(result);
    }

    const DisplayObjectRegistry& OccView::displayObjectRegistry() const
    {
        static const DisplayObjectRegistry EmptyRegistry;

        if (!m_aisDisplayManager)
        {
            return EmptyRegistry;
        }

        return m_aisDisplayManager->registry();
    }

    DisplayObjectId OccView::displayShape(
        const TopoDS_Shape& shape,
        DisplayLayer layer,
        const DisplayStyle& style,
        DisplayObjectSourceKind sourceKind,
        int sourceElementIndex)
    {
        if (!isInitialized())
        {
            initializeOcc();
        }

        if (!isInitialized() ||
            !m_aisDisplayManager)
        {
            return -1;
        }

        const DisplayObjectId id =
            m_aisDisplayManager->displayShape(
                shape,
                layer,
                style,
                sourceKind,
                sourceElementIndex);

        if (id >= 0)
        {
            updateViewer();
        }

        return id;
    }

    std::vector<DisplayObjectId> OccView::displayShapes(
        const std::vector<TopoDS_Shape>& shapes,
        DisplayLayer layer,
        const DisplayStyle& style,
        DisplayObjectSourceKind sourceKind,
        int sourceElementIndex)
    {
        if (!isInitialized())
        {
            initializeOcc();
        }

        if (!isInitialized() ||
            !m_aisDisplayManager)
        {
            return {};
        }

        const auto ids =
            m_aisDisplayManager->displayShapes(
                shapes,
                layer,
                style,
                sourceKind,
                sourceElementIndex);

        if (!ids.empty())
        {
            updateViewer();
        }

        return ids;
    }

    void OccView::removeObject(DisplayObjectId id)
    {
        if (!isInitialized() ||
            !m_aisDisplayManager)
        {
            return;
        }

        m_aisDisplayManager->removeObject(id);
        updateViewer();
    }

    void OccView::clearLayer(DisplayLayer layer)
    {
        if (!isInitialized() ||
            !m_aisDisplayManager)
        {
            return;
        }

        m_aisDisplayManager->clearLayer(layer);
        updateViewer();
    }

    void OccView::clearLayers(
        const std::vector<DisplayLayer>& layers)
    {
        if (!isInitialized() ||
            !m_aisDisplayManager)
        {
            return;
        }

        m_aisDisplayManager->clearLayers(layers);
        updateViewer();
    }

    void OccView::clearAll()
    {
        if (!m_aisDisplayManager)
        {
            return;
        }

        m_aisDisplayManager->clearAll();
        updateViewer();
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

        if (!isInitialized() ||
            !m_aisDisplayManager)
        {
            return;
        }

        m_aisDisplayManager->applyStyleToLayer(
            DisplayLayer::Model,
            m_shapeStyle);

        updateViewer();
    }

    void OccView::updateViewer()
    {
        if (!m_context.IsNull())
        {
            m_context->UpdateCurrentViewer();
        }

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
