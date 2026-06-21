#ifndef HOLEELEMENTGRAPH_H
#define HOLEELEMENTGRAPH_H

#include <vector>

#include "Feature/HoleElement.h"
#include "Feature/HoleElementConnection.h"

namespace OccQtCore::Feature
{
    class HoleElementGraph
    {
    public:
        void clear();

        int addElement(HoleElement element);

        int addConnection(HoleElementConnection connection);

        const std::vector<HoleElement>& elements() const;

        const std::vector<HoleElementConnection>& connections() const;

        const HoleElement* elementAt(int elmentIndex) const;

        const HoleElementConnection* connectionAt(int connectionIndex) const;

        std::vector<int> connectionIndicesOfElement(int elementIndex) const;

        std::vector<int> adjacentElementIndices(int elementIndex) const;

    private:
        std::vector<HoleElement> m_elements;
        std::vector<HoleElementConnection> m_connections;
        std::vector<std::vector<int>> m_elementToConnections;
    };
}

#endif // HOLEELEMENTGRAPH_H
