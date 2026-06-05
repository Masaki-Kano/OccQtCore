#include "Feature/HoleWallCandidateDetector.h"

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

    bool isSameCylinderCandidate(
        const OccQtCore::Feature::HoleWallCandidate& lhs,
        const OccQtCore::Feature::HoleWallCandidate& rhs)
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

            for (int faceIndex : wallCandidates[candidateIndex].geometryRefs.faceIndices)
            {
                totalUSpan += cylinderFaceUSpan(model, faceIndex);
            }
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
    const OccQtCore::Feature::HoleWallCandidate& candidate)
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
}

namespace OccQtCore::Feature
{
    std::vector<HoleWallCandidate> HoleWallCandidateDetector::detect(
        const GeometryModel& model) const
    {
        const auto rawCandidates = collectRawCylinderCandidates(model);

        std::vector<HoleWallCandidate> wallCandidates;
        std::vector<bool> used(rawCandidates.size(), false);

        for (int baseCandidateIndex = 0;
             baseCandidateIndex < static_cast<int>(rawCandidates.size());
             ++baseCandidateIndex)
        {
            if (used[baseCandidateIndex])
            {
                continue;
            }

            const auto groupCandidateIndices =
                collectConnectedSameCylinderGroup(model, rawCandidates, baseCandidateIndex, used);

            if (!isClosedCylinderWallGroup(model, rawCandidates, groupCandidateIndices))
            {
                continue;
            }

            auto wallCandidate = buildWallCandidateFromGroup(rawCandidates, groupCandidateIndices);

            if (!isLikelyHoleWallByOrientation(model, wallCandidate))
            {
                continue;
            }

            wallCandidate.index = static_cast<int>(wallCandidates.size());

            wallCandidates.push_back(wallCandidate);
        }

        return wallCandidates;
    }

    std::vector<HoleWallCandidate> HoleWallCandidateDetector::collectRawCylinderCandidates(
        const GeometryModel& model) const
    {
        std::vector<HoleWallCandidate> rawCandidates;

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
            candidate.index = static_cast<int>(rawCandidates.size());

            OccQtCore::CollectionUtil::addUnique(
                candidate.geometryRefs.faceIndices,
                face.index);

            candidate.center = cylinder.axis.Location();
            candidate.axisDirection = cylinder.axis.Direction();
            candidate.radius = cylinder.radius;
            candidate.depth = 0.0;

            rawCandidates.push_back(candidate);
        }

        return rawCandidates;
    }

    std::vector<int> HoleWallCandidateDetector::collectConnectedSameCylinderGroup(
        const GeometryModel& model,
        const std::vector<HoleWallCandidate>& rawCandidates,
        int baseCandidateIndex,
        std::vector<bool>& used) const
    {
        std::vector<int> groupCandidateIndices;
        std::vector<int> stack;

        used[baseCandidateIndex] = true;
        stack.push_back(baseCandidateIndex);

        while (!stack.empty())
        {
            const int currentCandidateIndex = stack.back();
            stack.pop_back();

            groupCandidateIndices.push_back(currentCandidateIndex);

            const auto& currentCandidate =
                rawCandidates[currentCandidateIndex];

            for (int nextCandidateIndex = 0;
                 nextCandidateIndex < static_cast<int>(rawCandidates.size());
                 ++nextCandidateIndex)
            {
                if (used[nextCandidateIndex])
                {
                    continue;
                }

                const auto& nextCandidate =
                    rawCandidates[nextCandidateIndex];

                if (!isSameCylinderCandidate(
                        currentCandidate,
                        nextCandidate))
                {
                    continue;
                }

                if (currentCandidate.geometryRefs.faceIndices.empty() ||
                    nextCandidate.geometryRefs.faceIndices.empty())
                {
                    continue;
                }

                const int currentFaceIndex =
                    currentCandidate.geometryRefs.faceIndices.front();

                const int nextFaceIndex =
                    nextCandidate.geometryRefs.faceIndices.front();

                if (!TopologyQuery::hasSharedEdge(
                        model,
                        currentFaceIndex,
                        nextFaceIndex))
                {
                    continue;
                }

                used[nextCandidateIndex] = true;
                stack.push_back(nextCandidateIndex);
            }
        }

        return groupCandidateIndices;
    }

    HoleWallCandidate HoleWallCandidateDetector::buildWallCandidateFromGroup(
        const std::vector<HoleWallCandidate>& rawCandidates,
        const std::vector<int>& groupCandidateIndices) const
    {
        HoleWallCandidate wallCandidate;

        if (groupCandidateIndices.empty())
        {
            return wallCandidate;
        }

        const auto& baseCandidate =
            rawCandidates[groupCandidateIndices.front()];

        for (int candidateIndex : groupCandidateIndices)
        {
            if (candidateIndex < 0 ||
                candidateIndex >= static_cast<int>(rawCandidates.size()))
            {
                continue;
            }

            for (int faceIndex :
                 rawCandidates[candidateIndex].geometryRefs.faceIndices)
            {
                OccQtCore::CollectionUtil::addUnique(
                    wallCandidate.geometryRefs.faceIndices,
                    faceIndex);
            }
        }

        wallCandidate.center = baseCandidate.center;
        wallCandidate.axisDirection = baseCandidate.axisDirection;
        wallCandidate.radius = baseCandidate.radius;
        wallCandidate.depth = 0.0;

        return wallCandidate;
    }
}
