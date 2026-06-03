#include "Feature/HoleRecognitionUtil.h"

#include <algorithm>
#include <cmath>
#include <tuple>

#include <TopoDS.hxx>

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
    constexpr double AxialPositionTolerance = 1.0e-4;
    constexpr double AxialRangeGapTolerance = 0.6;

    bool containsIndex(
        const std::vector<int>& indices,
        int target)
    {
        return std::find(
                   indices.begin(),
                   indices.end(),
                   target) != indices.end();
    }

    bool hasSharedIndex(
        const std::vector<int>& lhs,
        const std::vector<int>& rhs)
    {
        for (const int index : lhs)
        {
            if (containsIndex(rhs, index))
            {
                return true;
            }
        }

        return false;
    }

    bool isValidEndIndex(
        int index,
        const std::vector<OccQtCore::Feature::HoleEndCandidate>& endCandidates)
    {
        return index >= 0 &&
               index < static_cast<int>(endCandidates.size());
    }

    /**
     * @brief HoleEndCandidate の重複除外用キー。
     *
     * 通常は wallCandidateIndex + faceIndices + endType で同一端候補を判定する。
     *
     * ただし、面取りなしOpen端では faceIndices が空になる。
     * そのままでは貫通穴の両端Openが同一扱いで潰れてしまうため、
     * faceIndices が空のOpen端のみ edgeIndices もキーに含める。
     */
    struct HoleEndCandidateKey
    {
        int wallCandidateIndex = -1;
        std::vector<int> faceIndices;

        int axialPositionKey = 0;
        bool hasAxialPositionKey = false;

        OccQtCore::Feature::HoleEndCandidateType type =
            OccQtCore::Feature::HoleEndCandidateType::Unknown;

        bool operator<(const HoleEndCandidateKey& other) const
        {
            return std::tie(
                       wallCandidateIndex,
                       faceIndices,
                       axialPositionKey,
                       hasAxialPositionKey,
                       type)
                   < std::tie(
                       other.wallCandidateIndex,
                       other.faceIndices,
                       other.axialPositionKey,
                       other.hasAxialPositionKey,
                       other.type);
        }

        bool operator==(const HoleEndCandidateKey& other) const
        {
            return wallCandidateIndex == other.wallCandidateIndex &&
                   faceIndices == other.faceIndices &&
                   axialPositionKey == other.axialPositionKey &&
                   hasAxialPositionKey == other.hasAxialPositionKey &&
                   type == other.type;
        }
    };

    HoleEndCandidateKey makeHoleEndCandidateKey(
        const OccQtCore::Feature::HoleEndCandidate& candidate)
    {
        HoleEndCandidateKey key;

        key.wallCandidateIndex = candidate.wallCandidateIndex;
        key.faceIndices = candidate.geometryRefs.faceIndices;
        key.type = candidate.type;

        OccQtCore::CollectionUtil::sortUnique(key.faceIndices);

        if (candidate.hasAxialPosition)
        {
            key.axialPositionKey =
                static_cast<int>(
                    std::round(
                        candidate.axialPosition / AxialPositionTolerance));

            key.hasAxialPositionKey = true;
        }

        return key;
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

        return OccQtCore::SurfaceUtil::parameterSpan(
            faceData->info.uMin,
            faceData->info.uMax);
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

    /**
     * @brief 指定Faceがsource以外のHoleWallCandidateに属するか判定する。
     *
     * 端探索中に別の穴壁候補へ到達した場合、
     * その先へ深追いすると奥のOpen/Bottomを誤って拾う可能性がある。
     *
     * ただし、この段階ではStep確定ではない。
     * 別Wallに当たった事実だけを WallConnection として保持し、
     * Stepかどうかは後続のセグメント接続判定で決める。
     */
    bool isFaceOwnedByOtherWallCandidate(
        int faceIndex,
        int sourceWallCandidateIndex,
        const std::vector<OccQtCore::Feature::HoleWallCandidate>& wallCandidates)
    {
        for (const auto& wallCandidate : wallCandidates)
        {
            if (wallCandidate.index == sourceWallCandidateIndex)
            {
                continue;
            }

            if (OccQtCore::CollectionUtil::contains(wallCandidate.geometryRefs.faceIndices, faceIndex))
            {
                return true;
            }
        }

        return false;
    }

    /**
     * @brief 接続Edgeが指定Faceの閉じたInnerWire上にあるかを判定する。
     *
     * 穴壁から隣接Faceへ接続している場合、その接続Edgeが隣接Face側で
     * InnerWireに属していれば、外部に開いたOpen端として扱う。
     *
     * 例:
     * - Wall -> 外部Plane の InnerWire: 面取りなしOpen端
     * - Wall -> Cone/Torus -> 外部Plane の InnerWire: 面取り/R付きOpen端
     */
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

    /**
     * @brief 穴壁と端面の間に挟まる遷移面かを判定する。
     *
     * 現時点では、面取りを Cone、Rを Torus として扱う。
     */
    bool isTransitionSurface(
        OccQtCore::SurfaceKind kind)
    {
        return kind == OccQtCore::SurfaceKind::Cone ||
               kind == OccQtCore::SurfaceKind::Torus;
    }

    /**
     * @brief 穴壁候補から穴端候補の共通情報を初期化する。
     *
     * 端種別や構成ジオメトリは接続先Faceの判定後に設定する。
     */
    OccQtCore::Feature::HoleEndCandidate makeBaseEndCandidateFromWall(
        const OccQtCore::Feature::HoleWallCandidate& wallCandidate)
    {
        OccQtCore::Feature::HoleEndCandidate candidate;

        candidate.wallCandidateIndex = wallCandidate.index;
        candidate.center = wallCandidate.center;
        candidate.axisDirection = wallCandidate.axisDirection;
        candidate.normalDirection = wallCandidate.axisDirection;
        candidate.radius = wallCandidate.radius;

        return candidate;
    }

    /**
     * @brief 別の穴壁候補へ接続する端候補を生成する。
     *
     * この段階ではStepとは確定しない。
     * 後続のセグメント接続判定で、同軸・別径・接続関係を見て
     * 段付き穴のStepとして扱えるかを判断する。
     */
    OccQtCore::Feature::HoleEndCandidate makeWallConnectionEndCandidateFromWall(
        const OccQtCore::Feature::HoleWallCandidate& wallCandidate,
        int connectionEdgeIndex,
        int connectedFaceIndex)
    {
        auto candidate = makeBaseEndCandidateFromWall(wallCandidate);

        candidate.type =
            OccQtCore::Feature::HoleEndCandidateType::WallConnection;

        OccQtCore::CollectionUtil::addUnique(
            candidate.geometryRefs.edgeIndices,
            connectionEdgeIndex);

        OccQtCore::CollectionUtil::addUnique(
            candidate.geometryRefs.faceIndices,
            connectedFaceIndex);

        return candidate;
    }

    void setAxialPositionFromEdge(
        const OccQtCore::GeometryModel& model,
        const OccQtCore::Feature::HoleWallCandidate& wallCandidate,
        int edgeIndex,
        OccQtCore::Feature::HoleEndCandidate& candidate)
    {
        const auto* edgeData = model.edgeAt(edgeIndex);

        if (edgeData == nullptr)
        {
            return;
        }
        if (edgeData->info.kind != OccQtCore::CurveKind::Circle ||
            !edgeData->info.circle.has_value())
        {
            return;
        }

        const auto& circle = edgeData->info.circle.value();

        candidate.center = circle.center;

        const auto vectorFromWallCenter =
            circle.center.XYZ() - wallCandidate.center.XYZ();

        candidate.axialPosition =
            vectorFromWallCenter.Dot(wallCandidate.axisDirection.XYZ());

        candidate.hasAxialPosition = true;
    }

    void setAxialPositionFromAnyEdge(
        const OccQtCore::GeometryModel& model,
        const OccQtCore::Feature::HoleWallCandidate& wallCandidate,
        OccQtCore::Feature::HoleEndCandidate& candidate)
    {
        if (candidate.hasAxialPosition)
        {
            return;
        }

        for (int edgeIndex : candidate.geometryRefs.edgeIndices)
        {
            setAxialPositionFromEdge(
                model,
                wallCandidate,
                edgeIndex,
                candidate);

            if (candidate.hasAxialPosition)
            {
                return;
            }
        }
    }

    /**
     * @brief 遷移面を経由しない穴端候補を生成する。
     *
     * Wall -> 別Wall は WallConnection端、
     * Wall -> 外部FaceのInnerWire は面取りなしOpen端、
     * Wall -> その他の閉じたFace は Bottom端として扱う。
     */
    std::optional<OccQtCore::Feature::HoleEndCandidate>
    buildDirectEndCandidateFromWallConnection(
        const OccQtCore::GeometryModel& model,
        const OccQtCore::Feature::HoleWallCandidate& sourceWallCandidate,
        const std::vector<OccQtCore::Feature::HoleWallCandidate>& allWallCandidates,
        int adjacentFaceIndex,
        int connectionEdgeIndex)
    {
        const auto* faceData = model.faceAt(adjacentFaceIndex);

        if (faceData == nullptr)
        {
            return std::nullopt;
        }

        if (isFaceOwnedByOtherWallCandidate(
                adjacentFaceIndex,
                sourceWallCandidate.index,
                allWallCandidates))
        {
            return makeWallConnectionEndCandidateFromWall(
                sourceWallCandidate,
                connectionEdgeIndex,
                adjacentFaceIndex);
        }

        auto candidate = makeBaseEndCandidateFromWall(sourceWallCandidate);

        if (isConnectionEdgeOnInnerWireOfFace(
                model,
                adjacentFaceIndex,
                connectionEdgeIndex))
        {
            candidate.type =
                OccQtCore::Feature::HoleEndCandidateType::Open;

            OccQtCore::CollectionUtil::addUnique(
                candidate.geometryRefs.edgeIndices,
                connectionEdgeIndex);

            setAxialPositionFromAnyEdge(
                model,
                sourceWallCandidate,
                candidate);

            return candidate;
        }

        candidate.type =
            OccQtCore::Feature::HoleEndCandidateType::Bottom;

        OccQtCore::CollectionUtil::addUnique(
            candidate.geometryRefs.faceIndices,
            adjacentFaceIndex);

        OccQtCore::CollectionUtil::addUnique(
            candidate.geometryRefs.edgeIndices,
            connectionEdgeIndex);

        setAxialPositionFromAnyEdge(
            model,
            sourceWallCandidate,
            candidate);

        return candidate;
    }

    /**
     * @brief Cone/Torusなどの遷移面を経由する穴端候補を生成する。
     *
     * Wall -> Cone/Torus -> 別Wall は WallConnection端、
     * Wall -> Cone/Torus -> 外部FaceのInnerWire は面取り/R付きOpen端、
     * Wall -> Cone/Torus -> その他の閉じたFace は Bottom端として扱う。
     *
     * 別Wallへ到達した場合は、その先のOpen/Bottomを拾わないように即停止する。
     */
    std::optional<OccQtCore::Feature::HoleEndCandidate>
    buildEndCandidateThroughTransitionSurface(
        const OccQtCore::GeometryModel& model,
        const OccQtCore::Feature::HoleWallCandidate& sourceWallCandidate,
        const std::vector<OccQtCore::Feature::HoleWallCandidate>& allWallCandidates,
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

        bool foundNextConnection = false;

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

                foundNextConnection = true;

                auto candidate =
                    makeBaseEndCandidateFromWall(sourceWallCandidate);

                OccQtCore::CollectionUtil::addUnique(
                    candidate.geometryRefs.faceIndices,
                    transitionFaceIndex);

                OccQtCore::CollectionUtil::addUnique(
                    candidate.geometryRefs.edgeIndices,
                    wallConnectionEdgeIndex);

                OccQtCore::CollectionUtil::addUnique(
                    candidate.geometryRefs.edgeIndices,
                    nextEdgeIndex);

                if (isFaceOwnedByOtherWallCandidate(
                        nextFaceIndex,
                        sourceWallCandidate.index,
                        allWallCandidates))
                {
                    candidate.type =
                        OccQtCore::Feature::HoleEndCandidateType::WallConnection;

                    OccQtCore::CollectionUtil::addUnique(
                        candidate.geometryRefs.faceIndices,
                        nextFaceIndex);

                    return candidate;
                }

                if (isConnectionEdgeOnInnerWireOfFace(
                        model,
                        nextFaceIndex,
                        nextEdgeIndex))
                {
                    candidate.type =
                        OccQtCore::Feature::HoleEndCandidateType::Open;

                    setAxialPositionFromAnyEdge(
                        model,
                        sourceWallCandidate,
                        candidate);

                    return candidate;
                }

                candidate.type =
                    OccQtCore::Feature::HoleEndCandidateType::Bottom;

                OccQtCore::CollectionUtil::addUnique(
                    candidate.geometryRefs.faceIndices,
                    nextFaceIndex);

                setAxialPositionFromAnyEdge(
                    model,
                    sourceWallCandidate,
                    candidate);

                return candidate;
            }
        }

        if (!foundNextConnection &&
            transitionFaceData->info.kind == OccQtCore::SurfaceKind::Cone)
        {
            auto candidate =
                makeBaseEndCandidateFromWall(sourceWallCandidate);

            candidate.type =
                OccQtCore::Feature::HoleEndCandidateType::Bottom;

            OccQtCore::CollectionUtil::addUnique(
                candidate.geometryRefs.faceIndices,
                transitionFaceIndex);

            OccQtCore::CollectionUtil::addUnique(
                candidate.geometryRefs.edgeIndices,
                wallConnectionEdgeIndex);

            setAxialPositionFromAnyEdge(
                model,
                sourceWallCandidate,
                candidate);

            return candidate;
        }

        return std::nullopt;
    }
}

