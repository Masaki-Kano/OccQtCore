#include "Geometry/SurfaceQuery.h"

#include "Geometry/GeometryModel.h"
#include "Geometry/GeometryTypes.h"
#include "Geometry/SurfaceUtil.h"

namespace
{
constexpr double RadiusTolerance = 1.0e-6;
constexpr double AxisLineTolerance = 1.0e-6;
constexpr double DirectionTolerance = 1.0e-9;
}

namespace OccQtCore::SurfaceQuery
{
    bool areOnSameCylinder(
        const GeometryModel& model,
        int lhsFaceIndex,
        int rhsFaceIndex)
    {
        const auto* lhsFace = model.faceAt(lhsFaceIndex);

        const auto* rhsFace = model.faceAt(rhsFaceIndex);

        if (lhsFace == nullptr ||
            rhsFace == nullptr)
        {
            return false;
        }

        if (lhsFace->info.kind != SurfaceKind::Cylinder ||
            rhsFace->info.kind != SurfaceKind::Cylinder)
        {
            return false;
        }

        if (!lhsFace->info.cylinder.has_value() ||
            !rhsFace->info.cylinder.has_value())
        {
            return false;
        }

        const auto& lhsCylinder = lhsFace->info.cylinder.value();

        const auto& rhsCylinder = rhsFace->info.cylinder.value();

        return SurfaceUtil::
            isSameCylinderAxisAndRadius(
                lhsCylinder.axis.Location(),
                lhsCylinder.axis.Direction(),
                lhsCylinder.radius,
                rhsCylinder.axis.Location(),
                rhsCylinder.axis.Direction(),
                rhsCylinder.radius,
                RadiusTolerance,
                AxisLineTolerance,
                DirectionTolerance);
    }
}
