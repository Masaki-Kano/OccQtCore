#ifndef APPLOGREPORTER_H
#define APPLOGREPORTER_H

#include <vector>

#include <QString>

#include "Core/SelectionInfo.h"
#include "Geometry/GeometryGraph.h"

namespace OccQtCore
{
    class AppLogger;
    class GeometryModel;

    class AppLogReporter
    {
    public:
        explicit AppLogReporter(AppLogger* logger);

        void logSelection(const SelectionInfo& selectionInfo) const;

        void logFaceGraph(
            const GeometryGraph& graph,
            int faceIndex) const;

        void logEdgeGraph(
            const GeometryGraph& graph,
            int edgeIndex) const;

        void logStepLoadFailed(const QString& errorMessage) const;
        void logStepLoaded(const QString& filePath) const;

        void logGeometryModelDiagnostics(
            const GeometryModel& model) const;

    private:
        QString formatIndexList(const std::vector<int>& indices) const;

    private:
        AppLogger* m_logger = nullptr;
    };
}

#endif // APPLOGREPORTER_H
