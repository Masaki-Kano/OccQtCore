#include <cmath>
#include <algorithm>
#include <set>
#include <tuple>

#include <BRepAdaptor_Surface.hxx>
#include <BRepLProp_SLProps.hxx>
#include <TopAbs_Orientation.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Vec.hxx>

#include "Feature/HoleFeatureRecognizer.h"

#include "Geometry/GeometryModel.h"

namespace
{
    constexpr double Pi = 3.14159265358979323846;
    constexpr double RadiusTolerance = 1.0e-4;
    constexpr double AxisLineTolerance = 1.0e-4;
    constexpr double DirectionTolerance = 1.0e-6;
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

    void normalizeIndices(std::vector<int>& indices)
    {
        std::sort(indices.begin(), indices.end());

        indices.erase(
            std::unique(indices.begin(), indices.end()),
            indices.end());
    }

    struct HoleEndComponentKey
    {
        int wallComponentIndex = -1;
        std::vector<int> faceIndices;
        std::vector<int> edgeIndicesForFaceLessOpen;
        OccQtCore::Feature::Hole::EndType endType =
            OccQtCore::Feature::Hole::EndType::Unknown;

        bool operator<(const HoleEndComponentKey& other) const
        {
            return std::tie(
                       wallComponentIndex,
                       faceIndices,
                       edgeIndicesForFaceLessOpen,
                       endType)
                   < std::tie(
                       other.wallComponentIndex,
                       other.faceIndices,
                       other.edgeIndicesForFaceLessOpen,
                       other.endType);
        }
    };

    HoleEndComponentKey makeHoleEndComponentKey(
        const OccQtCore::Feature::HoleEndComponent& component)
    {
        HoleEndComponentKey key;

        key.wallComponentIndex = component.wallComponentIndex;
        key.faceIndices = component.geometryRefs.faceIndices;
        key.endType = component.endType;

        normalizeIndices(key.faceIndices);

        // 面取りなしOpenは faceIndices が空になる。
        // 貫通穴の両端Openを同一扱いで潰さないため、
        // faceIndices が空のOpen端だけ edgeIndices をキーに含める。
        if (component.endType == OccQtCore::Feature::Hole::EndType::Open &&
            component.geometryRefs.faceIndices.empty())
        {
            key.edgeIndicesForFaceLessOpen = component.geometryRefs.edgeIndices;
            normalizeIndices(key.edgeIndicesForFaceLessOpen);
        }

        return key;
    }

    bool isSameRadius(double lhs, double rhs)
    {
        return std::abs(lhs - rhs) <= RadiusTolerance;
    }

    bool isSameDirectionOrReverse(const gp_Dir& lhs, const gp_Dir& rhs)
    {
        return std::abs(lhs.Dot(rhs)) >= 1.0 - DirectionTolerance;
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
            if (candidateIndex < 0 ||
                candidateIndex >= static_cast<int>(wallCandidates.size()))
            {
                continue;
            }

            const int faceIndex = wallCandidates[candidateIndex].faceIndex;
            totalUSpan += cylinderFaceUSpan(model, faceIndex);
        }

        return totalUSpan >= 2.0 * Pi - AngleTolerance;
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

        const gp_Pnt point = props.Value();
        gp_Dir normal = props.Normal();

        if (!props.IsNormalDefined())
        {
            return false;
        }

        // OCCのFace向きを反映する
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

        const gp_Pnt projectPoint(
            axisOrigin.X() + axisVec.X() * t,
            axisOrigin.Y() + axisVec.Y() * t,
            axisOrigin.Z() + axisVec.Z() * t);

        gp_Vec radial(projectPoint, point);

        if (radial.Magnitude() <= 1.0e-6)
        {
            return false;
        }

        radial.Normalize();

        const gp_Vec normalizeVec(normal);

