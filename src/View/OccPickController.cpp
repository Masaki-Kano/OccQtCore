#include "View/OccPickController.h"

namespace OccQtCore
{
    OccPickController::OccPickController(
        const Handle(AIS_InteractiveContext)& context,
        const Handle(V3d_View)& view,
        const AisDisplayManager* displayManager)
        : m_context(context),
        m_view(view),
        m_displayManager(displayManager)
    {
    }

    PickResult OccPickController::pickAt(
        const QPoint& pos) const
    {
        PickResult result;

        m_lastPickedOwner.Nullify();

        if (m_context.IsNull() ||
            m_view.IsNull())
        {
            return result;
        }

        auto clearPickState =
            [this]()
        {
            m_context->ClearSelected(Standard_False);
            m_context->ClearDetected(Standard_False);
        };

        clearPickState();

        m_context->MoveTo(
            pos.x(),
            pos.y(),
            m_view,
            Standard_False);

        if (!m_context->HasDetected())
        {
            clearPickState();
            return result;
        }

        const Handle(AIS_InteractiveObject) pickedObject =
            m_context->DetectedInteractive();

        m_lastPickedOwner =
            m_context->DetectedOwner();

        if (m_lastPickedOwner.IsNull())
        {
            clearPickState();
            return result;
        }

        m_context->SelectDetected(
            AIS_SelectionScheme_Replace);

        m_context->InitSelected();

        if (!m_context->MoreSelected())
        {
            m_lastPickedOwner.Nullify();
            clearPickState();
            return result;
        }

        const TopoDS_Shape pickedShape =
            m_context->SelectedShape();

        // ピック総督は選択表示を残さない。
        clearPickState();

        if (pickedShape.IsNull())
        {
            m_lastPickedOwner.Nullify();
            return result;
        }

        result.hasShape = true;
        result.shape = pickedShape;
        result.elementKind =
            toGeometryElementKind(
                pickedShape.ShapeType());

        result.sourceDisplayObjectId =
            m_displayManager != nullptr
                ? m_displayManager->findDisplayObjectId(pickedObject)
                : -1;

        return result;
    }

    const Handle(SelectMgr_EntityOwner)&
        OccPickController::lastPickedOwner() const
    {
        return m_lastPickedOwner;
    }

    GeometryElementKind OccPickController::toGeometryElementKind(
        TopAbs_ShapeEnum shapeType) const
    {
        switch (shapeType)
        {
        case TopAbs_VERTEX:
            return GeometryElementKind::Vertex;

        case TopAbs_EDGE:
            return GeometryElementKind::Edge;

        case TopAbs_FACE:
            return GeometryElementKind::Face;

        case TopAbs_WIRE:
            return GeometryElementKind::Wire;

        case TopAbs_SHELL:
            return GeometryElementKind::Shell;

        case TopAbs_SOLID:
            return GeometryElementKind::Solid;

        case TopAbs_COMPOUND:
            return GeometryElementKind::Compound;

        default:
            return GeometryElementKind::Unknown;
        }
    }

}
