#include <cmath>
#include <algorithm>

#include <BRepAdaptor_Surface.hxx>
#include <BRepLProp_SLProps.hxx>
#include <TopAbs_Orientation.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Vec.hxx>

#include "Feature/HoleFeatureRecognizer.h"

#include "Geometry/GeometryModel.h"
#include "Geometry/GeometryGraph.h"

namespace
{
    constexpr double Pi = 3.14159265358979323846;
    constexpr double RadiusTolerance = 1.0e-4;
    constexpr double CenterTolerance = 1.0e-6;
    constexpr double AxisLineTolerance = 1.0e-4;
    constexpr double DirectionTolerance = 1.0e-6;
    constexpr double ArcLengthTolerance = 1.0e-3;
    constexpr double AngleTolerance = 1.0e-3;

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

    bool isPointOnAxis(
        const gp_Pnt& axisPoint,
        const gp_Dir& axisDirection,
        const gp_Pnt& point)
    {
        const gp_Vec v(axisPoint, point);
        const gp_Vec axisVec(axisDirection);

        return v.Crossed(axisVec).Magnitude() <= AxisLineTolerance;
    }

    bool isSameCylinderCandidate(
        const OccQtCore::Feature::HoleWallCandidate& lhs,
        const OccQtCore::Feature::HoleWallCandidate& rhs)
    {
        if (!isSameRadius(lhs.radius, rhs.radius))
        {
            return false;
        }

        if (!isSameDirectionOrReverse(lhs.axisDirection, rhs.axisDirection))
        {
            return false;
        }

        if (!isPointOnAxis(lhs.center, lhs.axisDirection, rhs.center))
        {
            return false;
        }

        return true;
    }

    bool hasSharedEdge(
        const OccQtCore::GeometryModel& model,
        int lhsFaceIndex,
        int rhsFaceIndex)
    {
        const auto& graph = model.graph();

        for (int wireIndex : graph.wiresOfFace(lhsFaceIndex))
        {
            for (int edgeIndex : graph.edgesOfWire(wireIndex))
            {
                const auto connectedFaceIndices = graph.facesOfEdge(edgeIndex);

                if (containsIndex(connectedFaceIndices, rhsFaceIndex))
                {
                    return true;
                }
            }
        }

        return false;
    }

    double cylinderFaceUSpan(
        const OccQtCore::GeometryModel& model,
        int faceIndex)
    {
        const auto* faceData = model.faceAt(faceIndex);

        if (faceData == nullptr)
        {
            return 0.0;
        }

        return std::abs(faceData->info.uMax - faceData->info.uMin);
    }

    bool isClosedCylinderWallGroup(
        const OccQtCore::GeometryModel& model,
        const std::vector<OccQtCore::Feature::HoleWallCandidate>& wallCandidates,
        const std::vector<int>& candidateIndices)
    {
        double totalUSpan = 0.0;

        for (int candidateIndex : candidateIndices)
        {
            if (candidateIndex < 0 || candidateIndex >= static_cast<int>(wallCandidates.size()))
            {
                continue;
            }

            const int faceIndex = wallCandidates[candidateIndex].faceIndex;
            totalUSpan += cylinderFaceUSpan(model, faceIndex);
        }

        return totalUSpan >= 2.0 * Pi - AngleTolerance;
    }


    bool isHoleEndTransitionSurface(OccQtCore::SurfaceKind kind)
    {
        return kind == OccQtCore::SurfaceKind::Cone
               || kind == OccQtCore::SurfaceKind::Torus;
    }

    bool isHoleWallSurface(OccQtCore::SurfaceKind kind)
    {
        return kind == OccQtCore::SurfaceKind::Cylinder;
    }