namespace OccQtCore::Feature::HoleRecognitionUtil
{
    void mergeEndCandidateGeometryRefs(
        HoleEndCandidate& dst,
        const HoleEndCandidate& src)
    {
        for (int faceIndex : src.geometryRefs.faceIndices)
        {
            OccQtCore::CollectionUtil::addUnique(
                dst.geometryRefs.faceIndices,
                faceIndex);
        }

        for (int edgeIndex : src.geometryRefs.edgeIndices)
        {
            OccQtCore::CollectionUtil::addUnique(
                dst.geometryRefs.edgeIndices,
                edgeIndex);
        }
    }

    bool isSameCylinderCandidate(
        const HoleWallCandidate& lhs,
        const HoleWallCandidate& rhs)
    {
        return OccQtCore::SurfaceUtil::isSameCylinderAxisAndRadius(
            lhs.center,
            lhs.axisDirection,
            lhs.radius,
            rhs.center,
            rhs.axisDirection,
            rhs.radius,
            RadiusTolerance,
            AxisLineTolerance,
            DirectionTolerance);
    }

    bool isClosedCylinderWallGroup(
        const GeometryModel& model,
        const std::vector<HoleWallCandidate>& wallCandidates,
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

            for (int faceIndex : wallCandidates[candidateIndex].geometryRefs.faceIndices)
            {
                totalUSpan += cylinderFaceUSpan(model, faceIndex);
            }
        }

