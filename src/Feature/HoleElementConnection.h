#ifndef HOLEELEMENTCONNECTION_H
#define HOLEELEMENTCONNECTION_H

#include <vector>

namespace OccQtCore::Feature
{
    struct HoleElementConnection
    {
        int index = -1;

        int lhsElementIndex = -1;
        int rhsElementIndex = -1;

        std::vector<int> edgeIndices;
    };
}

#endif // HOLEELEMENTCONNECTION_H
