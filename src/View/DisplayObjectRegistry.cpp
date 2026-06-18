#include "View/DisplayObjectRegistry.h"

#include "Core/CollectionUtil.h"

namespace OccQtCore
{
    DisplayObjectId DisplayObjectRegistry::registerObject(
        DisplayLayer layer,
        DisplayObjectSourceKind sourceKind,
        int sourceElementIndex)
    {
        DisplayObjectRecord record;
        record.id = m_nextId++;
        record.layer = layer;
        record.sourceKind = sourceKind;
        record.sourceElementIndex = sourceElementIndex;

        m_records.push_back(record);

        return record.id;
    }

    const DisplayObjectRecord* DisplayObjectRegistry::findById(
        DisplayObjectId id) const
    {
        return CollectionUtil::findPtr(
            m_records,
            [id](const DisplayObjectRecord& record)
            {
                return record.id == id;
            });
    }

    std::vector<DisplayObjectRecord> DisplayObjectRegistry::recordsInLayer(
        DisplayLayer layer) const
    {
        std::vector<DisplayObjectRecord> records;

        for (const auto& record : m_records)
        {
            if (record.layer != layer)
            {
                continue;
            }

            records.push_back(record);
        }

        return records;
    }

    std::vector<DisplayObjectRecord> DisplayObjectRegistry::takeRecordsInLayer(
        DisplayLayer layer)
    {
        return CollectionUtil::takeIf(
            m_records,
            [layer](const DisplayObjectRecord& record)
            {
                return record.layer == layer;
            });
    }

    bool DisplayObjectRegistry::contains(
        DisplayObjectId id) const
    {
        return findById(id) != nullptr;
    }

    void DisplayObjectRegistry::removeById(
        DisplayObjectId id)
    {
        CollectionUtil::removeIf(
            m_records,
            [id](const DisplayObjectRecord& record)
            {
                return record.id == id;
            });
    }

    void DisplayObjectRegistry::clear()
    {
        m_records.clear();
        m_nextId = -1;
    }

    const std::vector<DisplayObjectRecord>&
    DisplayObjectRegistry::records() const
    {
        return m_records;
    }


}
