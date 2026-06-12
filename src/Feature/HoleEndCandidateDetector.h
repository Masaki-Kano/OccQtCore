#ifndef HOLEENDCANDIDATEDETECTOR_H
#define HOLEENDCANDIDATEDETECTOR_H

#include <vector>

#include "Feature/HoleRecognitionTypes.h"
#include "Geometry/GeometryTypes.h"

namespace OccQtCore
{
    class GeometryModel;
}

namespace OccQtCore::Feature
{
    class HoleEndCandidateDetector
    {
    public:
        HoleEndCandidateDetector(
            const GeometryModel& model,
            const std::vector<HoleWallCandidate>& wallCandidates);

        std::vector<HoleEndCandidate> detect() const;

    private:
        std::vector<HoleEndCandidate> buildFromWallConnection(
            const HoleWallCandidate& sourceWallCandidate,
            int adjacentFaceIndex,
            int connectionEdgeIndex,
            int wallBoundaryLoopIndex) const;

        std::vector<HoleEndCandidate> buildDirectConnection(
            const HoleWallCandidate& sourceWallCandidate,
            int adjacentFaceIndex,
            int connectionEdgeIndex,
            int wallBoundaryLoopIndex) const;

        std::vector<HoleEndCandidate> buildThroughTransitionSurface(
            const HoleWallCandidate& sourceWallCandidate,
            int transitionFaceIndex,
            int wallConnectionEdgeIndex,
            int wallBoundaryLoopIndex) const;

        bool isFaceOwnedByOtherWallCandidate(
            int faceIndex,
            int sourceWallCandidateIndex) const;

        bool isConnectionEdgeOnInnerWireOfFace(
            int faceIndex,
            int edgeIndex) const;

        bool isTransitionSurface(
            SurfaceKind kind) const;

        HoleEndCandidate makeBaseEndCandidateFromWall(
            const HoleWallCandidate& wallCandidate,
            int wallBoundaryLoopIndex) const;

        HoleEndCandidate makeConnectedEndCandidateFromWall(
            const HoleWallCandidate& wallCandidate,
            int connectionEdgeIndex,
            int wallBoundaryLoopIndex) const;

    private:
        const GeometryModel& m_model;
        const std::vector<HoleWallCandidate>& m_wallCandidates;
    };
}

#endif // HOLEENDCANDIDATEDETECTOR_H
