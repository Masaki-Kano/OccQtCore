#ifndef DOCUMENTDATA_H
#define DOCUMENTDATA_H

#include <QString>
#include <TopoDS_Shape.hxx>

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

    private:
        QString m_filePath;
        TopoDS_Shape m_shape;
    };
}

#endif // DOCUMENTDATA_H