        return totalUSpan >= 2.0 * Pi - AngleTolerance;
    }

    bool isLikelyHoleWallByOrientation(
        const GeometryModel& model,
        const HoleWallCandidate& candidate)
    {
        int inwardCount = 0;

        for (int faceIndex : candidate.geometryRefs.faceIndices)
        {
            if (isCylinderFaceInwardOriented(model, faceIndex))
            {
                ++inwardCount;
            }
        }

        return inwardCount > 0;
    }

    bool isSameAxisCandidate(
        const HoleWallCandidate& lhs,
        const HoleWallCandidate& rhs)
    {
        return SurfaceUtil::isSameAxis(
            lhs.center,
            lhs.axisDirection,
            rhs.center,
            rhs.axisDirection,
            AxisLineTolerance,
            DirectionTolerance);
    }

    std::optional<HoleEndCandidate> buildEndCandidateFromWallConnection(
        const GeometryModel& model,
        const HoleWallCandidate& sourceWallCandidate,
        const std::vector<HoleWallCandidate>& allWallCandidates,
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
            return buildEndCandidateThroughTransitionSurface(
                model,
                sourceWallCandidate,
                allWallCandidates,
                adjacentFaceIndex,
                connectionEdgeIndex);
        }

        return buildDirectEndCandidateFromWallConnection(
            model,
            sourceWallCandidate,
            allWallCandidates,
            adjacentFaceIndex,
            connectionEdgeIndex);
    }

    bool isWallOnCandidateAxis(
        const HoleCandidateAxis& candidateAxis,
        const HoleWallCandidate& wallCandidate)
    {
        if (!candidateAxis.isValid)
        {
            return false;
        }

        return SurfaceUtil::isSameAxis(
            candidateAxis.point,
            candidateAxis.direction,
            wallCandidate.center,
            wallCandidate.axisDirection,
            AxisLineTolerance,
            DirectionTolerance);
    }

    HoleCandidateAxis buildHoleCandidateAxis(
        const HoleCandidate& candidate,
        const std::vector<HoleSegmentCandidate>& segmentCandidates,
        const std::vector<HoleWallCandidate>& wallCandidates)
    {
        HoleCandidateAxis axis;

        for (const int segmentIndex : candidate.segmentCandidateIndices)
        {
            if (segmentIndex < 0 ||
                segmentIndex >= static_cast<int>(segmentCandidates.size()))
            {
                continue;
            }

            const auto& segment = segmentCandidates[segmentIndex];

            if (segment.wallCandidateIndex < 0 ||
                segment.wallCandidateIndex >= static_cast<int>(wallCandidates.size()))
            {
                continue;
            }

            const auto& wall = wallCandidates[segment.wallCandidateIndex];

            axis.point = wall.center;
            axis.direction = wall.axisDirection;
            axis.isValid = true;
            break;
        }

        if (!axis.isValid)
        {
            return axis;
        }

        for (const int segmentIndex : candidate.segmentCandidateIndices)
        {
            if (segmentIndex < 0 ||
                segmentIndex >= static_cast<int>(segmentCandidates.size()))
            {
                axis.isValid = false;
                return axis;
            }

            const auto& segment = segmentCandidates[segmentIndex];

            if (segment.wallCandidateIndex < 0 ||
                segment.wallCandidateIndex >= static_cast<int>(wallCandidates.size()))
            {
                axis.isValid = false;
                return axis;
            }

            const auto& wall = wallCandidates[segment.wallCandidateIndex];

            if (!isWallOnCandidateAxis(axis, wall))
            {
                axis.isValid = false;
                return axis;
            }
        }

        return axis;
    }

    HoleSegmentRangeOnCandidateAxis buildHoleSegmentRangeOnCandidateAxis(
        const HoleSegmentCandidate& segment,
        const HoleCandidateAxis& candidateAxis,
        const std::vector<HoleEndCandidate>& endCandidates)
    {
        HoleSegmentRangeOnCandidateAxis range;
        range.segmentCandidateIndex = segment.index;

        if (!candidateAxis.isValid)
        {
            return range;
        }

        bool hasValue = false;

        for (const int endIndex : segment.endCandidateIndices)
        {
            if (endIndex < 0 ||
                endIndex >= static_cast<int>(endCandidates.size()))
            {
                continue;
            }

            const auto& end = endCandidates[endIndex];

            const double axial =
                SurfaceUtil::projectPointToAxis(
                    candidateAxis.point,
                    candidateAxis.direction,
                    end.center);

            if (!hasValue)
            {
                range.minAxial = axial;
                range.maxAxial = axial;
                range.minEndCandidateIndex = endIndex;
                range.maxEndCandidateIndex = endIndex;
                hasValue = true;
                continue;
            }

            if (axial < range.minAxial)
            {
                range.minAxial = axial;
                range.minEndCandidateIndex = endIndex;
            }

            if (axial > range.maxAxial)
            {
                range.maxAxial = axial;
                range.maxEndCandidateIndex = endIndex;
            }
        }

        range.isValid = hasValue;
        return range;
    }

    bool isSameEndCandidate(
        const HoleEndCandidate& lhs,
        const HoleEndCandidate& rhs)
    {
        return makeHoleEndCandidateKey(lhs) ==
               makeHoleEndCandidateKey(rhs);
    }

    void appendOrMergeEndCandidate(
        std::vector<HoleEndCandidate>& candidates,
        const HoleEndCandidate& candidate)
    {
        for (auto& existingCandidate : candidates)
        {
            if (isSameEndCandidate(existingCandidate, candidate))
            {
                mergeEndCandidateGeometryRefs(
                    existingCandidate,
                    candidate);

                return;
            }
        }

        candidates.push_back(candidate);
    }

    bool hasSharedEndGeometry(
        const HoleEndCandidate& lhs,
        const HoleEndCandidate& rhs)
    {
        if (hasSharedIndex(
                lhs.geometryRefs.faceIndices,
                rhs.geometryRefs.faceIndices))
        {
            return true;
        }

        if (hasSharedIndex(
                lhs.geometryRefs.edgeIndices,
                rhs.geometryRefs.edgeIndices))
        {
            return true;
        }

        return false;
    }

    HoleSegmentConnectionKind classifyAdjacentSegmentConnection(
        const HoleSegmentRangeOnCandidateAxis& currentRange,
        const HoleSegmentRangeOnCandidateAxis& nextRange,
        const std::vector<HoleEndCandidate>& endCandidates)
    {
        if (!currentRange.isValid || !nextRange.isValid)
        {
            return HoleSegmentConnectionKind::Unknown;
        }

        const int currentEndIndex = currentRange.maxEndCandidateIndex;
        const int nextEndIndex = nextRange.minEndCandidateIndex;

        if (!isValidEndIndex(currentEndIndex, endCandidates) ||
            !isValidEndIndex(nextEndIndex, endCandidates))
        {
            return HoleSegmentConnectionKind::Unknown;
        }

        const auto& currentEnd = endCandidates[currentEndIndex];
        const auto& nextEnd = endCandidates[nextEndIndex];

        if (hasSharedEndGeometry(currentEnd, nextEnd))
        {
            return HoleSegmentConnectionKind::SharedEndGeometry;
        }

        const double axialRangeGap =
            nextRange.minAxial - currentRange.maxAxial;

        if (axialRangeGap >= 0.0 &&
            axialRangeGap <= AxialRangeGapTolerance)
        {
            return HoleSegmentConnectionKind::AxialRangeNear;
        }

        return HoleSegmentConnectionKind::Unknown;
    }

    HoleSegmentConnection buildHoleSegmentConnection(
        const HoleSegmentRangeOnCandidateAxis& currentRange,
        const HoleSegmentRangeOnCandidateAxis& nextRange,
        const std::vector<HoleEndCandidate>& endCandidates)
    {
        HoleSegmentConnection connection;

        connection.currentSegmentCandidateIndex =
            currentRange.segmentCandidateIndex;
        connection.nextSegmentCandidateIndex =
            nextRange.segmentCandidateIndex;

        connection.currentEndCandidateIndex =
            currentRange.maxEndCandidateIndex;
        connection.nextEndCandidateIndex =
            nextRange.minEndCandidateIndex;

        connection.axialRangeGap =
            nextRange.minAxial - currentRange.maxAxial;

        connection.kind =
            classifyAdjacentSegmentConnection(
                currentRange,
                nextRange,
                endCandidates);

        return connection;
    }

    std::vector<HoleSegmentConnection> buildHoleSegmentConnections(
        const HoleCandidate& candidate,
        const std::vector<HoleEndCandidate>& endCandidates)
    {
        std::vector<HoleSegmentConnection> connections;

        if (candidate.segmentRanges.size() < 2)
        {
            return connections;
        }

        connections.reserve(candidate.segmentRanges.size() - 1);

        for (int i = 0;
             i + 1 < static_cast<int>(candidate.segmentRanges.size());
             ++i)
        {
            const auto& currentRange = candidate.segmentRanges[i];
            const auto& nextRange = candidate.segmentRanges[i + 1];

            connections.push_back(
                buildHoleSegmentConnection(
                    currentRange,
                    nextRange,
                    endCandidates));
        }

        return connections;
    }

    std::vector<HoleSegmentChain> buildHoleSegmentChains(
        const HoleCandidate& candidate,
        const std::vector<HoleSegmentConnection>& connections)
    {
        std::vector<HoleSegmentChain> chains;

        if (candidate.segmentRanges.empty())
        {
            return chains;
        }

        HoleSegmentChain currentChain;
        currentChain.index = 0;
        currentChain.segmentCandidateIndices.push_back(
            candidate.segmentRanges.front().segmentCandidateIndex);

        for (const auto& connection : connections)
        {
            const bool isConnected =
                connection.kind == HoleSegmentConnectionKind::SharedEndGeometry ||
                connection.kind == HoleSegmentConnectionKind::ShoulderPlane ||
                connection.kind == HoleSegmentConnectionKind::AxialRangeNear;

            if (isConnected)
            {
                currentChain.segmentCandidateIndices.push_back(
                    connection.nextSegmentCandidateIndex);
                continue;
            }

            chains.push_back(currentChain);

            currentChain = HoleSegmentChain{};
            currentChain.index = static_cast<int>(chains.size());
            currentChain.segmentCandidateIndices.push_back(
                connection.nextSegmentCandidateIndex);
        }

        chains.push_back(currentChain);

        return chains;
    }

    bool HoleRecognitionUtil::isPointOnPlaneFace(
        const GeometryModel& model,
        int faceIndex,
        const gp_Pnt& point,
        double tolerance)
    {
        const auto* faceData = model.faceAt(faceIndex);

        if (faceData == nullptr)
        {
            return false;
        }

        if (!faceData->info.plane.has_value())
        {
            return false;
        }

        const auto& plane = faceData->info.plane.value();

        const gp_Vec originToPoint(
            plane.origin,
            point);

        const double distance =
            std::abs(originToPoint.Dot(gp_Vec(plane.normal)));

        return distance <= tolerance;
    }

    bool HoleRecognitionUtil::hasPlaneFaceContainingEndCenter(
        const GeometryModel& model,
        const HoleEndCandidate& planeEnd,
        const HoleEndCandidate& targetEnd)
    {
        constexpr double PlaneDistanceTolerance = 1.0e-4;

        for (const int faceIndex : planeEnd.geometryRefs.faceIndices)
        {
            if (isPointOnPlaneFace(
                    model,
                    faceIndex,
                    targetEnd.center,
                    PlaneDistanceTolerance))
            {
                return true;
            }
        }

        return false;
    }

    bool HoleRecognitionUtil::isShoulderPlaneConnection(
        const GeometryModel& model,
        const HoleEndCandidate& currentEnd,
        const HoleEndCandidate& nextEnd)
    {
        if (hasPlaneFaceContainingEndCenter(
                model,
                currentEnd,
                nextEnd))
        {
            return true;
        }

        if (hasPlaneFaceContainingEndCenter(
                model,
                nextEnd,
                currentEnd))
        {
            return true;
        }

        return false;
    }
}
