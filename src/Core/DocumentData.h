#ifndef DOCUMENTDATA_H
#define DOCUMENTDATA_H

#include <QString>
#include <TopoDS_Shape.hxx>

#include "Geometry/ShapeIndex.h"

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

        const ShapeIndex& shapeIndex() const;
        ShapeIndex& shapeIndex();

    private:
        QString m_filePath;
        TopoDS_Shape m_shape;
        ShapeIndex m_shapeIndex;
    };
}

#endif // DOCUMENTDATA_H
