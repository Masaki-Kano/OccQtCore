#ifndef DISPLAYSTYLE_H
#define DISPLAYSTYLE_H

#include <AIS_DisplayMode.hxx>
#include <Quantity_Color.hxx>
#include <Quantity_NameOfColor.hxx>

namespace OccQtCore
{
struct DisplayStyle
{
    enum class Preset
    {
        DefaultShape,
        PickHighlightFace,

        ContextGroupFace,
        TracePortEdge
    };

    Quantity_Color color = Quantity_Color(Quantity_NOC_WHITE);
    AIS_DisplayMode displayMode = AIS_Shaded;
    double lineWidth = 1.0;
    double transparency = 0.0;

    static DisplayStyle preset(Preset preset);
};
}

#endif // DISPLAYSTYLE_H
