#ifndef AISDISPLAYMANAGER_H
#define AISDISPLAYMANAGER_H

#include <vector>

#include <AIS_InteractiveContext.hxx>
#include <AIS_InteractiveObject.hxx>
#include <TopoDS_Shape.hxx>
#include <V3d_View.hxx>

#include "View/DisplayObjectRegistry.h"
#include "View/DisplayStyle.h"

namespace OccQtCore
{
    class AisDisplayManager
    {
    public:
        explicit AisDisplayManager(
            const Handle(AIS_InteractiveContext)& context);

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

        void removeObject(
            DisplayObjectId id);

        void clearLayer(
            DisplayLayer layer);

        void clearLayers(
            const std::vector<DisplayLayer>& layers);

        void clearAll();

        void applyStyleToLayer(
            DisplayLayer layer,
            const DisplayStyle& style);

        DisplayObjectId findDisplayObjectId(
            const Handle(AIS_InteractiveObject)& object) const;

        const DisplayObjectRegistry& registry() const;

    private:
        struct DisplayObjectHandle
        {
            DisplayObjectId id = -1;
            Handle(AIS_InteractiveObject) object;
        };

    private:
        DisplayObjectId displayObject(
            const Handle(AIS_InteractiveObject)& object,
            DisplayLayer layer,
            const DisplayStyle& style,
            DisplayObjectSourceKind sourceKind,
            int sourceElementIndex);

        const Handle(AIS_InteractiveObject)* findHandle(
            DisplayObjectId id) const;

        void removeHandle(
            DisplayObjectId id);

        void applyDisplayStyle(
            const Handle(AIS_InteractiveObject)& object,
            const DisplayStyle& style);

    private:
        Handle(AIS_InteractiveContext) m_context;

        DisplayObjectRegistry m_registry;
        std::vector<DisplayObjectHandle> m_handles;
    };
}

#endif // AISDISPLAYMANAGER_H
