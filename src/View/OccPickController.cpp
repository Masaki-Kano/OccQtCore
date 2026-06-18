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

        m_context->SelectDetected(
            AIS_SelectionScheme_Replace);

        m_context->InitSelected();

        if (!m_context->MoreSelected())
        {
            clearPickState();
            return result;
        }

        const TopoDS_Shape pickedShape =
            m_context->SelectedShape();

        clearPickState();

        if (pickedShape.IsNull())
        {
            return result;
        }

        result.hasShape = true;
        result.shape = pickedShape;
        result.type =
            toPickedShapeType(
                pickedShape.ShapeType());

        result.sourceDisplayObjectId =
            m_displayManager != nullptr
                ? m_displayManager->findDisplayObjectId(pickedObject)
                : -1;

        return result;
    }

    PickedShapeType OccPickController::toPickedShapeType(
        TopAbs_ShapeEnum shapeType) const
    {
        switch (shapeType)
        {
        case TopAbs_VERTEX:
            return PickedShapeType::Vertex;

        case TopAbs_EDGE:
            return PickedShapeType::Edge;

        case TopAbs_FACE:
            return PickedShapeType::Face;

        case TopAbs_WIRE:
            return PickedShapeType::Wire;

        case TopAbs_SHELL:
            return PickedShapeType::Shell;

        case TopAbs_SOLID:
            return PickedShapeType::Solid;

        case TopAbs_COMPOUND:
            return PickedShapeType::Compound;

        default:
            return PickedShapeType::Unknown;
        }
    }

}
