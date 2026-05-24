#ifndef SELECTIONINFO_H
#define SELECTIONINFO_H

#include <TopoDS_Shape.hxx>

#include "Core/PickResult.h"

namespace OccQtCore
{
    struct SelectionInfo
    {
        bool isValid = false;

        PickedShapeType type = PickedShapeType::Unknown;
        TopoDS_Shape shape;

        int elementIndex = -1;
        int sourceDisplayObjectId = -1;
    };
}

#endif // SELECTIONINFO_H
