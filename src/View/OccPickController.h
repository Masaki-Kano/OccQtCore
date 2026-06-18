#ifndef OCCPICKCONTROLLER_H
#define OCCPICKCONTROLLER_H

#include <QPoint>

#include <AIS_InteractiveContext.hxx>
#include <AIS_InteractiveObject.hxx>
#include <TopoDS_Shape.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <V3d_View.hxx>

#include "Core/PickResult.h"
#include "View/AisDisplayManager.h"

namespace OccQtCore
{
    class OccPickController
    {
    public:
        OccPickController(
            const Handle(AIS_InteractiveContext)& context,
            const Handle(V3d_View)& view,
            const AisDisplayManager* displayManager);

        PickResult pickAt(
            const QPoint& pos) const;

    private:
        PickedShapeType toPickedShapeType(
            TopAbs_ShapeEnum shapeType) const;

    private:
        Handle(AIS_InteractiveContext) m_context;
        Handle(V3d_View) m_view;

        // 所有しない。OccView が AisDisplayManager を所有する。
        // 検出された AIS object から DisplayObjectId を引くためだけに使う。
        const AisDisplayManager* m_displayManager = nullptr;
    };
}

#endif // OCCPICKCONTROLLER_H
