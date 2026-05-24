#include "DocumentData.h"

namespace OccQtCore
{
    void DocumentData::clear()
    {
        m_filePath.clear();
        m_shape.Nullify();
        m_geometryModel.clear();
    }

    bool DocumentData::hasShape() const
    {
        return !m_shape.IsNull();
    }

    void DocumentData::setFilePath(const QString& filePath)
    {
        m_filePath = filePath;
    }

    const QString& DocumentData::filePath() const
    {
        return m_filePath;
    }

    void DocumentData::setShape(const TopoDS_Shape& shape)
    {
        m_shape = shape;
        m_geometryModel.build(m_shape);
    }

    const TopoDS_Shape& DocumentData::shape() const
    {
        return m_shape;
    }

    const GeometryModel& DocumentData::geometryModel() const
    {
        return m_geometryModel;
    }

    GeometryModel& DocumentData::geometryModel()
    {
        return m_geometryModel;
    }

}
