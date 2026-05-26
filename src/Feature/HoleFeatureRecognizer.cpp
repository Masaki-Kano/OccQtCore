#include <cmath>
#include <algorithm>

#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <gp_Circ.hxx>

#include "Feature/HoleFeatureRecognizer.h"

#include "Geometry/GeometryModel.h"
#include "Geometry/GeometryGraph.h"

namespace
{
    constexpr double Pi = 3.14159265358979323846;
    constexpr double RadiusTolerance = 1.0e-6;
    constexpr double CenterTolerance = 1.0e-6;
    constexpr double DirectionTolerance = 1.0e-6;
    constexpr double ArcLengthTolerance = 1.0e-3;

    bool containsIndex(const std::vector<int>& values, int index)
    {
        return std::find(values.begin(), values.end(), index) != values.end();
    }

    void addUniqueIndex(std::vector<int>& values, int index)
    {
        if (!containsIndex(values, index))
        {
            values.push_back(index);
        }
    }

    bool isSameRadius(double lhs, double rhs)
    {
        return std::abs(lhs - rhs) <= RadiusTolerance;
    }

    bool isSameCenter(const gp_Pnt& lhs, const gp_Pnt& rhs)
    {
        return lhs.Distance(rhs) <= CenterTolerance;
    }

    bool isSameDirectionOrReverse(const gp_Dir& lhs, const gp_Dir& rhs)
    {
        return std::abs(lhs.Dot(rhs)) >= 1.0 - DirectionTolerance;
    }

    bool isFullCircleLength(double totalArcLength, double radius)
    {
        const double expected = 2.0 * Pi * radius;
        return std::abs(totalArcLength - expected) <= ArcLengthTolerance;
    }

    bool isValidHoleAdjacentSurface(GeomAbs_SurfaceType surfaceType)
    {
        return surfaceType == GeomAbs_Cylinder
               || surfaceType == GeomAbs_Cone
               || surfaceType == GeomAbs_Torus;
    }
}

namespace OccQtCore::Feature
{

    std::vector<Hole::Data> HoleFeatureRecognizer::recognize(const GeometryModel& model) const
    {
        const auto endCandidates = detectEndCandidates(model);

        // まずは候補生成だけ確認する段階。
        // Component化、Wall候補、Element化、Hole確定は次段階で実装する。
        (void)endCandidates;

        return {};
    }

    std::vector<HoleEndCandidate> HoleFeatureRecognizer::detectEndCandidates(const GeometryModel& model) const
    {
        std::vector<HoleEndCandidate> candidates;

        const auto& graph = model.graph();
        const auto& wires = model.wires();

        for (const auto& face : model.faces())
        {
            const int faceIndex = face.index;

            const auto wireIndices = graph.wiresOfFace(faceIndex);

            for (const int wireIndex : wireIndices)
            {
                if (wireIndex < 0 || wireIndex >= static_cast<int>(wires.size()))
                {
                    continue;
                }

                const auto& wire = wires[wireIndex];

                // Candidate段階では「閉じたInnerWire」だけを見る
                if (!wire.info.isInner)
                {
                    continue;
                }

                if (!wire.info.isClosed)
                {
                    continue;
                }

                HoleEndCandidate candidate;
                candidate.faceIndex = faceIndex;
                candidate.wireIndex = wireIndex;
                candidate.endType = Hole::EndType::Open;

                candidates.push_back(candidate);
            }
        }

        return candidates;
    }

