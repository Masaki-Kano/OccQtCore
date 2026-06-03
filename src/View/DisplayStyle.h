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

        AnalysisCandidateWire,
        AnalysisComponentEdge,
        AnalysisAdjacentFace,

        HoleWallFace,

        HoleOpenFace,
        HoleOpenEdge,

        HoleBottomFace,
        HoleBottomEdge,

        HoleConnectionFace,
        HoleConnectionEdge
    };

    Quantity_Color color = Quantity_Color(Quantity_NOC_WHITE);
    double transparency = 0.0;
    AIS_DisplayMode displayMode = AIS_Shaded;

    static DisplayStyle preset(Preset preset);
};
}

#endif // DISPLAYSTYLE_H
