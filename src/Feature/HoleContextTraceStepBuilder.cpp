#include "Feature/HoleContextTraceStepBuilder.h"

#include "Core/CollectionUtil.h"
#include "Feature/HoleContextQuery.h"
#include "Geometry/TopologyQuery.h"

namespace OccQtCore::Feature
{
    namespace
    {
    namespace HoleContextQuery = OccQtCore::Feature::HoleContextQuery;
    }

    HoleContextTraceStepBuilder::HoleContextTraceStepBuilder(
        const GeometryModel& model,
        const std::vector<HoleContextGeometryGroup>& groups,
        const std::vector<HoleContextTracePort>& ports)
        : m_model(model)
        , m_groups(groups)
        , m_ports(ports)
    {
    }

    std::vector<HoleContextTraceStep> HoleContextTraceStepBuilder::build() const
    {
        std::vector<HoleContextTraceStep> steps;

        for (const auto& port : m_ports)
        {
            HoleContextTraceStep step;

            step.index = static_cast<int>(steps.size());
            step.sourceGroupIndex = port.sourceGroupIndex;
            step.sourcePortIndex = port.index;
            step.portGeometryRefs = port.geometryRefs;

            buildStepFromPort(port, step);

            steps.push_back(step);
        }

        return steps;
    }

    void HoleContextTraceStepBuilder::buildStepFromPort(const HoleContextTracePort& port, HoleContextTraceStep& step) const
    {
        collectOutsideFaces(port, step);
        classifyStep(step);
    }

    void HoleContextTraceStepBuilder::collectOutsideFaces(const HoleContextTracePort& port, HoleContextTraceStep& step) const
    {
        const HoleContextGeometryGroup* sourceGroup =
            HoleContextQuery::findGroupByIndex(
                m_groups,
                port.sourceGroupIndex);

        if (sourceGroup == nullptr)
        {
            return;
        }

        for (const int edgeIndex : port.geometryRefs.edgeIndices)
        {
            const auto faceIndices =
                TopologyQuery::facesOfEdge(
                    m_model,
                    edgeIndex);

            for (const int faceIndex : faceIndices)
            {
                if (HoleContextQuery::groupContainsFace(
                        *sourceGroup,
                        faceIndex))
                {
                    continue;
                }

                CollectionUtil::addUnique(
                    step.outsideGeometryRefs.faceIndices,
                    faceIndex);
            }
        }

        CollectionUtil::sortUnique(step.outsideGeometryRefs.faceIndices);
    }

    void HoleContextTraceStepBuilder::classifyStep(
        HoleContextTraceStep& step) const
    {
        if (step.outsideGeometryRefs.faceIndices.empty())
        {
            step.kind = HoleContextTraceStepKind::NoOutsideFace;
            step.note = "No outside adjacent face found.";
            return;
        }

        step.kind = HoleContextTraceStepKind::OutsideFace;
        step.note = "Found outside adjacent faces.";
    }
}
