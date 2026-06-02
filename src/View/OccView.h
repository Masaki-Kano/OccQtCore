#ifndef OCCVIEW_H
#define OCCVIEW_H

#include <vector>

#include <QPoint>
#include <QWidget>

#include <AIS_InteractiveContext.hxx>
#include <AIS_InteractiveObject.hxx>
#include <AIS_Shape.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <TopoDS_Shape.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>
#include <Quantity_Color.hxx>
#include <AIS_DisplayMode.hxx>
#include <Quantity_NameOfColor.hxx>

#include "Core/PickResult.h"

class QMouseEvent;
class QWheelEvent;

namespace OccQtCore
{
    class GeometryModel;

    using DisplayObjectId = int;

    enum class DisplayLayer
    {
        Shape,          // 通常モデル
        PickHighlight,  // ピック選択
        Analysis,       // 穴候補、ポケット候補などの解析結果
        Helper,         // 法線・座標軸・補助線・パス
        Temporary       // 一時表示
    };

    struct DisplayStyle
    {
        Quantity_Color color = Quantity_Color(Quantity_NOC_WHITE);
        double transparency = 0.0;
        AIS_DisplayMode displayMode = AIS_Shaded;

        static DisplayStyle defaultShape();
        static DisplayStyle pickHighlightFace();
        static DisplayStyle analysisCandidateWire();
        static DisplayStyle analysisComponentEdge();
        static DisplayStyle analysisAdjacentFace();

        static DisplayStyle analysisHoleWallFace();

        static DisplayStyle analysisHoleOpenFace();
        static DisplayStyle analysisHoleOpenEdge();

        static DisplayStyle analysisHoleBottomFace();
        static DisplayStyle analysisHoleBottomEdge();

        static DisplayStyle analysisHoleConnectionFace();
        static DisplayStyle analysisHoleConnectionEdge();
    };

    class OccView : public QWidget
    {
        Q_OBJECT

    public:
        explicit OccView(QWidget* parent = nullptr);
        ~OccView() override = default;

        QPaintEngine* paintEngine() const override;

        DisplayObjectId displayObject(
            const Handle(AIS_InteractiveObject)& object,
            DisplayLayer layer = DisplayLayer::Shape);

        DisplayObjectId displayShape(
            const TopoDS_Shape& shape,
            DisplayLayer layer = DisplayLayer::Shape);

        DisplayObjectId displayShape(
            const TopoDS_Shape& shape,
            DisplayLayer layer,
            const DisplayStyle& style);

        std::vector<DisplayObjectId> displayShapes(
            const std::vector<TopoDS_Shape>& shapes,
            DisplayLayer layer,
            const DisplayStyle& style);

        void removeObject(DisplayObjectId id);
        void clearLayer(DisplayLayer layer);
        void clearAll();

        void fitAll();

        void viewX();
        void viewY();
        void viewZ();
        void viewIso();

        void setShadedMode();
        void setWireframeMode();
        void setShapeColor(const Quantity_Color& color);

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

        void applyDisplayStyle(
            const Handle(AIS_InteractiveObject)& object,
            const DisplayStyle& style);

        bool isClickOperation(const QPoint& releasePos) const;
        PickedShapeType toPickedShapeType(TopAbs_ShapeEnum shapeType);
        DisplayObjectId findDisplayObjectId(const Handle(AIS_InteractiveObject)& object) const;
        void pickAt(const QPoint& pos);

    private:
        // OccView内部だけで使う表示管理情報
        struct DisplayObject
        {
            DisplayObjectId id = -1;
            DisplayLayer layer = DisplayLayer::Shape;
            Handle(AIS_InteractiveObject) object;
        };

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

        std::vector<DisplayObject> m_displayObjects;
        DisplayObjectId m_nextDisplayObjectId = 1;

        MouseState m_mouseState;

        DisplayStyle m_shapeStyle{
            Quantity_Color(0.75, 0.78, 0.82, Quantity_TOC_RGB),
            0.0,
            AIS_Shaded
        };

        bool m_initialized = false;
    };
}



#endif // OCCVIEW_H
