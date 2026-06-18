#ifndef HOLECONTEXTTRACESTEPBUILDER_H
#define HOLECONTEXTTRACESTEPBUILDER_H

#include "Feature/HoleRecognitionModel.h"
#include "Geometry/GeometryModel.h"

#include <vector>

namespace OccQtCore::Feature
{

    class HoleContextTraceStepBuilder
    {
    public:
        HoleContextTraceStepBuilder(const GeometryModel& model, const std::vector<HoleContextGeometryGroup>& groups, const std::vector<HoleContextTracePort>& ports);

        std::vector<HoleContextTraceStep> build() const;

    private:
        void buildStepFromPort(const HoleContextTracePort& port, HoleContextTraceStep& step) const;

        void collectOutsideFaces(const HoleContextTracePort& port, HoleContextTraceStep& step) const;

        void classifyStep(HoleContextTraceStep& step) const;

    private:
        const GeometryModel& m_model;
        const std::vector<HoleContextGeometryGroup>& m_groups;
        const std::vector<HoleContextTracePort>& m_ports;
    };
}

#endif // HOLECONTEXTTRACESTEPBUILDER_H
