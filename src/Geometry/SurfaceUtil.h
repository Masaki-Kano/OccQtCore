#ifndef SURFACEUTIL_H
#define SURFACEUTIL_H

#include <TopoDS_Face.hxx>
#include <gp_Ax1.hxx>

namespace OccQtCore::SurfaceUtil
{
    /**
    * @brief 円筒Faceの向きが内向きか判定する
    *
    * radial は「円筒軸 → 面上点」方向。
    *
    * 外径円筒:
    *   Face法線が radial と同方向になりやすい
    *
    * 内径円筒:
    *   Face法線が radial と逆方向になりやすい
    */
    bool isCylinderFaceInwardOriented(
        const TopoDS_Face& face,
        const gp_Ax1& cylinderAxis,
        double uMin,
        double uMax,
        double vMin,
        double vMax);
}

#endif // SURFACEUTIL_H
