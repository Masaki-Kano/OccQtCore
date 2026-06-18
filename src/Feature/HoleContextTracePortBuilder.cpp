#include "Feature/HoleContextTracePortBuilder.h"

#include <algorithm>
#include <map>
#include <queue>
#include <set>

#include <gp_Vec.hxx>
#include <BRep_Tool.hxx>
#include <gp_Pnt.hxx>

#include "Core/CollectionUtil.h"
#include "Geometry/TopologyQuery.h"

namespace OccQtCore::Feature
{
    HoleContextTracePortBuilder::HoleContextTracePortBuilder(
        const GeometryModel& model)
        : m_model(model)
    {
    }

    std::vector<HoleContextTracePort>
    HoleContextTracePortBuilder::build(
        const std::vector<HoleContextGeometryGroup>& groups) const
    {
        std::vector<HoleContextTracePort> ports;

        int nextPortIndex = 0;

        for (const auto& group : groups)
        {
            auto groupPorts = buildPortsOfGroup(group, nextPortIndex);

            ports.insert(
                ports.end(),
                groupPorts.begin(),
                groupPorts.end());
        }

        return ports;
    }

    std::vector<HoleContextTracePort>
    HoleContextTracePortBuilder::buildForGroup(
        const std::vector<HoleContextGeometryGroup>& groups,
        int sourceGroupIndex,
        int startPortIndex) const
    {
        for (const auto& group : groups)
        {
            if (group.index != sourceGroupIndex)
            {
                continue;
            }

            int nextPortIndex = startPortIndex;

            return buildPortsOfGroup(
                group,
                nextPortIndex);
        }

        return {};
    }

    std::vector<HoleContextTracePort>
    HoleContextTracePortBuilder::buildPortsOfGroup(
        const HoleContextGeometryGroup& group,
        int& nextPortIndex) const
    {
        std::vector<HoleContextTracePort> ports;

        const auto edgeIndices = collectPortCandidateEdgesOfGroup(group);
        const auto components = buildEdgeComponents(edgeIndices);

        for (const auto& component : components)
        {
            if (component.edgeIndices.empty())
            {
                continue;
            }

            ports.push_back(
                buildPortFromComponent(
                    group,
                    component,
                    nextPortIndex));

            ++nextPortIndex;
        }

        return ports;
    }

    std::vector<int>
    HoleContextTracePortBuilder::collectPortCandidateEdgesOfGroup(
        const HoleContextGeometryGroup& group) const
    {
        std::vector<int> candidateEdgeIndices;

        for (const int faceIndex : group.geometryRefs.faceIndices)
        {
            if (!TopologyQuery::isValidFaceIndex(m_model, faceIndex))
            {
                continue;
            }

            const auto wireIndices =
                m_model.graph().wiresOfFace(faceIndex);

            for (const int wireIndex : wireIndices)
            {
                if (!TopologyQuery::isValidWireIndex(m_model, wireIndex))
                {
                    continue;
                }

                const auto edgeIndices =
                    m_model.graph().edgesOfWire(wireIndex);

                for (const int edgeIndex : edgeIndices)
                {
                    if (!isPortCandidateEdge(group, edgeIndex))
                    {
                        continue;
                    }

                    CollectionUtil::addUnique(
                        candidateEdgeIndices,
                        edgeIndex);
                }
            }
        }

        CollectionUtil::sortUnique(candidateEdgeIndices);

        return candidateEdgeIndices;
    }

    bool HoleContextTracePortBuilder::isPortCandidateEdge(
        const HoleContextGeometryGroup& group,
        int edgeIndex) const
    {
        if (!TopologyQuery::isValidEdgeIndex(m_model, edgeIndex))
        {
            return false;
        }

        if (hasOutsideFace(group, edgeIndex))
        {
            return true;
        }

        if (isOpenBoundaryEdgeOfGroup(group, edgeIndex))
        {
            return true;
        }

        return false;
    }

    bool HoleContextTracePortBuilder::isOpenBoundaryEdgeOfGroup(
        const HoleContextGeometryGroup& group,
        int edgeIndex) const
    {
        const auto faceIndices =
            TopologyQuery::facesOfEdge(m_model, edgeIndex);

        if (faceIndices.size() != 1)
        {
            return false;
        }

        return containsFaceIndex(group, faceIndices.front());
    }

