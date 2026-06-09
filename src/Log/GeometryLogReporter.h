#ifndef GEOMETRYLOGREPORTER_H
#define GEOMETRYLOGREPORTER_H

#include "Core/PickResult.h"
#include "Geometry//GeometryTypes.h"

namespace OccQtCore
{
    class AppLogger;
    class GeometryModel;

    struct GeometryLogReport
    {
        const GeometryModel& model;

        bool outputSummary = true;
        bool outputTopologySummary = true;
        bool outputComplexGeometrySummary = true;
        bool outputDetailDiagnostics = true;
    };

    struct GeometryElementLogReport
    {
        const GeometryModel& model;

        PickedShapeType type = PickedShapeType::Unknown;
        int elementIndex = -1;

        bool outputTree = false;
    };

    class GeometryLogReporter
    {
    public:
        explicit GeometryLogReporter(AppLogger* logger);
        void logGeometry(const GeometryLogReport& report) const;
        void logGeometryElement(const GeometryElementLogReport& report) const;

    private:
        void logSummary(const GeometryModel& model) const;
        void logTopologySummary(const GeometryModel& model) const;
        void logComplexGeometrySummary(const GeometryModel& model) const;
        void logDetailDiagnostics(const GeometryModel& model) const;
        void logFaceDetails(const GeometryModel& model, int faceIndex) const;
        void logWireDetails(const GeometryModel& model, int wireIndex) const;
        void logEdgeDetails(const GeometryModel& model, int edgeIndex) const;
        void logVertexDetails(const GeometryModel& model, int vertexIndex) const;
        void logFaceTreeDetails(const GeometryModel& model, int faceIndex) const;
        void logWireTreeDetails(const GeometryModel& model, int wireIndex) const;

        void logFaceSurfaceDetails(const FaceInfo& info) const;
        void logPlaneDetails(const PlaneInfo& info) const;
        void logCylinderDetails(const CylinderInfo& info) const;
        void logConeDetails(const ConeInfo& info) const;
        void logSphereDetails(const SphereInfo& info) const;
        void logTorusDetails(const TorusInfo& info) const;
        void logEdgeCurveDetails(const EdgeInfo& info) const;
        void logLineDetails(const LineInfo& info) const;
        void logCircleDetails(const CircleInfo& info) const;
        void logEllipseDetails(const EllipseInfo& info) const;

    private:
        AppLogger* m_logger = nullptr;
    };
}

#endif // GEOMETRYLOGREPORTER_H
