#ifndef OCCVIEW_H
#define OCCVIEW_H

#include <memory>
#include <vector>

#include <QPoint>
#include <QWidget>

#include <AIS_DisplayMode.hxx>
#include <AIS_InteractiveContext.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <Quantity_Color.hxx>
#include <TopoDS_Shape.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>

#include "Core/PickResult.h"
#include "View/AisDisplayManager.h"
#include "View/DisplayObjectRegistry.h"
#include "View/DisplayStyle.h"
#include "View/DisplayTypes.h"
#include "View/OccPickController.h"

class QMouseEvent;
class QWheelEvent;

namespace OccQtCore
{
    class GeometryModel;

    class OccView : public QWidget
    {
        Q_OBJECT

    public:
        explicit OccView(QWidget* parent = nullptr);
        ~OccView() override = default;

        QPaintEngine* paintEngine() const override;

        DisplayObjectId displayShape(
            const TopoDS_Shape& shape,
            DisplayLayer layer,
            const DisplayStyle& style,
            DisplayObjectSourceKind sourceKind = DisplayObjectSourceKind::Unknown,
            int sourceElementIndex = -1);

        std::vector<DisplayObjectId> displayShapes(
            const std::vector<TopoDS_Shape>& shapes,
            DisplayLayer layer,
            const DisplayStyle& style,
            DisplayObjectSourceKind sourceKind = DisplayObjectSourceKind::Unknown,
            int sourceElementIndex = -1);

        const DisplayObjectRegistry& displayObjectRegistry() const;

        void removeObject(DisplayObjectId id);

        void clearLayer(DisplayLayer layer);
        void clearLayers(const std::vector<DisplayLayer>& layers);
        void clearAll();

        void fitAll();
        void viewX();
        void viewY();
        void viewZ();
        void viewIso();

        void setShadedMode();
        void setWireframeMode();
        void setShapeColor(const Quantity_Color& color);

        void updateViewer();
        void redraw();

    signals:
        void shapePicked(const OccQtCore::PickResult& result);

    protected:
        void showEvent(QShowEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void wheelEvent(QWheelEvent* event) override;

    private:
        void initializeOcc();
        bool isInitialized() const;

        void beginRotate(const QPoint& pos);
        void beginPan(const QPoint& pos);

        void updateRotate(const QPoint& pos);
        void updatePan(const QPoint& pos);

        void endMouseOperation();
        void zoomView(double factor);

        void setShapeDisplayMode(AIS_DisplayMode displayMode);

        bool isClickOperation(const QPoint& releasePos) const;
        void pickAt(const QPoint& pos);

    private:
        enum class MouseMode
        {
            None,
            Rotate,
            Pan
        };

        struct MouseState
        {
            MouseMode mode = MouseMode::None;
            QPoint pressPos;
            QPoint lastPos;
        };

    private:
        Handle(Aspect_DisplayConnection) m_displayConnection;
        Handle(OpenGl_GraphicDriver) m_graphicDriver;
        Handle(V3d_Viewer) m_viewer;
        Handle(V3d_View) m_view;
        Handle(AIS_InteractiveContext) m_context;

        std::unique_ptr<AisDisplayManager> m_aisDisplayManager;
        std::unique_ptr<OccPickController> m_pickController;

        MouseState m_mouseState;

        DisplayStyle m_shapeStyle =
            DisplayStyle::preset(DisplayStyle::Preset::DefaultShape);

        bool m_initialized = false;
    };
}



#endif // OCCVIEW_H
