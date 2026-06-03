#ifndef SURFACEUTIL_H
#define SURFACEUTIL_H

#include <TopoDS_Face.hxx>
#include <gp_Ax1.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

namespace OccQtCore::SurfaceUtil
{
    /**
     * @brief 2つの半径が許容値内で一致するか判定する。
     *
     * @param lhs 比較元半径
     * @param rhs 比較先半径
     * @param tolerance 半径比較の許容値。
     * @return 許容値内で一致する場合 true
     */
    bool isSameRadius(
        double lhs,
        double rhs,
        double tolerance);

    /**
     * @brief 2つの方向が同方向または逆方向として扱えるかを判定する。
     *
     * gp_Dirの内積の絶対値を用いて判定する。
     * 円筒軸の比較など、向きの正負を問わず同一直線方向とみなしたい場合に使用する。
     *
     * @param lhs 比較元方向。
     * @param rhs 比較先方向
     * @param tolerance 方向比較の許容値
     * @return 同方向または逆方向として扱える場合 true
     */
    bool isSameDirectionOrReverse(
        const gp_Dir& lhs,
        const gp_Dir& rhs,
        double tolerance);

    /**
     * @brief 点が指定軸上に存在するか判定する。
     *
     * 軸上の基準点から対称点へのベクトルと、軸方向ベクトルの外積の長さを用いて、
     * 点と軸線の距離を判定する。
     *
     * @param axisPoint 軸線上の基準点
     * @param axisDirection 軸方向
     * @param point 判定対象点
     * @param tolerance 軸線からの距離許容値
     * @return 点が軸線上にあるとみなせる場合 true
     */
    bool isPointOnAxis(
        const gp_Pnt& axisPoint,
        const gp_Dir& axisDirection,
        const gp_Pnt& point,
        double tolerance);

    bool isSameAxis(
        const gp_Pnt& lhsAxisPoint,
        const gp_Dir& lhsAxisDirection,
        const gp_Pnt& rhsAxisPoint,
        const gp_Dir& rhsAxisDirection,
        double axisLineTolerance,
        double directionTolerance);

    /**
     * @brief 2つの円筒が同じ軸線と半径を持つかを判定する。
     *
     * 円筒面全体の同一性を保証するものではない。
     * 判定対象は、半径、軸方向、軸線位置のみであり、
     * U/V範囲、Faceのトリム状態、Face向きなどは考慮しない。
     *
     * @param lhsAxisPoint 比較元円筒軸上の点。
     * @param lhsAxisDirection 比較元円筒軸方向。
     * @param lhsRadius 比較元円筒半径。
     * @param rhsAxisPoint 比較先円筒軸上の点。
     * @param rhsAxisDirection 比較先円筒軸方向。
     * @param rhsRadius 比較先円筒半径。
     * @param radiusTolerance 半径比較の許容値。
     * @param axisLineTolerance 軸線位置比較の許容値。
     * @param directionTolerance 軸方向比較の許容値。
     * @return 同じ軸線と半径を持つとみなせる場合 true。
     */
    bool isSameCylinderAxisAndRadius(
        const gp_Pnt& lhsAxisPoint,
        const gp_Dir& lhsAxisDirection,
        double lhsRadisu,
        const gp_Pnt& rhsAxisPoint,
        const gp_Dir& rhsAxisDirection,
        double rhsRadius,
        double radiusTolerance,
        double axisLineTolerance,
        double directionTolerance);

    /**
     * @brief パラメータ範囲の幅を取得する。
     *
     * 単純に max - min の絶対値を返す。
     * U/Vなど、任意の1次元パラメータ範囲の幅を取得するために使用する。
     *
     * @param min パラメータ最小値。
     * @param max パラメータ最大値。
     * @return パラメータ範囲の幅。
     */
    double parameterSpan(
        double uMin,
        double uMax);

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

    double projectPointToAxis(
        const gp_Pnt& axisPoint,
        const gp_Dir& axisDirection,
        const gp_Pnt& point);
}

#endif // SURFACEUTIL_H
