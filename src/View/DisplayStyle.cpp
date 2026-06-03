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
        style.color = Quantity_Color(Quantity_NOC_CYAN);
        style.transparency = 0.45;
        style.displayMode = AIS_Shaded;
        break;

    case Preset::AnalysisCandidateWire:
        style.color = Quantity_Color(Quantity_NOC_CYAN);
        style.transparency = 0.0;
        style.displayMode = AIS_WireFrame;
        break;

    case Preset::AnalysisComponentEdge:
        style.color = Quantity_Color(Quantity_NOC_YELLOW);
        style.transparency = 0.0;
        style.displayMode = AIS_WireFrame;
        break;

    case Preset::AnalysisAdjacentFace:
        style.color = Quantity_Color(Quantity_NOC_ORANGE);
        style.transparency = 0.55;
        style.displayMode = AIS_Shaded;
        break;

    case Preset::HoleWallFace:
        style.color = Quantity_Color(Quantity_NOC_ORANGE);
        style.transparency = 0.55;
        style.displayMode = AIS_Shaded;
        break;

    case Preset::HoleOpenFace:
        style.color = Quantity_Color(Quantity_NOC_GREEN);
        style.transparency = 0.50;
        style.displayMode = AIS_Shaded;
        break;

    case Preset::HoleOpenEdge:
        style.color = Quantity_Color(Quantity_NOC_GREEN);
        style.transparency = 0.0;
        style.displayMode = AIS_WireFrame;
        break;

    case Preset::HoleBottomFace:
        style.color = Quantity_Color(Quantity_NOC_RED);
        style.transparency = 0.45;
        style.displayMode = AIS_Shaded;
        break;

    case Preset::HoleBottomEdge:
        style.color = Quantity_Color(Quantity_NOC_RED);
        style.transparency = 0.0;
        style.displayMode = AIS_WireFrame;
        break;

    case Preset::HoleConnectionFace:
        style.color = Quantity_Color(Quantity_NOC_CYAN);
        style.transparency = 0.45;
        style.displayMode = AIS_Shaded;
        break;

    case Preset::HoleConnectionEdge:
        style.color = Quantity_Color(Quantity_NOC_CYAN);
        style.transparency = 0.0;
        style.displayMode = AIS_WireFrame;
        break;
    }

    return style;
}
}
