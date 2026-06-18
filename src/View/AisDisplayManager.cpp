#include "View/AisDisplayManager.h"

#include <AIS_Shape.hxx>
#include <TopAbs_ShapeEnum.hxx>

namespace OccQtCore
{
    AisDisplayManager::AisDisplayManager(
        const Handle(AIS_InteractiveContext)& context)
        : m_context(context)
    {
    }

    DisplayObjectId AisDisplayManager::displayShape(
        const TopoDS_Shape& shape,
        DisplayLayer layer,
        const DisplayStyle& style,
        DisplayObjectSourceKind sourceKind,
        int sourceElementIndex)
    {
        if (shape.IsNull())
        {
            return -1;
        }

        Handle(AIS_Shape) aisShape = new AIS_Shape(shape);

        const DisplayObjectId id =
            displayObject(
                aisShape,
                layer,
                style,
                sourceKind,
                sourceElementIndex);

        if (id < 0 || m_context.IsNull())
        {
            return id;
        }

        // Display() 後のデフォルト選択を一旦切る。
        // Overlay類がピック対象になるのを防ぐ。
        m_context->Deactivate(aisShape);

        if (layer == DisplayLayer::Model)
        {
            m_context->Activate(
                aisShape,
                AIS_Shape::SelectionMode(TopAbs_FACE));

            m_context->Activate(
                aisShape,
                AIS_Shape::SelectionMode(TopAbs_EDGE));

            m_context->Activate(
                aisShape,
                AIS_Shape::SelectionMode(TopAbs_VERTEX));
        }

        return id;
    }

    std::vector<DisplayObjectId> AisDisplayManager::displayShapes(
        const std::vector<TopoDS_Shape>& shapes,
        DisplayLayer layer,
        const DisplayStyle& style,
        DisplayObjectSourceKind sourceKind,
        int sourceElementIndex)
    {
        std::vector<DisplayObjectId> ids;
        ids.reserve(shapes.size());

        for (const auto& shape : shapes)
        {
            if (shape.IsNull())
            {
                continue;
            }

            const DisplayObjectId id =
                displayShape(
                    shape,
                    layer,
                    style,
                    sourceKind,
                    sourceElementIndex);

            if (id >= 0)
            {
                ids.push_back(id);
            }
        }

        return ids;
    }

    DisplayObjectId AisDisplayManager::displayObject(
        const Handle(AIS_InteractiveObject)& object,
        DisplayLayer layer,
        const DisplayStyle& style,
        DisplayObjectSourceKind sourceKind,
        int sourceElementIndex)
    {
        if (m_context.IsNull())
        {
            return -1;
        }

        if (object.IsNull())
        {
            return -1;
        }

        const DisplayObjectId id =
            m_registry.registerObject(
                layer,
                sourceKind,
                sourceElementIndex);

        DisplayObjectHandle handle;
        handle.id = id;
        handle.object = object;

        m_handles.push_back(handle);

        m_context->Display(object, Standard_False);

        applyDisplayStyle(object, style);

        return id;
    }

    void AisDisplayManager::removeObject(DisplayObjectId id)
    {
        if (m_context.IsNull())
        {
            return;
        }

        if (!m_registry.contains(id))
        {
            return;
        }

        removeHandle(id);
        m_registry.removeById(id);
    }

    void AisDisplayManager::clearLayer(DisplayLayer layer)
    {
        if (m_context.IsNull())
        {
            return;
        }

        const auto records = m_registry.takeRecordsInLayer(layer);

        for (const auto& record : records)
        {
            removeHandle(record.id);
        }
    }

    void AisDisplayManager::clearLayers(
        const std::vector<DisplayLayer>& layers)
    {
        if (m_context.IsNull())
        {
            return;
        }

        for (const DisplayLayer layer : layers)
        {
            const auto records =
                m_registry.takeRecordsInLayer(layer);

            for (const auto& record : records)
            {
                removeHandle(record.id);
            }
        }
    }

    void AisDisplayManager::clearAll()
    {
        if (!m_context.IsNull())
        {
            for (const auto& handle : m_handles)
            {
                if (!handle.object.IsNull())
                {
                    m_context->Remove(
                        handle.object,
                        Standard_False);
                }
            }
        }

        m_handles.clear();
        m_registry.clear();
    }

    void AisDisplayManager::applyStyleToLayer(
        DisplayLayer layer, const DisplayStyle& style)
    {
        if (m_context.IsNull())
        {
            return;
        }

        const auto records = m_registry.recordsInLayer(layer);

        for (const auto& record : records)
        {
            const auto* object = findHandle(record.id);

            if (object == nullptr ||
                object->IsNull())
            {
                continue;
            }

            applyDisplayStyle(*object, style);
        }
    }

    DisplayObjectId AisDisplayManager::findDisplayObjectId(const Handle(AIS_InteractiveObject)& object) const
    {
        if (object.IsNull())
        {
            return -1;
        }

        for (const auto& handle : m_handles)
        {
            if (handle.object == object)
            {
                return handle.id;
            }
        }

        return -1;
    }

    const DisplayObjectRegistry& AisDisplayManager::registry() const
    {
        return m_registry;
    }

    const Handle(AIS_InteractiveObject)* AisDisplayManager::findHandle(
        DisplayObjectId id) const
    {
        for (const auto& handle : m_handles)
        {
            if (handle.id == id)
            {
                return &handle.object;
            }
        }

        return nullptr;
    }

    void AisDisplayManager::removeHandle(
        DisplayObjectId id)
    {
        auto it = m_handles.begin();

        while (it != m_handles.end())
        {
            if (it->id != id)
            {
                ++it;
                continue;
            }

            if (!it->object.IsNull() &&
                !m_context.IsNull())
            {
                m_context->Remove(
                    it->object,
                    Standard_False);
            }

            it = m_handles.erase(it);
        }
    }

    void AisDisplayManager::applyDisplayStyle(
        const Handle(AIS_InteractiveObject)& object,
        const DisplayStyle& style)
    {
        if (m_context.IsNull())
        {
            return;
        }

        if (object.IsNull())
        {
            return;
        }

        m_context->SetDisplayMode(
            object,
            style.displayMode,
            Standard_False);

        m_context->SetColor(
            object,
            style.color,
            Standard_False);

        m_context->SetTransparency(
            object,
            style.transparency,
            Standard_False);

        m_context->SetWidth(
            object,
            style.lineWidth,
            Standard_False);

        m_context->Redisplay(
            object,
            Standard_False);
    }
}
