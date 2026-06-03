#include "Geometry/SurfaceUtil.h"

#include <cmath>

#include <BRepAdaptor_Surface.hxx>
#include <BRepLProp_SLProps.hxx>
#include <TopAbs_Orientation.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>

namespace OccQtCore::SurfaceUtil
{

    bool isSameRadius(
        double lhs,
        double rhs,
        double tolerance)
    {
        return std::abs(lhs - rhs) <= tolerance;
    }

    bool isSameDirectionOrReverse(
        const gp_Dir& lhs,
        const gp_Dir& rhs,
        double tolerance)
    {
        return std::abs(lhs.Dot(rhs)) >= 1.0 - tolerance;
    }

    bool isPointOnAxis(
        const gp_Pnt& axisPoint,
        const gp_Dir& axisDirection,
        const gp_Pnt& point,
        double tolerance)
    {
        const gp_Vec v(axisPoint, point);
        const gp_Vec axisVec(axisDirection);

        return v.Crossed(axisVec).Magnitude() <= tolerance;
    }

    bool isSameAxis(
        const gp_Pnt& lhsAxisPoint,
        const gp_Dir& lhsAxisDirection,
        const gp_Pnt& rhsAxisPoint,
        const gp_Dir& rhsAxisDirection,
        double axisLineTolerance,
        double directionTolerance)
    {
        if (!isSameDirectionOrReverse(
                lhsAxisDirection,
                rhsAxisDirection,
                directionTolerance))
        {
            return false;
        }

        if (!isPointOnAxis(
                lhsAxisPoint,
                lhsAxisDirection,
                rhsAxisPoint,
                axisLineTolerance))
        {
            return false;
        }

        return true;
    }

    bool isSameCylinderAxisAndRadius(
        const gp_Pnt& lhsAxisPoint,
        const gp_Dir& lhsAxisDirection,
        double lhsRadius,
        const gp_Pnt& rhsAxisPoint,
        const gp_Dir& rhsAxisDirection,
        double rhsRadius,
        double radiusTolerance,
        double axisLineTolerance,
        double directionTolerance)
    {
        if (!isSameRadius(lhsRadius, rhsRadius, radiusTolerance))
        {
            return false;
        }

        return isSameAxis(
            lhsAxisPoint,
            lhsAxisDirection,
            rhsAxisPoint,
            rhsAxisDirection,
            axisLineTolerance,
            directionTolerance);
    }

    double parameterSpan(
        double min,
        double max)
    {
        return std::abs(max - min);
    }

    bool isCylinderFaceInwardOriented(
        const TopoDS_Face& face,
        const gp_Ax1& cylinderAxis,
        double uMin,
        double uMax,
        double vMin,
        double vMax)
    {
        BRepAdaptor_Surface surface(face);

        const double u = 0.5 * (uMin + uMax);
        const double v = 0.5 * (vMin + vMax);

        BRepLProp_SLProps props(surface, u, v, 1, 1.0e-6);

        if (!props.IsNormalDefined())
        {
            return false;
        }

        const gp_Pnt point = props.Value();
        gp_Dir normal = props.Normal();

        // OCCのFace向きを反映する
        if (face.Orientation() == TopAbs_REVERSED)
        {
            normal.Reverse();
        }

        const gp_Pnt axisOrigin = cylinderAxis.Location();
        const gp_Dir axisDirection = cylinderAxis.Direction();

        const gp_Vec axisVec(axisDirection);
        const gp_Vec originToPoint(axisOrigin, point);

        const double t = originToPoint.Dot(axisVec);

        const gp_Pnt projectedPoint(
            axisOrigin.X() + axisVec.X() * t,
            axisOrigin.Y() + axisVec.Y() * t,
            axisOrigin.Z() + axisVec.Z() * t);

        gp_Vec radial(projectedPoint, point);

        if (radial.Magnitude() <= 1.0e-6)
        {
            return false;
        }

        radial.Normalize();

        const gp_Vec normalVec(normal);

        return normalVec.Dot(radial) < 0.0;
    }

    double projectPointToAxis(
        const gp_Pnt& axisPoint,
        const gp_Dir& axisDirextion,
        const gp_Pnt& point)
    {
        const gp_Vec axisVector(axisDirextion);
        const gp_Vec axisPointToPoint(axisPoint, point);

        return axisPointToPoint.Dot(axisVector);
    }
}
