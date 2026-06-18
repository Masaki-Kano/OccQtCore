#ifndef DISPLAYOBJECTREGISTRY_H
#define DISPLAYOBJECTREGISTRY_H

#include <vector>

#include "View/DisplayTypes.h"

namespace OccQtCore
{
    struct DisplayObjectRecord
    {
        DisplayObjectId id = -1;

        DisplayLayer layer = DisplayLayer::Model;

        DisplayObjectSourceKind sourceKind =
            DisplayObjectSourceKind::Unknown;

        int sourceElementIndex = -1;
    };

    class DisplayObjectRegistry
    {
    public:
        DisplayObjectRegistry() = default;

        DisplayObjectId registerObject(
            DisplayLayer layer,
            DisplayObjectSourceKind sourceKind = DisplayObjectSourceKind::Unknown,
            int sourceElementIndex = -1);

        const DisplayObjectRecord* findById(
            DisplayObjectId id) const;

        std::vector<DisplayObjectRecord> recordsInLayer(
            DisplayLayer layer) const;

        std::vector<DisplayObjectRecord> takeRecordsInLayer(
            DisplayLayer layer);

        bool contains(
            DisplayObjectId id) const;

        void removeById(
            DisplayObjectId id);

        void clear();

        const std::vector<DisplayObjectRecord>& records() const;

    private:
        std::vector<DisplayObjectRecord> m_records;
        DisplayObjectId m_nextId = 1;
    };
}

#endif // DISPLAYOBJECTREGISTRY_H
