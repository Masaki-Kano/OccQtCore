#ifndef DOCUMENTDATA_H
#define DOCUMENTDATA_H

#include <QString>
#include <TopoDS_Shape.hxx>

#include "Geometry/GeometryModel.h"

namespace OccQtCore
{
    class DocumentData
    {
    public:
        void clear();

        bool hasShape() const;

        void setFilePath(const QString& filePath);
        const QString& filePath() const;

        void setShape(const TopoDS_Shape& shape);
        const TopoDS_Shape& shape() const;

        const GeometryModel& geometryModel() const;
        GeometryModel& geometryModel();

    private:
        QString m_filePath;
        TopoDS_Shape m_shape;
        GeometryModel m_geometryModel;
    };
}

#endif // DOCUMENTDATA_H
