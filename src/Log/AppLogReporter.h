#ifndef APPLOGREPORTER_H
#define APPLOGREPORTER_H

#include <vector>

#include <QString>

#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

#include "Core/SelectionInfo.h"
#include "Feature/HoleRecognitionTypes.h"

namespace OccQtCore
{
    class AppLogger;
    class GeometryModel;

    class AppLogReporter
    {
    public:
        explicit AppLogReporter(AppLogger* logger);

        // ============================================================
        // Operation logs
        // 操作・イベントログ
        // ============================================================
        void logSelection(const SelectionInfo& selectionInfo) const;
        void logStepLoaded(const QString& filePath) const;
        void logStepLoadFailed(const QString& errorMessage) const;

        void logActionStarted(const QString& actionName) const;
        void logActionFinished(const QString& actionName) const;
        void logActionFailed(const QString& actionName, const QString& reason) const;

        // ============================================================
        // Geometry logs
        // 読み込んだ形状データそのもののログ
        // ============================================================
        void logGeometryAnalysisReport(const GeometryModel& model) const;
        void logGeometryDetailDiagnostics(const GeometryModel& model) const;

        void logGeometrySummary(const GeometryModel& model) const;
        void logGeometryTopologySummary(const GeometryModel& model) const;
        void logComplexGeometrySummary(const GeometryModel& model) const;
        void logCircleGroupSummary(const GeometryModel& model) const;

        void logSelectionGeometryDetails(
            const GeometryModel& model,
            const SelectionInfo& selectionInfo) const;

        // 単体詳細：ピック向け
        void logFaceDetails(const GeometryModel& model, int faceIndex) const;
        void logWireDetails(const GeometryModel& model, int wireIndex) const;
        void logEdgeDetails(const GeometryModel& model, int edgeIndex) const;
        void logVertexDetails(const GeometryModel& model, int vertexIndex) const;

        // 階層詳細：診断向け
        void logFaceTreeDetails(const GeometryModel& model, int faceIndex) const;
        void logWireTreeDetails(const GeometryModel& model, int wireIndex) const;

        // ============================================================
        // Feature logs
        // 加工フィーチャとして意味づけした情報のログ
        // ============================================================
        void logHoleRecognitionReport(
            const GeometryModel& model,
            const std::vector<Feature::HoleWallCandidate>& wallCandidates,
            const std::vector<Feature::HoleEndCandidate>& endCandidates,
            const std::vector<Feature::HoleSegmentCandidate>& segmentCandidates) const;

        void logHoleWallCandidates(
            const GeometryModel& model,
            const std::vector<Feature::HoleWallCandidate>& candidates) const;

        void logHoleEndCandidates(
            const GeometryModel& model,
            const std::vector<Feature::HoleEndCandidate>& candidates) const;

        void logHoleSegmentCandidates(
            const GeometryModel& model,
            const std::vector<Feature::HoleSegmentCandidate>& segments,
            const std::vector<Feature::HoleWallCandidate>& wallCandidates,
            const std::vector<Feature::HoleEndCandidate>& endCandidates) const;

        void logHoleCandidates(
            const std::vector<OccQtCore::Feature::HoleCandidate>& candidates,
            const std::vector<OccQtCore::Feature::HoleSegmentCandidate>& segmentCandidates,
            const std::vector<OccQtCore::Feature::HoleWallCandidate>& wallCandidates,
            const std::vector<OccQtCore::Feature::HoleEndCandidate>& endCandidates) const;

    private:
        AppLogger* m_logger = nullptr;
    };
}

#endif // APPLOGREPORTER_H