    bool transitionSurfaceReachesWallOneStep(
        const OccQtCore::GeometryModel& model,
        int transitionFaceIndex,
        int sourceEdgeIndex,
        const gp_Pnt& baseCenter,
        const gp_Dir& baseAxis,
        double baseRadius)
    {
        const auto& graph = model.graph();
        const auto& faces = model.faces();
        const auto& edges = model.edges();

        if (transitionFaceIndex < 0 || transitionFaceIndex >= static_cast<int>(faces.size()))
        {
            return false;
        }

        const auto wireIndices = graph.wiresOfFace(transitionFaceIndex);

        for (int wireIndex : wireIndices)
        {
            const auto edgeIndices = graph.edgesOfWire(wireIndex);

            for (int edgeIndex : edgeIndices)
            {
                if (edgeIndex == sourceEdgeIndex)
                {
                    continue;
                }

                if (edgeIndex < 0 || edgeIndex >= static_cast<int>(edges.size()))
                {
                    continue;
                }

                const auto& edgeData = edges[edgeIndex];

                if (edgeData.info.kind != OccQtCore::CurveKind::Circle ||
                    !edgeData.info.circle.has_value())
                {
                    continue;
                }

                const auto& circle = edgeData.info.circle.value();

                const gp_Pnt center = circle.center;
                const gp_Dir axis = circle.axis.Direction();
                const double radius = circle.radius;

                if (!isPointOnAxis(baseCenter, baseAxis, center))
                {
                    continue;
                }

                if (!isSameDirectionOrReverse(baseAxis, axis))
                {
                    continue;
                }

                if (radius > baseRadius + RadiusTolerance)
                {
                    continue;
                }

                const auto connectedFaceIndices = graph.facesOfEdge(edgeIndex);

                for (int connectedFaceIndex : connectedFaceIndices)
                {
                    if (connectedFaceIndex == transitionFaceIndex)
                    {
                        continue;
                    }

                    if (connectedFaceIndex < 0 || connectedFaceIndex >= static_cast<int>(faces.size()))
                    {
                        continue;
                    }

                    if (isHoleWallSurface(faces[connectedFaceIndex].info.kind))
                    {
                        return true;
                    }
                }
            }
        }

        return false;
    }

    bool isCylinderFaceInwardOriented(
        const OccQtCore::GeometryModel& model,
        int faceIndex)
    {
        const auto* faceData = model.faceAt(faceIndex);

        if (faceData == nullptr)
        {
            return false;
        }

        if (faceData->info.kind != OccQtCore::SurfaceKind::Cylinder ||
            !faceData->info.cylinder.has_value())
        {
            return false;
        }

        const TopoDS_Face face = TopoDS::Face(faceData->shape);

        BRepAdaptor_Surface surface(face);

        const double u = 0.5 * (faceData->info.uMin + faceData->info.uMax);
        const double v = 0.5 * (faceData->info.vMin + faceData->info.vMax);

        BRepLProp_SLProps props(surface, u, v, 1, 1.0e-6);

        if (!props.IsNormalDefined())
        {
            return false;
        }

        const gp_Pnt point = props.Value();
        gp_Dir normal = props.Normal();

        // OCCのFace向きを反映する。
        if (face.Orientation() == TopAbs_REVERSED)
        {
            normal.Reverse();
        }

        const auto& cylinder = faceData->info.cylinder.value();

        const gp_Pnt axisOrigin = cylinder.axis.Location();
        const gp_Dir axisDirection = cylinder.axis.Direction();

        const gp_Vec axisVec(axisDirection);
        const gp_Vec originToPoint(axisOrigin, point);

        const double t = originToPoint.Dot(axisVec);

        const gp_Pnt projectedPoint(
            axisOrigin.X() + axisVec.X() * t,
            axisOrigin.Y() + axisVec.Y() * t,
            axisOrigin.Z() + axisVec.Z() * t);

        gp_Vec radial(projectedPoint, point);

        if (radial.Magnitude() <= 1.0e-6)
        {
            return false;
        }

        radial.Normalize();

        const gp_Vec normalVec(normal);

        // radial は「円筒軸 → 面上点」方向。
        // 外径円筒: 法線が radial と同方向になりやすい。
        // 内径/穴壁: 法線が radial と逆方向になりやすい。
        return normalVec.Dot(radial) < 0.0;
    }

