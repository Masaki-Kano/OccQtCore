#ifndef TOPOLOGYQUERY_H
#define TOPOLOGYQUERY_H

#include <vector>

namespace OccQtCore
{
    class GeometryModel;

    namespace TopologyQuery
    {
        /**
         * @brief Face集合から外部Faceへ出る境界接続
         */
        struct FaceBoundaryConnection
        {
            int sourceFaceIndex = -1;
            int boundaryEdgeIndex = -1;
            int adjacentFaceIndex = -1;
        };

        //==================================================
        // Index validation
        //==================================================

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

        //==================================================
        // Single-element queries
        //==================================================

        /**
         * @brief Faceに属するEdgeを取得する
         *
         * Face -> Wire -> Edge の合成問い合わせ。
         * 同一EdgeがFace内で複数使用されていても、
         * 戻り値は一意化される。
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
         * @brief Faceに属するVertexを取得する
         *
         * Face -> Edge -> Vertex の合成問い合わせ。
         */
        std::vector<int> verticesOfFace(
            const GeometryModel& model,
            int faceIndex);

        /**
         * @brief Face内におけるEdgeの使用回数を取得する
         *
         * シームEdgeなど、同一Edgeが同じFace内で
         * 複数回使用されている場合は二つ以上を返す。
         */
        int edgeUseCountInFace(
            const GeometryModel& model,
            int faceIndex,
            int edgeIndex);

        //==================================================
        // Face-to-Face relation queries
        //==================================================

        /**
         * @brief 2つのFaceが共有するEdgeを取得する
         */
        std::vector<int> sharedEdgesOfFace(
            const GeometryModel& model,
            int lhsFaceIndex,
            int rhsFaceIndex);

        /**
         * @brief 2つのFaceが共通のEdgeをもつか判定する
         */
        bool hasSharedEdge(
            const GeometryModel& model,
            int lhsFaceIndex,
            int rhsFaceIndex);

        /**
         * @brief Faceに隣接しているFaceを取得する
         *
         * Face -> Edge -> Face の合成問い合わせ。
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
         * @brief Edgeに接続しているFaceから、
         *        指定されたFace集合を除外して取得する
         */
        std::vector<int> adjacentFacesOfEdgeExcludingFaces(
            const GeometryModel& model,
            int edgeIndex,
            const std::vector<int>& excludeFaceIndices);

        //==================================================
        // Face-set queries
        //==================================================

        /**
         * @brief Face集合に属するEdgeを取得する
         *
         * 戻り値は一意化される
         */
        std::vector<int> edgesOfFaces(
            const GeometryModel& model,
            const std::vector<int>& faceIndices);

        /**
         * @brief Face集合の境界Edgeを取得する
         *
         * Face集合の内側だけで完結するEdgeは除外し、
         * 集合の外側に露出するEdgeを返す。
         *
         * シームEdgeについてはFace内使用回数も考慮する
         */
        std::vector<int> boundaryEdgesOfFaces(
            const GeometryModel& model,
            const std::vector<int>& faceIndices);

        /**
         * @brief Face集合に隣接する外部Faceを取得する
         *
         * faceIndicesに含まれるFaceは戻り値から除外される。
         */
        std::vector<int> adjacentFacesOfFaces(
            const GeometryModel& model,
            const std::vector<int>& faceIndices);

        /**
         * @brief Face集合から外部Faceへ出る境界接続を取得する
         *
         * sourceFace / boundaryEdge / adjacentFace の組を返す。
         */
        std::vector<FaceBoundaryConnection> boundaryConnectionsOfFace(
            const GeometryModel& model,
            const std::vector<int>& faceIndices);
    }
}

#endif // TOPOLOGYQUERY_H
