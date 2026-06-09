#ifndef LOGFORMATUTIL_H
#define LOGFORMATUTIL_H

#include <vector>

#include <QString>

#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

#include "Core/PickResult.h"

namespace OccQtCore
{
    class GeometryModel;

    namespace LogFormatUtil
    {
        QString formatIndexList(const std::vector<int>& indices);
        QString formatFaceIndexList(const GeometryModel& model, const std::vector<int>& faceIndices);
        QString formatEdgeIndexList(const GeometryModel& model, const std::vector<int>& edgeIndices);
        QString formatWireIndexList(const GeometryModel& model, const std::vector<int>& wireIndices);
        QString formatFaceIndex(const GeometryModel& model, int faceIndex);
        QString formatWireIndex(const GeometryModel& model, int wireIndex);
        QString formatEdgeIndex(const GeometryModel& model, int edgeIndex);
        QString formatPoint(const gp_Pnt& point);
        QString formatDirection(const gp_Dir& direction);
        QString formatPickedShapeType(PickedShapeType type);

    }
}

#endif // LOGFORMATUTIL_H