    bool isLikelyHoleWallByOrientation(
        const OccQtCore::GeometryModel& model,
        const OccQtCore::Feature::HoleWallComponent& component)
    {
        int inwardCount = 0;
        int outwardCount = 0;

        for (int faceIndex : component.geometryRefs.faceIndices)
        {
            if (isCylinderFaceInwardOriented(model, faceIndex))
            {
                ++inwardCount;
            }
            else
            {
                ++outwardCount;
            }
        }

        return inwardCount > 0;
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

    std::vector<HoleWallCandidate> HoleFeatureRecognizer::detectWallCandidates(
        const GeometryModel& model) const
    {
        std::vector<HoleWallCandidate> candidates;

        for (const auto& face : model.faces())
        {
            if (face.info.kind != SurfaceKind::Cylinder)
            {
                continue;
            }

            if (!face.info.cylinder.has_value())
            {
                continue;
            }

            const auto& cylinder = face.info.cylinder.value();

            HoleWallCandidate candidate;
            candidate.faceIndex = face.index;
            candidate.center = cylinder.axis.Location();
            candidate.axisDirection = cylinder.axis.Direction();
            candidate.radius = cylinder.radius;

            candidates.push_back(candidate);
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

                if (edgeData.info.kind != CurveKind::Circle ||
                    !edgeData.info.circle.has_value())
                {
                    circular = false;
                    break;
                }

                const auto& circle = edgeData.info.circle.value();

                const gp_Pnt center = circle.center;
                const gp_Dir axis = circle.axis.Direction();
                const double radius = circle.radius;

                const double arcLength = edgeData.info.length;

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

            std::vector<int> componentFaceIndices;
            addUniqueIndex(componentFaceIndices, faceIndex);

            bool hasWallConnection = false;

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
                        const auto surfaceKind = connectedFace.info.kind;

                        if (isHoleWallSurface(surfaceKind))
                        {
                            // 面取り無しなど、入口円エッジが直接壁に接続するパターン。
                            // Cylinder は HoleEndComponent の構成Faceには含めない。
                            hasWallConnection = true;
                        }
                        else if (isHoleEndTransitionSurface(surfaceKind))
                        {
                            // 面取り面/R面など、穴端コンポーネントの構成Face。
                            addUniqueIndex(componentFaceIndices, connectedFaceIndex);

                            if (transitionSurfaceReachesWallOneStep(
                                    model,
                                    connectedFaceIndex,
                                    edgeIndex,
                                    baseCenter,
                                    baseAxis,
                                    baseRadius))
                            {
                                hasWallConnection = true;
                            }
                        }
                    }
                }
            }

            if (!hasWallConnection)
            {
                continue;
            }

            HoleEndComponent component;

            component.geometryRefs.faceIndices = componentFaceIndices;
            component.geometryRefs.wireIndices.push_back(wireIndex);
            component.geometryRefs.edgeIndices = edgeIndices;

            component.endType = candidate.endType;
            component.center = baseCenter;
            component.axisDirection = baseAxis;

            // 今は暫定で円軸方向を入れる。
            // 親Face法線を安定取得できるようになったら normalDirection に置き換える。
            component.normalDirection = baseAxis;

            component.radius = baseRadius;

            components.push_back(component);
        }

        return components;
    }

    std::vector<HoleWallComponent> HoleFeatureRecognizer::buildWallComponents(
        const GeometryModel& model,
        const std::vector<HoleWallCandidate>& wallCandidates) const
    {
        std::vector<HoleWallComponent> components;

        std::vector<bool> used(wallCandidates.size(), false);

        for (int baseCandidateIndex = 0;
             baseCandidateIndex < static_cast<int>(wallCandidates.size());
             ++baseCandidateIndex)
        {
            if (used[baseCandidateIndex])
            {
                continue;
            }

            const auto& baseCandidate = wallCandidates[baseCandidateIndex];

            std::vector<int> groupCandidateIndices;
            std::vector<int> stack;

            used[baseCandidateIndex] = true;
            stack.push_back(baseCandidateIndex);

            while (!stack.empty())
            {
                const int currentCandidateIndex = stack.back();
                stack.pop_back();

                groupCandidateIndices.push_back(currentCandidateIndex);

                const auto& currentCandidate = wallCandidates[currentCandidateIndex];

                for (int nextCandidateIndex = 0;
                     nextCandidateIndex < static_cast<int>(wallCandidates.size());
                     ++nextCandidateIndex)
                {
                    if (used[nextCandidateIndex])
                    {
                        continue;
                    }

                    const auto& nextCandidate = wallCandidates[nextCandidateIndex];

                    if (!isSameCylinderCandidate(currentCandidate, nextCandidate))
                    {
                        continue;
                    }

                    if (!hasSharedEdge(
                            model,
                            currentCandidate.faceIndex,
                            nextCandidate.faceIndex))
                    {
                        continue;
                    }

                    used[nextCandidateIndex] = true;
                    stack.push_back(nextCandidateIndex);
                }
            }

            if (!isClosedCylinderWallGroup(model, wallCandidates, groupCandidateIndices))
            {
                continue;
            }

            HoleWallComponent component;

            for (int candidateIndex : groupCandidateIndices)
            {
                const int faceIndex = wallCandidates[candidateIndex].faceIndex;
                addUniqueIndex(component.geometryRefs.faceIndices, faceIndex);
            }

            component.center = baseCandidate.center;
            component.axisDirection = baseCandidate.axisDirection;
            component.radius = baseCandidate.radius;
            component.depth = 0.0;

            if (!isLikelyHoleWallByOrientation(model, component))
            {
                continue;
            }

            components.push_back(component);
        }

        return components;
    }
}
