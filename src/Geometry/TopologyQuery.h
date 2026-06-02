#ifndef TOPOLOGYQUERY_H
#define TOPOLOGYQUERY_H

#include <vector>

namespace OccQtCore
{
    class GeometryModel;

    namespace TopologyQuery
    {
        struct FaceGroupBoundaryConnection
        {
            int sourceFaceIndex = -1;
            int boundaryEdgeIndex = -1;
            int adjacentFaceIndex = -1;
        };

        bool isValidFaceIndex(
            const GeometryModel& model,
            int faceIndex);

        bool isValidWireIndex(
            const GeometryModel& model,
            int wireIndex);

        bool isValidEdgeIndex(
            const GeometryModel& model,
            int edgeIndex);

        bool isValidVertexIndex(
            const GeometryModel& model,
            int vertexIndex);

        /**
         * @brief 2つのFaceが共通Edgeを持つか判定する
         * @param model 対象ジオメトリモデル
         * @param lhsFaceIndex 比較元Faceインデックス
         * @param rhsFaceIndex 比較先Faceインデックス
         * @return
         */
        bool hasSharedEdge(
            const GeometryModel& model,
            int lhsFaceIndex,
            int rhsFaceIndex);

        /**
         * @brief Faceに属するEdgeを取得する
         *
         * Face -> Wire -> Edge の合成問い合わせ。
         */
        std::vector<int> edgesOfFace(
            const GeometryModel& model,
            int faceIndex);

        /**
         * @brief Edgeに接続しているFaceを取得する
         *
         * Edge -> Wire -> Face の合成問い合わせ。
         */
        std::vector<int> facesOfEdge(
            const GeometryModel& model,
            int edgeIndex);

        /**
         * @brief Edgeに接続しているFaceを取得する
         *
         * excludeFaceIndex が指定された場合、そのFaceは除外する。
         */
        std::vector<int> adjacentFacesOfEdge(
            const GeometryModel& model,
            int edgeIndex,
            int excludeFaceIndex = -1);

        /**
         * @brief Faceに隣接しているFaceを取得する
         *
         * Face -> Edge -> Face の合成問い合わせ。
         */
        std::vector<int> adjacentFacesOfFace(
            const GeometryModel& model,
            int faceIndex);

        /**
         * @brief Face群の外側境界接続を取得する
         *
         * 指定されたFace群から外側へ出る
         * sourceFace / boundaryEdge / adjacentFace の組を返す。
         */
        std::vector<FaceGroupBoundaryConnection> collectBoundaryConnectionsOfFaceGroup(
            const GeometryModel& model,
            const std::vector<int>& faceIndices);
    }
}

#endif // TOPOLOGYQUERY_H
