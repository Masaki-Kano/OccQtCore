#include <QStringList>

#include "Log/LogFormatUtil.h"

#include "Geometry/GeometryModel.h"
#include "Geometry/GeometryTypes.h"

namespace
{
    template <typename TextBuilder> QString joinIndexTexts(const std::vector<int>& indices, TextBuilder buildText)
    {
        QStringList texts;
        texts.reserve(static_cast<qsizetype>(indices.size()));

        for (int index : indices)
        {
            texts.append(buildText(index));
        }

        return texts.join(", ");
    }

    QString formatXyz(double x, double y, double z, int precision)
    {
        return QString("(%1, %2, %3)")
            .arg(x, 0, 'f', precision)
            .arg(y, 0, 'f', precision)
            .arg(z, 0, 'f', precision);
    }

    QString formatWireKind(bool isOuter, bool isInner, bool isClosed)
    {
        QString kindText;

        if (isOuter)
        {
            kindText = "Outer";
        }
        else if (isInner)
        {
            kindText = "Inner";
        }
        else
        {
            kindText = "Unknown";
        }

        kindText += isClosed ? "/Closed" : "/Open";

        return kindText;
    }
}

namespace OccQtCore::LogFormatUtil
{
    QString formatIndexList(const std::vector<int>& indices)
    {
        if (indices.empty())
        {
            return "なし";
        }

        return joinIndexTexts(
            indices,
            [](int index)
            {
                return QString::number(index);
            });
    }

    QString formatFaceIndexList(const GeometryModel& model, const std::vector<int>& faceIndices)
    {
        return joinIndexTexts(
            faceIndices,
            [&model](int faceIndex)
            {
                return formatFaceIndex(model, faceIndex);
            });
    }

    QString formatWireIndexList(const GeometryModel& model, const std::vector<int>& wireIndices)
    {
        return joinIndexTexts(
            wireIndices,
            [&model](int wireIndex)
            {
                return formatWireIndex(model, wireIndex);
            });
    }

    QString formatWireRefList(
        const GeometryModel& model,
        const std::vector<OrientedWireRef>& wireRefs)
    {
        if (wireRefs.empty())
        {
            return "なし";
        }

        QStringList texts;
        texts.reserve(
            static_cast<qsizetype>(wireRefs.size()));

        for (const auto& wireRef : wireRefs)
        {
            texts.append(
                formatWireRef(model, wireRef));
        }

        return texts.join(", ");
    }

    QString formatEdgeIndexList(const GeometryModel& model, const std::vector<int>& edgeIndices)
    {
        return joinIndexTexts(
            edgeIndices,
            [&model](int edgeIndex)
            {
                return formatEdgeIndex(model, edgeIndex);
            });
    }

    QString formatFaceIndex(const GeometryModel& model, int faceIndex)
    {
        QString kindText = "Invalid";

        const auto* faceData = model.faceAt(faceIndex);
        if (faceData != nullptr)
        {
            kindText = surfaceKindDisplayName(faceData->info.kind);
        }

        return QString("%1:%2")
            .arg(faceIndex)
            .arg(kindText);
    }

    QString formatWireIndex(
        const GeometryModel& model,
        int wireIndex)
    {
        QString kindText = "Invalid";

        const auto* wireData = model.wireAt(wireIndex);

        if (wireData != nullptr)
        {
            kindText = wireData->info.isClosed
                           ? "Closed"
                           : "Open";
        }

        return QString("%1:%2")
            .arg(wireIndex)
            .arg(kindText);
    }

    QString formatWireRef(
        const GeometryModel& model,
        const OrientedWireRef& wireRef)
    {
        QString kindText = "Invalid";

        const auto* wireData =
            model.wireAt(wireRef.wireIndex);

        if (wireData != nullptr)
        {
            kindText = formatWireKind(
                wireRef.isOuter,
                wireRef.isInner,
                wireData->info.isClosed);
        }

        return QString("%1:%2/%3")
            .arg(wireRef.wireIndex)
            .arg(kindText)
            .arg(orientationDisplayName(
                wireRef.orientation));
    }

    QString formatEdgeIndex(const GeometryModel& model, int edgeIndex)
    {
        QString kindText = "Invalid";

        const auto* edgeData = model.edgeAt(edgeIndex);
        if (edgeData != nullptr)
        {
            kindText = curveKindDisplayName(edgeData->info.kind);
        }

        return QString("%1:%2")
            .arg(edgeIndex)
            .arg(kindText);
    }

    QString formatPoint(const gp_Pnt& point)
    {
        return formatXyz(point.X(), point.Y(), point.Z(), 3);
    }

    QString formatDirection(const gp_Dir& direction)
    {
        return formatXyz(direction.X(), direction.Y(), direction.Z(), 6);
    }
}