        // radial は「円筒軸 → 面上点」方向
        // 外形円筒: 法線がradialと同方向になりやすい。
        // 内径円筒: 法線がradialと逆方向になりやすい。
        return normalizeVec.Dot(radial) < 0.0;
    }

    bool isLikelyHoleWallByOrientation(
        const OccQtCore::GeometryModel& model,
        const OccQtCore::Feature::HoleWallComponent& component)
    {
        int inwardCount = 0;

        for (int faceIndex : component.geometryRefs.faceIndices)
        {
            if (isCylinderFaceInwardOriented(model, faceIndex))
            {
                ++inwardCount;
            }
        }

        return inwardCount > 0;
    }

    bool isConnectionEdgeOnInnerWireOfFace(
        const OccQtCore::GeometryModel& model,
        int faceIndex,
        int edgeIndex)
    {
        const auto& graph = model.graph();
        const auto& wires = model.wires();

        const auto& wireIndices = graph.wiresOfFace(faceIndex);

        for (int wireIndex : wireIndices)
        {
            if (wireIndex < 0 ||
                wireIndex >= static_cast<int>(wires.size()))
            {
                continue;
            }

            const auto edgeIndices = graph.edgesOfWire(wireIndex);

            if (!containsIndex(edgeIndices, edgeIndex))
            {
                continue;
            }

            const auto& wire = wires[wireIndex];

            return wire.info.isInner && wire.info.isClosed;
        }

        return false;
    }

    bool isTransitionSurface(OccQtCore::SurfaceKind kind)
    {
        return kind == OccQtCore::SurfaceKind::Cone ||
               kind == OccQtCore::SurfaceKind::Torus;
    }

    OccQtCore::Feature::HoleEndComponent makeBaseEndComponentFromWall(
        const OccQtCore::Feature::HoleWallComponent& wallComponent)
    {
        OccQtCore::Feature::HoleEndComponent component;

        component.wallComponentIndex = wallComponent.index;
        component.center = wallComponent.center;
        component.axisDirection = wallComponent.axisDirection;
        component.normalDirection = wallComponent.axisDirection;
        component.radius = wallComponent.radius;

        return component;
    }

    std::optional<OccQtCore::Feature::HoleEndComponent>
    buildDirectEndComponentFromWallConnection(
        const OccQtCore::GeometryModel& model,
        const OccQtCore::Feature::HoleWallComponent& wallComponent,
        int adjacentFaceIndex,
        int connectionEdgeIndex)
    {
        const auto* faceData = model.faceAt(adjacentFaceIndex);

        if (faceData == nullptr)
        {
            return std::nullopt;
        }

        auto component = makeBaseEndComponentFromWall(wallComponent);

        // 面取り無しOpen端:
        // Wallとの接続Edgeが、隣接Face側でInnerWireに属しているならOpen端。
        if (isConnectionEdgeOnInnerWireOfFace(
                model,
                adjacentFaceIndex,
                connectionEdgeIndex))
        {
            component.endType = OccQtCore::Feature::Hole::EndType::Open;

            // Open先の外部Planeは穴端構成Faceには入れない。
            addUniqueIndex(component.geometryRefs.edgeIndices, connectionEdgeIndex);

            return component;
        }

        // Wall -> Plane
        // InnerWireでなければ平底として扱う。
        if (faceData->info.kind == OccQtCore::SurfaceKind::Plane)
        {
            component.endType = OccQtCore::Feature::Hole::EndType::Bottom;

            addUniqueIndex(component.geometryRefs.faceIndices, adjacentFaceIndex);
            addUniqueIndex(component.geometryRefs.edgeIndices, connectionEdgeIndex);

            return component;
        }

        return std::nullopt;
    }

    std::optional<OccQtCore::Feature::HoleEndComponent>
    buildEndComponentThroughTransitionSurface(
        const OccQtCore::GeometryModel& model,
        const OccQtCore::Feature::HoleWallComponent& wallComponent,
        int transitionFaceIndex,
        int wallConnectionEdgeIndex)
    {
        const auto* transitionFaceData = model.faceAt(transitionFaceIndex);

        if (transitionFaceData == nullptr)
        {
            return std::nullopt;
        }

        if (!isTransitionSurface(transitionFaceData->info.kind))
        {
            return std::nullopt;
        }

        const auto& graph = model.graph();
        const auto& faces = model.faces();

        const auto wireIndices = graph.wiresOfFace(transitionFaceIndex);

        for (int wireIndex : wireIndices)
        {
            const auto edgeIndices = graph.edgesOfWire(wireIndex);

            for (int nextEdgeIndex : edgeIndices)
            {
                if (nextEdgeIndex == wallConnectionEdgeIndex)
                {
                    continue;
                }

                const auto connectedFaceIndices = graph.facesOfEdge(nextEdgeIndex);

                for (int nextFaceIndex : connectedFaceIndices)
                {
                    if (nextFaceIndex == transitionFaceIndex)
                    {
                        continue;
                    }

                    if (nextFaceIndex < 0 ||
                        nextFaceIndex >= static_cast<int>(faces.size()))
                    {
                        continue;
                    }

                    const auto& nextFace = faces[nextFaceIndex];

                    auto component = makeBaseEndComponentFromWall(wallComponent);

                    // 遷移面は穴端コンポーネントの構成Faceに含める。
                    addUniqueIndex(component.geometryRefs.faceIndices, transitionFaceIndex);
                    addUniqueIndex(component.geometryRefs.edgeIndices, wallConnectionEdgeIndex);
                    addUniqueIndex(component.geometryRefs.edgeIndices, nextEdgeIndex);

                    // Wall -> Cone/Torus -> 外部PlaneのInnerWire
                    // 面取り/R付きOpen端。
                    if (isConnectionEdgeOnInnerWireOfFace(
                            model,
                            nextFaceIndex,
                            nextEdgeIndex))
                    {
                        component.endType = OccQtCore::Feature::Hole::EndType::Open;

                        // Open先の外部Planeは構成Faceには入れない。
                        return component;
                    }

                    // Wall -> Cone/Torus -> Plane
                    // 面取り/R付きBottom。
                    if (nextFace.info.kind == OccQtCore::SurfaceKind::Plane)
                    {
                        component.endType = OccQtCore::Feature::Hole::EndType::Bottom;

                        addUniqueIndex(component.geometryRefs.faceIndices, nextFaceIndex);

                        return component;
                    }

                    // Wall -> Cone/Torus -> Cylinder
                    // 別径Wall / Step候補。
                    // Stepはこの段階ではまだ確定しない。
                }
            }
        }

        // Wall -> Cone で、その先に有効な接続が見つからない場合。
        // 普通のドリル底の円錐面として Bottom 扱いする。
        if (transitionFaceData->info.kind == OccQtCore::SurfaceKind::Cone)
        {
            auto component = makeBaseEndComponentFromWall(wallComponent);

            component.endType = OccQtCore::Feature::Hole::EndType::Bottom;

            addUniqueIndex(component.geometryRefs.faceIndices, transitionFaceIndex);
            addUniqueIndex(component.geometryRefs.edgeIndices, wallConnectionEdgeIndex);

            return component;
        }

        // Torus単独終端は怪しいので、今はUnknown扱い。
        return std::nullopt;
    }

    std::optional<OccQtCore::Feature::HoleEndComponent>
    buildEndComponentFromWallConnection(
        const OccQtCore::GeometryModel& model,
        const OccQtCore::Feature::HoleWallComponent& wallComponent,
        int adjacentFaceIndex,
        int connectionEdgeIndex)
    {
        const auto* faceData = model.faceAt(adjacentFaceIndex);

        if (faceData == nullptr)
        {
            return std::nullopt;
        }

        if (isTransitionSurface(faceData->info.kind))
        {
            return buildEndComponentThroughTransitionSurface(
                model,
                wallComponent,
                adjacentFaceIndex,
                connectionEdgeIndex);
        }

        return buildDirectEndComponentFromWallConnection(
            model,
            wallComponent,
            adjacentFaceIndex,
            connectionEdgeIndex);
    }
}

