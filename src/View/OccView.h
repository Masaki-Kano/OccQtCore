#ifndef OCCVIEW_H
#define OCCVIEW_H

#include <QWidget>

#include <AIS_InteractiveContext.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>

namespace OccQtCore
{
    class OccView : public QWidget
    {
        Q_OBJECT

    public:
        explicit OccView(QWidget* parent = nullptr);

        void displayTestBox();

    protected:
        void resizeEvent(QResizeEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
        QPaintEngine* paintEngine() const override;

    private:
        void initializeOcc();

    private:
        bool m_initialized = false;

        Handle(Aspect_DisplayConnection) m_displayConnection;
        Handle(OpenGl_GraphicDriver) m_graphicDriver;
        Handle(V3d_Viewer) m_viewer;
        Handle(V3d_View) m_view;
        Handle(AIS_InteractiveContext) m_context;
    };
}

#endif // OCCVIEW_H