    std::vector<HoleEndComponent> HoleFeatureRecognizer::buildEndComponents(
        const GeometryModel& model,
        const std::vector<HoleEndCandidate>& endCandidates) const
    {
        std::vector<HoleEndComponent> components;

        const auto& graph = model.graph();
        const auto& faces = model.faces();
        const auto& wires = model.wires();
        const auto& edges = model.edges();

        for (const auto& candidate : endCandidates)
        {
            const int faceIndex = candidate.faceIndex;
            const int wireIndex = candidate.wireIndex;

            if (faceIndex < 0 || faceIndex >= static_cast<int>(faces.size()))
            {
                continue;
            }

            if (wireIndex < 0 || wireIndex >= static_cast<int>(wires.size()))
            {
                continue;
            }

            const auto& wire = wires[wireIndex];

            if (!wire.info.isInner || !wire.info.isClosed)
            {
                continue;
            }

            const auto edgeIndices = graph.edgesOfWire(wireIndex);

            if (edgeIndices.empty())
            {
                continue;
            }

            bool initialized = false;

            gp_Pnt baseCenter;
            gp_Dir baseAxis;
            double baseRadius = 0.0;
            double totalArcLength = 0.0;

            bool circular = true;

            for (const int edgeIndex : edgeIndices)
            {
                if (edgeIndex < 0 || edgeIndex >= static_cast<int>(edges.size()))
                {
                    circular = false;
                    break;
                }

                const auto& edgeData = edges[edgeIndex];

                BRepAdaptor_Curve curve(edgeData.shape);

                if (curve.GetType() != GeomAbs_Circle)
                {
                    circular = false;
                    break;
                }

                const gp_Circ circle = curve.Circle();

                const gp_Pnt center = circle.Location();
                const gp_Dir axis = circle.Axis().Direction();
                const double radius = circle.Radius();

                const double first = curve.FirstParameter();
                const double last = curve.LastParameter();
                const double arcLength = std::abs(last - first) * radius;

                if (!initialized)
                {
                    baseCenter = center;
                    baseAxis = axis;
                    baseRadius = radius;
                    initialized = true;
                }
                else
                {
                    if (!isSameRadius(baseRadius, radius)
                        || !isSameCenter(baseCenter, center)
                        || !isSameDirectionOrReverse(baseAxis, axis))
                    {
                        circular = false;
                        break;
                    }
                }

                totalArcLength += arcLength;
            }

            if (!circular || !initialized)
            {
                continue;
            }

            if (!isFullCircleLength(totalArcLength, baseRadius))
            {
                continue;
            }

            std::vector<int> adjacentFaceIndices;

            for (const int edgeIndex : edgeIndices)
            {
                const auto connectedWireIndices = graph.wiresOfEdge(edgeIndex);

                for (const int connectedWireIndex : connectedWireIndices)
                {
                    if (connectedWireIndex == wireIndex)
                    {
                        continue;
                    }

                    const auto connectedFaceIndices = graph.facesOfWire(connectedWireIndex);

                    for (const int connectedFaceIndex : connectedFaceIndices)
                    {
                        if (connectedFaceIndex == faceIndex)
                        {
                            continue;
                        }

                        if (connectedFaceIndex < 0 || connectedFaceIndex >= static_cast<int>(faces.size()))
                        {
                            continue;
                        }

                        const auto& connectedFace = faces[connectedFaceIndex];

                        BRepAdaptor_Surface surface(connectedFace.shape);
                        const auto surfaceType = surface.GetType();

                        if (!isValidHoleAdjacentSurface(surfaceType))
                        {
                            continue;
                        }

                        addUniqueIndex(adjacentFaceIndices, connectedFaceIndex);
                    }
                }
            }

            if (adjacentFaceIndices.empty())
            {
                continue;
            }

            HoleEndComponent component;

            component.geometryRefs.faceIndices.push_back(faceIndex);
            component.geometryRefs.wireIndices.push_back(wireIndex);
            component.geometryRefs.edgeIndices = edgeIndices;

            component.endType = candidate.endType;
            component.center = baseCenter;
            component.axisDirection = baseAxis;

            // 今は暫定で円軸方向を入れる。
            // 親Face法線を安定取得できるようになったら normalDirection に置き換える。
            component.normalDirection = baseAxis;

            component.radius = baseRadius;
            component.adjacentFaceIndices = adjacentFaceIndices;

            components.push_back(component);
        }

        return components;
    }
}
