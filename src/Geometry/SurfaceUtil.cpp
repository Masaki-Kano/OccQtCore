#include "Geometry/SurfaceUtil.h"

#include <BRepAdaptor_Surface.hxx>
#include <BRepLProp_SLProps.hxx>
#include <TopAbs_Orientation.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>

namespace OccQtCore::SurfaceUtil
{
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
}
