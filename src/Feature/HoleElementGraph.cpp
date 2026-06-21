#include "Feature/HoleElementGraph.h"

#include <utility>

#include "Core/CollectionUtil.h"

namespace OccQtCore::Feature
{
    void HoleElementGraph::clear()
    {
        m_elements.clear();
        m_connections.clear();
        m_elementToConnections.clear();
    }

    int HoleElementGraph::addElement(HoleElement element)
    {
        element.index = static_cast<int>(m_elements.size());

        const int elementIndex = element.index;

        m_elements.push_back(std::move(element));

        m_elementToConnections.emplace_back();

        return elementIndex;
    }

    int HoleElementGraph::addConnection(HoleElementConnection connection)
    {
        const int lhsElementIndex =
            connection.lhsElementIndex;

        const int rhsElementIndex =
            connection.rhsElementIndex;

        if (lhsElementIndex < 0 ||
            rhsElementIndex < 0 ||
            lhsElementIndex >=
                static_cast<int>(m_elements.size()) ||
            rhsElementIndex >=
                static_cast<int>(m_elements.size()) ||
            lhsElementIndex == rhsElementIndex)
        {
            return -1;
        }

        connection.index =
            static_cast<int>(m_connections.size());

        const int connectionIndex =
            connection.index;

        m_connections.push_back(
            std::move(connection));

        CollectionUtil::addUnique(
            m_elementToConnections[lhsElementIndex],
            connectionIndex);

        CollectionUtil::addUnique(
            m_elementToConnections[rhsElementIndex],
            connectionIndex);

        return connectionIndex;
    }

    const std::vector<HoleElement>& HoleElementGraph::elements() const
    {
        return m_elements;
    }

    const std::vector<HoleElementConnection>& HoleElementGraph::connections() const
    {
        return m_connections;
    }

    const HoleElement* HoleElementGraph::elementAt(int elementIndex) const
    {
        if (elementIndex < 0 ||
            elementIndex >= static_cast<int>(m_elements.size()))
        {
            return nullptr;
        }

        return &m_elements[elementIndex];
    }

    const HoleElementConnection* HoleElementGraph::connectionAt(int connectionIndex) const
    {
        if (connectionIndex < 0 ||
            connectionIndex >= static_cast<int>(m_connections.size()))
        {
            return nullptr;
        }

        return &m_connections[connectionIndex];
    }

    std::vector<int>
    HoleElementGraph::connectionIndicesOfElement(
        int elementIndex) const
    {
        if (elementIndex < 0 ||
            elementIndex >=
                static_cast<int>(
                    m_elementToConnections.size()))
        {
            return {};
        }

        return m_elementToConnections[elementIndex];
    }

    std::vector<int> HoleElementGraph::adjacentElementIndices(int elementIndex) const
    {
        std::vector<int> adjacentElementIndices;

        for (const int connectionIndex : connectionIndicesOfElement(elementIndex))
        {
            const auto* connection = connectionAt(connectionIndex);

            if (connection == nullptr)
            {
                continue;
            }

            const int adjacentElementIndex =
                connection->lhsElementIndex ==
                        elementIndex
                    ? connection->rhsElementIndex
                    : connection->lhsElementIndex;

            CollectionUtil::addUnique(
                adjacentElementIndices,
                adjacentElementIndex);
        }

        CollectionUtil::sortUnique(adjacentElementIndices);

        return adjacentElementIndices;
    }


}
