#include "View/DisplayStyle.h"

namespace OccQtCore
{
DisplayStyle DisplayStyle::preset(Preset preset)
{
    DisplayStyle style;

    switch (preset)
    {
    case Preset::DefaultShape:
        style.color = Quantity_Color(0.75, 0.78, 0.82, Quantity_TOC_RGB);
        style.transparency = 0.0;
        style.displayMode = AIS_Shaded;
        break;

    case Preset::PickHighlightFace:
        style.displayMode = AIS_WireFrame;
        style.color = Quantity_Color(Quantity_NOC_YELLOW);
        style.transparency = 0.0;
        style.lineWidth = 4.0;
        break;

    case Preset::ContextGroupFace:
        style.color = Quantity_Color(Quantity_NOC_ORANGE);
        style.transparency = 0.55;
        style.displayMode = AIS_Shaded;
        break;

    case Preset::TracePortEdge:
        style.color = Quantity_Color(Quantity_NOC_RED);
        style.transparency = 0.0;
        style.displayMode = AIS_WireFrame;
        break;
    }

    return style;
}
}
