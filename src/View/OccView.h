#ifndef OCCVIEW_H
#define OCCVIEW_H

#include <vector>

#include <QWidget>

#include <AIS_InteractiveContext.hxx>
#include <AIS_InteractiveObject.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <TopoDS_Shape.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>

namespace OccQtCore
{
    // 外部APIでも使う表示オブジェクトID
    using DisplayObjectId = int;

    // 外部から表示レイヤを指定できるように公開
    enum class DisplayLayer
    {
        Shape,      // 通常形状
        Overlay,    // パス・法線・補助線など
        Highlight,  // ハイライト表示
        Temporary   // 一時表示
    };

    class OccView : public QWidget
    {
        Q_OBJECT

    public:
        explicit OccView(QWidget* parent = nullptr);
        ~OccView() override = default;

        QPaintEngine* paintEngine() const override;

        // 任意のAISオブジェクトを表示
        DisplayObjectId displayObject(
            const Handle(AIS_InteractiveObject)& object,
            DisplayLayer layer = DisplayLayer::Shape);

        // TopoDS_Shape表示用の便利API
        DisplayObjectId displayShape(
            const TopoDS_Shape& shape,
            DisplayLayer layer = DisplayLayer::Shape);

        void removeObject(DisplayObjectId id);
        void clearLayer(DisplayLayer layer);
        void clearAll();

        void fitAll();
        void redraw();

        // 動作確認用
        void displayTestBox();

    protected:
        void showEvent(QShowEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        void initializeOcc();
        bool isInitialized() const;

    private:
        // OccView内部だけで使う表示管理情報
        struct DisplayObject
        {
            DisplayObjectId id = -1;
            DisplayLayer layer = DisplayLayer::Shape;
            Handle(AIS_InteractiveObject) object;
        };

    private:
        Handle(Aspect_DisplayConnection) m_displayConnection;
        Handle(OpenGl_GraphicDriver) m_graphicDriver;
        Handle(V3d_Viewer) m_viewer;
        Handle(V3d_View) m_view;
        Handle(AIS_InteractiveContext) m_context;

        std::vector<DisplayObject> m_displayObjects;
        DisplayObjectId m_nextDisplayObjectId = 1;

        bool m_initialized = false;
    };
}



#endif // OCCVIEW_H
