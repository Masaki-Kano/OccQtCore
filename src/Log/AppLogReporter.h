#ifndef APPLOGREPORTER_H
#define APPLOGREPORTER_H

#include <vector>

#include <QString>

#include "Core/SelectionInfo.h"

namespace OccQtCore
{
    class AppLogger;
    class GeometryModel;

    class AppLogReporter
    {
    public:
        explicit AppLogReporter(AppLogger* logger);

        void logSelection(const SelectionInfo& selectionInfo) const;

        void logStepLoadFailed(const QString& errorMessage) const;
        void logStepLoaded(const QString& filePath) const;

        void logGeometryModelDiagnostics(const GeometryModel& model) const;

        void logPickedFaceDetails(const GeometryModel& model, int faceIndex) const;
        void logPickedEdgeDetails(const GeometryModel& model, int edgeIndex) const;
        void logPickedVertexDetails(const GeometryModel& model, int vertexIndex) const;

        void logAllGeometryDetails(const GeometryModel& model) const;

    private:
        QString formatIndexList(const std::vector<int>& indices) const;

    private:
        AppLogger* m_logger = nullptr;
    };
}

#endif // APPLOGREPORTER_H
