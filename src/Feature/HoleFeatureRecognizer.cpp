#include <cmath>
#include <set>
#include <tuple>

#include <TopoDS.hxx>

#include "Feature/HoleFeatureRecognizer.h"
#include "Core/CollectionUtil.h"
#include "Geometry/GeometryModel.h"
#include "Geometry/TopologyQuery.h"
#include "Geometry/SurfaceUtil.h"

namespace
{
    constexpr double Pi = 3.14159265358979323846;
    constexpr double RadiusTolerance = 1.0e-4;
    constexpr double AxisLineTolerance = 1.0e-4;
    constexpr double DirectionTolerance = 1.0e-6;
    constexpr double AngleTolerance = 1.0e-3;

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

        OccQtCore::CollectionUtil::sortUnique(key.faceIndices);

        // 面取りなしOpenは faceIndices が空になる。
        // 貫通穴の両端Openを同一扱いで潰さないため、
        // faceIndices が空のOpen端だけ edgeIndices をキーに含める。
        if (component.endType == OccQtCore::Feature::Hole::EndType::Open &&
            component.geometryRefs.faceIndices.empty())
        {
            key.edgeIndicesForFaceLessOpen = component.geometryRefs.edgeIndices;
            OccQtCore::CollectionUtil::sortUnique(key.edgeIndicesForFaceLessOpen);
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

    bool isSameHoleAxis(
        const OccQtCore::Feature::HoleWallComponent& lhs,
        const OccQtCore::Feature::HoleWallComponent& rhs)
    {
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
        const auto lhsEdges =
            OccQtCore::TopologyQuery::edgesOfFace(model, lhsFaceIndex);

        const auto rhsEdges =
            OccQtCore::TopologyQuery::edgesOfFace(model, rhsFaceIndex);

        for (int edgeIndex : lhsEdges)
        {
            if (OccQtCore::CollectionUtil::contains(rhsEdges, edgeIndex))
            {
                return true;
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
        const auto& cylinder = faceData->info.cylinder.value();

        return OccQtCore::SurfaceUtil::isCylinderFaceInwardOriented(
            face,
            cylinder.axis,
            faceData->info.uMin,
            faceData->info.uMax,
            faceData->info.vMin,
            faceData->info.vMax);
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
        if (!OccQtCore::TopologyQuery::isValidFaceIndex(model, faceIndex) ||
            !OccQtCore::TopologyQuery::isValidEdgeIndex(model, edgeIndex))
        {
            return false;
        }

        const auto& graph = model.graph();
        const auto& wires = model.wires();

        for (int wireIndex : graph.wiresOfFace(faceIndex))
        {
            if (!OccQtCore::TopologyQuery::isValidWireIndex(model, wireIndex))
            {
                continue;
            }

            const auto edgeIndices = graph.edgesOfWire(wireIndex);

            if (!OccQtCore::CollectionUtil::contains(edgeIndices, edgeIndex))
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

    const OccQtCore::Feature::HoleEndComponent* endComponentByIndex(
        const std::vector<OccQtCore::Feature::HoleEndComponent>& endComponents,
        int endComponentIndex)
    {
        if (endComponentIndex < 0 ||
            endComponentIndex >= static_cast<int>(endComponents.size()))
        {
            return nullptr;
        }

        return &endComponents[endComponentIndex];
    }

    const OccQtCore::Feature::HoleWallComponent* wallComponentOfElement(
        const std::vector<OccQtCore::Feature::HoleWallComponent>& wallComponents,
        const OccQtCore::Feature::HoleElement& element)
    {
        if (element.wallComponentIndex < 0 ||
            element.wallComponentIndex >= static_cast<int>(wallComponents.size()))
        {
            return nullptr;
        }

        return &wallComponents[element.wallComponentIndex];
    }

    bool faceContainsAnyEdge(
        const OccQtCore::GeometryModel& model,
        int faceIndex,
        const std::vector<int>& edgeIndices)
    {
        if (!OccQtCore::TopologyQuery::isValidFaceIndex(model, faceIndex))
        {
            return false;
        }

        const auto faceEdges =
            OccQtCore::TopologyQuery::edgesOfFace(model, faceIndex);

        for (int edgeIndex : edgeIndices)
        {
            if (OccQtCore::CollectionUtil::contains(faceEdges, edgeIndex))
            {
                return true;
            }
        }

        return false;
    }

    bool bottomEndFacesContainOpenEndEdges(
        const OccQtCore::GeometryModel& model,
        const OccQtCore::Feature::HoleEndComponent& bottomEnd,
        const OccQtCore::Feature::HoleEndComponent& openEnd)
    {
        for (int bottomFaceIndex : bottomEnd.geometryRefs.faceIndices)
        {
            if (faceContainsAnyEdge(
                    model,
                    bottomFaceIndex,
                    openEnd.geometryRefs.edgeIndices))
            {
                return true;
            }
        }

        return false;
    }

    bool bottomEndFacesAdjacentToOpenWallFaces(
        const OccQtCore::GeometryModel& model,
        const OccQtCore::Feature::HoleEndComponent& bottomEnd,
        const OccQtCore::Feature::HoleWallComponent& openWall)
    {
        for (int bottomFaceIndex : bottomEnd.geometryRefs.faceIndices)
        {
            if (!OccQtCore::TopologyQuery::isValidFaceIndex(model, bottomFaceIndex))
            {
                continue;
            }

            const auto adjacentFaces =
                OccQtCore::TopologyQuery::adjacentFacesOfFace(
                    model,
                    bottomFaceIndex);

            for (int wallFaceIndex : openWall.geometryRefs.faceIndices)
            {
                if (OccQtCore::CollectionUtil::contains(
                        adjacentFaces,
                        wallFaceIndex))
                {
                    return true;
                }
            }
        }

        return false;
    }

    OccQtCore::Feature::HoleElementStepConnectionDirection stepConnectionDirection(
        double bottomRadius,
        double openRadius)
    {
        using Direction = OccQtCore::Feature::HoleElementStepConnectionDirection;

        if (isSameRadius(bottomRadius, openRadius))
        {
            return Direction::Unknown;
        }

        return bottomRadius > openRadius
            ? Direction::LargerToSmaller
            : Direction::SmallerToLarger;
    }

    std::optional<OccQtCore::Feature::HoleElementStepConnection>
    findBottomToOpenStepConnection(
        const OccQtCore::GeometryModel& model,
        int candidateIndex,
        const OccQtCore::Feature::HoleElement& bottomOwnerElement,
        const OccQtCore::Feature::HoleElement& openOwnerElement,
        const std::vector<OccQtCore::Feature::HoleWallComponent>& wallComponents,
        const std::vector<OccQtCore::Feature::HoleEndComponent>& endComponents)
    {
        const auto* bottomWall =
            wallComponentOfElement(
                wallComponents,
                bottomOwnerElement);

        const auto* openWall =
            wallComponentOfElement(
                wallComponents,
                openOwnerElement);

        if (bottomWall == nullptr || openWall == nullptr)
        {
            return std::nullopt;
        }

        if (isSameRadius(bottomWall->radius, openWall->radius))
        {
            return std::nullopt;
        }

        for (int bottomEndIndex : bottomOwnerElement.endComponentIndices)
        {
            const auto* bottomEnd =
                endComponentByIndex(
                    endComponents,
                    bottomEndIndex);

            if (bottomEnd == nullptr)
            {
                continue;
            }

            if (bottomEnd->endType != OccQtCore::Feature::Hole::EndType::Bottom)
            {
                continue;
            }

            for (int openEndIndex : openOwnerElement.endComponentIndices)
            {
                const auto* openEnd =
                    endComponentByIndex(
                        endComponents,
                        openEndIndex);

                if (openEnd == nullptr)
                {
                    continue;
                }

                if (openEnd->endType != OccQtCore::Feature::Hole::EndType::Open)
                {
                    continue;
                }

                if (bottomEndFacesContainOpenEndEdges(
                        model,
                        *bottomEnd,
                        *openEnd))
                {
                    OccQtCore::Feature::HoleElementStepConnection connection;

                    connection.candidateIndex = candidateIndex;
                    connection.bottomElementIndex = bottomOwnerElement.index;
                    connection.openElementIndex = openOwnerElement.index;
                    connection.bottomEndComponentIndex = bottomEndIndex;
                    connection.openEndComponentIndex = openEndIndex;
                    connection.bottomElementRadius = bottomWall->radius;
                    connection.openElementRadius = openWall->radius;
                    connection.direction =
                        stepConnectionDirection(
                            bottomWall->radius,
                            openWall->radius);
                    connection.reason =
                        OccQtCore::Feature::HoleElementStepConnectionReason::
                        BottomFaceContainsOpenEdge;

                    return connection;
                }

                if (bottomEndFacesAdjacentToOpenWallFaces(
                        model,
                        *bottomEnd,
                        *openWall))
                {
                    OccQtCore::Feature::HoleElementStepConnection connection;

                    connection.candidateIndex = candidateIndex;
                    connection.bottomElementIndex = bottomOwnerElement.index;
                    connection.openElementIndex = openOwnerElement.index;
                    connection.bottomEndComponentIndex = bottomEndIndex;
                    connection.openEndComponentIndex = openEndIndex;
                    connection.bottomElementRadius = bottomWall->radius;
                    connection.openElementRadius = openWall->radius;
                    connection.direction =
                        stepConnectionDirection(
                            bottomWall->radius,
                            openWall->radius);
                    connection.reason =
                        OccQtCore::Feature::HoleElementStepConnectionReason::
                        BottomEndFaceAdjacentToOpenWall;

                    return connection;
                }
            }
        }
        return std::nullopt;
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
            OccQtCore::CollectionUtil::addUnique(component.geometryRefs.edgeIndices, connectionEdgeIndex);

            return component;
        }

        // Wall -> Plane
        // InnerWireでなければ平底として扱う。
        if (faceData->info.kind == OccQtCore::SurfaceKind::Plane)
        {
            component.endType = OccQtCore::Feature::Hole::EndType::Bottom;

            OccQtCore::CollectionUtil::addUnique(component.geometryRefs.faceIndices, adjacentFaceIndex);
            OccQtCore::CollectionUtil::addUnique(component.geometryRefs.edgeIndices, connectionEdgeIndex);

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

        const auto transitionEdgeIndices =
            OccQtCore::TopologyQuery::edgesOfFace(
                model,
                transitionFaceIndex);

        for (int nextEdgeIndex : transitionEdgeIndices)
        {
            if (nextEdgeIndex == wallConnectionEdgeIndex)
            {
                continue;
            }

            const auto nextFaceIndices =
                OccQtCore::TopologyQuery::adjacentFacesOfEdge(
                    model,
                    nextEdgeIndex,
                    transitionFaceIndex);

            for (int nextFaceIndex : nextFaceIndices)
            {
                const auto* nextFaceData = model.faceAt(nextFaceIndex);

                if (nextFaceData == nullptr)
                {
                    continue;
                }

                auto component = makeBaseEndComponentFromWall(wallComponent);

                // 遷移面は穴端コンポーネントの構成Faceに含める。
                OccQtCore::CollectionUtil::addUnique(
                    component.geometryRefs.faceIndices,
                    transitionFaceIndex);

                OccQtCore::CollectionUtil::addUnique(
                    component.geometryRefs.edgeIndices,
                    wallConnectionEdgeIndex);

                OccQtCore::CollectionUtil::addUnique(
                    component.geometryRefs.edgeIndices,
                    nextEdgeIndex);

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
                if (nextFaceData->info.kind == OccQtCore::SurfaceKind::Plane)
                {
                    component.endType = OccQtCore::Feature::Hole::EndType::Bottom;

                    OccQtCore::CollectionUtil::addUnique(
                        component.geometryRefs.faceIndices,
                        nextFaceIndex);

                    return component;
                }

                // Wall -> Cone/Torus -> Cylinder
                // 別径Wall / Step候補。
                // Stepはこの段階ではまだ確定しない。
            }
        }

        // Wall -> Cone で、その先に有効な接続が見つからない場合。
        // 普通のドリル底の円錐面として Bottom 扱いする。
        if (transitionFaceData->info.kind == OccQtCore::SurfaceKind::Cone)
        {
            auto component = makeBaseEndComponentFromWall(wallComponent);

            component.endType = OccQtCore::Feature::Hole::EndType::Bottom;

            OccQtCore::CollectionUtil::addUnique(
                component.geometryRefs.faceIndices,
                transitionFaceIndex);

            OccQtCore::CollectionUtil::addUnique(
                component.geometryRefs.edgeIndices,
                wallConnectionEdgeIndex);

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
                OccQtCore::CollectionUtil::addUnique(component.geometryRefs.faceIndices, faceIndex);
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

        for (const auto& wallComponent : wallComponents)
        {
            const auto connections =
                TopologyQuery::collectBoundaryConnectionsOfFaceGroup(
                    model,
                    wallComponent.geometryRefs.faceIndices);

            for (const auto& connection : connections)
            {
                const auto componentOpt =
                    buildEndComponentFromWallConnection(
                        model,
                        wallComponent,
                        connection.adjacentFaceIndex,
                        connection.boundaryEdgeIndex);

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

                OccQtCore::CollectionUtil::addUnique(element.endComponentIndices, endComponent.index);

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

    std::vector<HoleCandidate> HoleFeatureRecognizer::buildHoleCandidatesFromElements(
        const std::vector<HoleWallComponent>& wallComponents,
        const std::vector<HoleElement>& holeElements) const
    {
        std::vector<HoleCandidate> candidates;
        std::vector<bool> used(holeElements.size(), false);

        for (int baseElementArrayIndex = 0;
             baseElementArrayIndex < static_cast<int>(holeElements.size());
             ++baseElementArrayIndex)
        {
            if (used[baseElementArrayIndex])
            {
                continue;
            }

            const auto& baseElement = holeElements[baseElementArrayIndex];

            const auto* baseWall =
                wallComponentOfElement(
                    wallComponents,
                    baseElement);

            if (baseWall == nullptr)
            {
                continue;
            }

            HoleCandidate candidate;
            candidate.index = static_cast<int>(candidates.size());

            OccQtCore::CollectionUtil::addUnique(
                candidate.elementIndices,
                baseElement.index);

            used[baseElementArrayIndex] = true;

            for (int nextElementArrayIndex = baseElementArrayIndex + 1;
                 nextElementArrayIndex < static_cast<int>(holeElements.size());
                 ++nextElementArrayIndex)
            {
                if (used[nextElementArrayIndex])
                {
                    continue;
                }

                const auto& nextElement = holeElements[nextElementArrayIndex];

                const auto* nextWall =
                    wallComponentOfElement(
                        wallComponents,
                        nextElement);

                if (nextWall == nullptr)
                {
                    continue;
                }

                if (!isSameHoleAxis(*baseWall, *nextWall))
                {
                    continue;
                }

                OccQtCore::CollectionUtil::addUnique(
                    candidate.elementIndices,
                    nextElement.index);

                used[nextElementArrayIndex] = true;
            }

            candidates.push_back(candidate);
        }

        return candidates;
    }

    std::vector<HoleElementStepConnection>
    HoleFeatureRecognizer::buildStepConnectionsInHoleCandidates(
        const GeometryModel& model,
        const std::vector<HoleCandidate>& candidates,
        const std::vector<HoleElement>& elements,
        const std::vector<HoleWallComponent>& wallComponents,
        const std::vector<HoleEndComponent>& endComponents) const
    {
        std::vector<HoleElementStepConnection> connections;

        for (const auto& candidate : candidates)
        {
            for (int i = 0;
                 i < static_cast<int>(candidate.elementIndices.size());
                 ++i)
            {
                const int firstElementIndex = candidate.elementIndices[i];

                if (firstElementIndex < 0 ||
                    firstElementIndex >= static_cast<int>(elements.size()))
                {
                    continue;
                }

                const auto& firstElement = elements[firstElementIndex];

                for (int j = i + 1;
                     j < static_cast<int>(candidate.elementIndices.size());
                     ++j)
                {
                    const int secondElementIndex = candidate.elementIndices[j];

                    if (secondElementIndex < 0 ||
                        secondElementIndex >= static_cast<int>(elements.size()))
                    {
                        continue;
                    }

                    const auto& secondElement = elements[secondElementIndex];

                    const auto firstToSecond =
                        findBottomToOpenStepConnection(
                            model,
                            candidate.index,
                            firstElement,
                            secondElement,
                            wallComponents,
                            endComponents);

                    if (firstToSecond.has_value())
                    {
                        connections.push_back(firstToSecond.value());
                    }

                    const auto secondToFirst =
                        findBottomToOpenStepConnection(
                            model,
                            candidate.index,
                            secondElement,
                            firstElement,
                            wallComponents,
                            endComponents);

                    if (secondToFirst.has_value())
                    {
                        connections.push_back(secondToFirst.value());
                    }
                }
            }
        }

        return connections;
    }
}