    std::vector<HoleContextTracePortBuilder::EdgeComponent>
    HoleContextTracePortBuilder::buildEdgeComponents(
        const std::vector<int>& edgeIndices) const
    {
        std::vector<EdgeComponent> components;

        const std::set<int> candidateEdgeSet(
            edgeIndices.begin(),
            edgeIndices.end());

        std::set<int> visitedEdges;

        for (const int startEdgeIndex : edgeIndices)
        {
            if (visitedEdges.find(startEdgeIndex) != visitedEdges.end())
            {
                continue;
            }

            EdgeComponent component;

            std::queue<int> edgeQueue;
            edgeQueue.push(startEdgeIndex);
            visitedEdges.insert(startEdgeIndex);

            while (!edgeQueue.empty())
            {
                const int edgeIndex = edgeQueue.front();
                edgeQueue.pop();

                CollectionUtil::addUnique(
                    component.edgeIndices,
                    edgeIndex);

                const auto vertexIndices =
                    m_model.graph().verticesOfEdge(edgeIndex);

                for (const int vertexIndex : vertexIndices)
                {
                    if (!TopologyQuery::isValidVertexIndex(m_model, vertexIndex))
                    {
                        continue;
                    }

                    CollectionUtil::addUnique(
                        component.vertexIndices,
                        vertexIndex);

                    const auto adjacentEdgeIndices =
                        m_model.graph().edgesOfVertex(vertexIndex);

                    for (const int adjacentEdgeIndex : adjacentEdgeIndices)
                    {
                        if (candidateEdgeSet.find(adjacentEdgeIndex) == candidateEdgeSet.end())
                        {
                            continue;
                        }

                        if (visitedEdges.find(adjacentEdgeIndex) != visitedEdges.end())
                        {
                            continue;
                        }

                        visitedEdges.insert(adjacentEdgeIndex);
                        edgeQueue.push(adjacentEdgeIndex);
                    }
                }
            }

            CollectionUtil::sortUnique(component.edgeIndices);
            CollectionUtil::sortUnique(component.vertexIndices);

            components.push_back(component);
        }

        return components;
    }

    HoleContextTracePort
    HoleContextTracePortBuilder::buildPortFromComponent(
        const HoleContextGeometryGroup& group,
        const EdgeComponent& component,
        int portIndex) const
    {
        HoleContextTracePort port;

        port.index = portIndex;
        port.sourceGroupIndex = group.index;

        port.geometryRefs.edgeIndices = component.edgeIndices;
        port.geometryRefs.vertexIndices = component.vertexIndices;

        fillAxialRange(group, port);

        if (hasOutsideFace(group, component.edgeIndices))
        {
            port.kind = HoleContextTracePortKind::ExternalTransition;
            port.note = "ExternalTransition";
        }
        else
        {
            port.kind = HoleContextTracePortKind::InternalLoop;
            port.note = "InternalLoop";
        }

        return port;
    }

    void HoleContextTracePortBuilder::fillAxialRange(
        const HoleContextGeometryGroup& group,
        HoleContextTracePort& port) const
    {
        if (!group.hasReferenceDirection)
        {
            return;
        }

        bool hasValue = false;

        double axialMin = 0.0;
        double axialMax = 0.0;

        for (const int vertexIndex : port.geometryRefs.vertexIndices)
        {
            if (!TopologyQuery::isValidVertexIndex(m_model, vertexIndex))
            {
                continue;
            }

            const VertexData* vertexData = m_model.vertexAt(vertexIndex);
            if (vertexData == nullptr)
            {
                continue;
            }

            const gp_Pnt point = BRep_Tool::Pnt(vertexData->shape);

            const gp_Vec axisVector(group.referencePoint, point);
            const double axial =
                axisVector.Dot(gp_Vec(group.referenceDirection));

            if (!hasValue)
            {
                axialMin = axial;
                axialMax = axial;
                hasValue = true;
            }
            else
            {
                axialMin = std::min(axialMin, axial);
                axialMax = std::max(axialMax, axial);
            }
        }

        if (!hasValue)
        {
            return;
        }

        port.axialMin = axialMin;
        port.axialMax = axialMax;
        port.axialPosition = 0.5 * (axialMin + axialMax);
    }

    bool HoleContextTracePortBuilder::hasOutsideFace(
        const HoleContextGeometryGroup& group,
        const std::vector<int>& edgeIndices) const
    {
        for (const int edgeIndex : edgeIndices)
        {
            if (hasOutsideFace(group, edgeIndex))
            {
                return true;
            }
        }

        return false;
    }

    bool HoleContextTracePortBuilder::hasOutsideFace(
        const HoleContextGeometryGroup& group,
        int edgeIndex) const
    {
        if (!TopologyQuery::isValidEdgeIndex(m_model, edgeIndex))
        {
            return false;
        }

        for (const int groupFaceIndex : group.geometryRefs.faceIndices)
        {
            if (!TopologyQuery::isValidFaceIndex(m_model, groupFaceIndex))
            {
                continue;
            }

            const auto faceEdgeIndices =
                TopologyQuery::edgesOfFace(m_model, groupFaceIndex);

            if (!CollectionUtil::contains(faceEdgeIndices, edgeIndex))
            {
                continue;
            }

            const auto adjacentFaceIndices =
                TopologyQuery::adjacentFacesOfEdge(
                    m_model,
                    edgeIndex,
                    groupFaceIndex);

            for (const int adjacentFaceIndex : adjacentFaceIndices)
            {
                if (!containsFaceIndex(group, adjacentFaceIndex))
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool HoleContextTracePortBuilder::containsFaceIndex(
        const HoleContextGeometryGroup& group,
        int faceIndex) const
    {
        return CollectionUtil::contains(
            group.geometryRefs.faceIndices,
            faceIndex);
    }
}