namespace OccQtCore::Feature
{

    std::vector<Hole::Data> HoleFeatureRecognizer::recognize(const GeometryModel& model) const
    {
        const auto wallCandidates = detectWallCandidates(model);
        const auto wallComponents = buildWallComponents(model, wallCandidates);
        const auto endComponents = buildEndComponentsFromWallComponents(model, wallComponents);
        const auto holeElements =
            buildHoleElements(model, wallComponents, endComponents);

        (void)holeElements;

        return {};
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
            candidate.index = static_cast<int>(candidates.size());
            candidate.faceIndex = face.index;
            candidate.center = cylinder.axis.Location();
            candidate.axisDirection = cylinder.axis.Direction();
            candidate.radius = cylinder.radius;

            candidates.push_back(candidate);
        }

        return candidates;
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

            component.index = static_cast<int>(components.size());
            components.push_back(component);
        }

        return components;
    }

    std::vector<HoleEndComponent> HoleFeatureRecognizer::buildEndComponentsFromWallComponents(
        const GeometryModel& model,
        const std::vector<HoleWallComponent>& wallComponents) const
    {
        std::vector<HoleEndComponent> components;
        std::set<HoleEndComponentKey> usedEndComponentKeys;

        const auto& graph = model.graph();
        const auto& faces = model.faces();

        for (const auto& wallComponent : wallComponents)
        {
            for (int wallFaceIndex : wallComponent.geometryRefs.faceIndices)
            {
                if (wallFaceIndex < 0 ||
                    wallFaceIndex >= static_cast<int>(faces.size()))
                {
                    continue;
                }

                const auto wireIndices = graph.wiresOfFace(wallFaceIndex);

                for (int wireIndex : wireIndices)
                {
                    const auto edgeIndices = graph.edgesOfWire(wireIndex);

                    for (int edgeIndex : edgeIndices)
                    {
                        const auto connectedFaceIndices = graph.facesOfEdge(edgeIndex);

                        for (int adjacentFaceIndex : connectedFaceIndices)
                        {
                            if (adjacentFaceIndex == wallFaceIndex)
                            {
                                continue;
                            }

                            if (adjacentFaceIndex < 0 ||
                                adjacentFaceIndex >= static_cast<int>(faces.size()))
                            {
                                continue;
                            }

                            // 同じWallComponent内の別Cylinder Faceなら端ではない。
                            if (containsIndex(
                                    wallComponent.geometryRefs.faceIndices,
                                    adjacentFaceIndex))
                            {
                                continue;
                            }

                            const auto componentOpt =
                                buildEndComponentFromWallConnection(
                                    model,
                                    wallComponent,
                                    adjacentFaceIndex,
                                    edgeIndex);

                            if (!componentOpt.has_value())
                            {
                                continue;
                            }

                            HoleEndComponent component = componentOpt.value();
                            component.index = static_cast<int>(components.size());

                            const auto key = makeHoleEndComponentKey(component);

                            if (usedEndComponentKeys.find(key) != usedEndComponentKeys.end())
                            {
                                continue;
                            }

                            usedEndComponentKeys.insert(key);
                            components.push_back(component);
                        }
                    }
                }
            }
        }

        return components;
    }

    std::vector<HoleElement> HoleFeatureRecognizer::buildHoleElements(
        const GeometryModel& model,
        const std::vector<HoleWallComponent>& wallComponents,
        const std::vector<HoleEndComponent>& endComponents) const
    {
        (void)model;

        std::vector<HoleElement> elements;

        for (const auto& wallComponent : wallComponents)
        {
            HoleElement element;

            element.index = static_cast<int>(elements.size());
            element.wallComponentIndex = wallComponent.index;

            int openCount = 0;
            int bottomCount = 0;

            for (const auto& endComponent : endComponents)
            {
                if (endComponent.wallComponentIndex != wallComponent.index)
                {
                    continue;
                }

                addUniqueIndex(element.endComponentIndices, endComponent.index);

                if (endComponent.endType == Hole::EndType::Open)
                {
                    ++openCount;
                }
                else if (endComponent.endType == Hole::EndType::Bottom)
                {
                    ++bottomCount;
                }
            }

            if (element.endComponentIndices.empty())
            {
                continue;
            }

            if (openCount == 2 && bottomCount == 0)
            {
                element.type = Hole::Type::SimpleThrough;
            }
            else if (openCount == 1 && bottomCount == 1)
            {
                element.type = Hole::Type::SimpleBlind;
            }
            else
            {
                element.type = Hole::Type::Unknown;
            }

            elements.push_back(element);
        }

        return elements;
    }
}
