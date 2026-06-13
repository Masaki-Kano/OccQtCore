#ifndef HOLERECOGNITIONMODEL_H
#define HOLERECOGNITIONMODEL_H

#include <string>
#include <vector>

#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

#include "Feature/FeatureTypes.h"

namespace OccQtCore::Feature
{
    /**
    * @brief GeometryTrace の探索方向
    *
    * 穴の入口方向・加工方向ではない。
    * 基準ジオメトリ要素から見た探索方向。
    */
    enum class GeometryTraceDirection
    {
        Unknown,
        AxialNegative,
        AxialPositive,
        Radial,
        Other
    };

    /**
    * @brief ジオメトリ探索木の1ノード
    *
    * 強いジオメトリ要素から接続方向へ探索したときに、
    * その深さで観測された接続ジオメトリ群を保持する。
    *
    * ここでは Plane / Cone / Open / Bottom / Connection などの
    * 意味分類はしない。
    *
    * 面種別・曲線種別が必要な場合は、GeometryRefs から
    * GeometryModel の FaceInfo / EdgeInfo を参照して判断する。
    */
    struct GeometryTraceNode
    {
        int index = -1;

        /**
        * @brief Trace内での探索深さ
        *
        * 0 は基準ジオメトリ要素に直接接続するジオメトリ群。
        */
        int depth = -1;

        /**
        * @brief このNodeの親Node
        *
        * 基準ジオメトリ要素に直接接続する場合は -1。
        * 分岐がある場合、同じ parentNodeIndex を持つNodeが複数できる。
        */
        int parentNodeIndex = -1;

        /**
        * @brief この探索Nodeで観測されたジオメトリ群
        *
        * Face / Wire / Edge / Vertex を複数持てる。
        * Trace段階では穴ドメインの意味づけはしない。
        */
        GeometryRefs geometryRefs;

        /**
        * @brief 基準軸へ投影した、このジオメトリ群の最小位置
        *
        * 軸方向評価が不要または未計算の場合は 0.0。
        */
        double axialMin = 0.0;

        /**
        * @brief 基準軸へ投影した、このジオメトリ群の最大位置
        *
        * 軸方向評価が不要または未計算の場合は 0.0。
        */
        double axialMax = 0.0;

        /**
        * @brief デバッグ用メモ
        */
        std::string note;
    };

    /**
    * @brief 強いジオメトリ要素から接続先へ辿った探索結果
    *
    * 穴専用ではない、生データ寄りの探索結果。
    *
    * nodes は一本道とは限らない。
    * depth と parentNodeIndex によって探索木として表現する。
    */
    struct GeometryTrace
    {
        int index = -1;

        GeometryTraceDirection direction = GeometryTraceDirection::Unknown;

        /**
        * @brief 探索開始位置
        *
        * 穴認識では HoleSection の axialMin / axialMax などを入れる想定。
        */
        double originAxialPosition = 0.0;

        /**
        * @brief 探索方向へ辿った接続ジオメトリ群
        */
        std::vector<GeometryTraceNode> nodes;

        /**
        * @brief デバッグ用メモ
        */
        std::string note;
    };

    /**
    * @brief 穴壁
    *
    * CAD上で複数Faceに分割されていても、
    * 同じ円筒壁として説明できるFace群を1つに束ねたもの。
    *
    * HoleWall は Face 1枚ではなく、円筒壁としての意味単位。
    * 生の円筒Face情報は GeometryModel / FaceData / FaceInfo を参照する。
    */
    struct HoleWall
    {
        int index = -1;
        bool isValid = false;

        GeometryRefs geometryRefs;

        gp_Pnt axisPoint;
        gp_Dir axisDirection;

        double radius = 0.0;

        double axialMin = 0.0;
        double axialMax = 0.0;
    };

    /**
    * @brief 穴を構成する円筒区間
    *
    * 同軸・同径・軸方向に連続する HoleWall 群からなる、
    * 穴の1つの円筒区間。
    *
    * これは穴全体ではない。
    * 段付き穴や座ぐり穴では、複数の HoleSection が
    * 1つの HoleAssembly に束ねられる。
    */
    struct HoleSection
    {
        int index = -1;
        bool isValid = false;

        gp_Pnt axisPoint;
        gp_Dir axisDirection;

        double radius = 0.0;

        double axialMin = 0.0;
        double axialMax = 0.0;

        /**
        * @brief この円筒区間を構成する HoleWall の index
        */
        std::vector<int> wallIndices;

        GeometryRefs geometryRefs;

        /**
        * @brief この円筒区間から接続方向へ探索した生データ
        *
        * 初期実装では AxialNegative / AxialPositive の2本を作る想定。
        * ただし、構造としては2方向固定にしない。
        */
        std::vector<GeometryTrace> traces;
    };

    /**
    * @brief HoleSection の終端解釈種別
    *
    * GeometryTrace を穴ドメインのルールで解釈した結果。
    */
    enum class HoleTerminalKind
    {
        Unknown,
        Open,
        Bottom,
        Broken
    };

    /**
    * @brief HoleSection の1つの GeometryTrace が終端として説明された結果
    *
    * GeometryTrace の探索木を穴ドメインのルールで解釈し、
    * そのTraceが Open / Bottom / Broken / Unknown として
    * 終わる場合に生成する。
    *
    * 別の HoleSection へ接続すると説明できる場合は、
    * HoleTerminal ではなく HoleConnection を生成する。
    */
    struct HoleTerminal
    {
        int index = -1;

        /**
        * @brief 解釈元の HoleSection
        */
        int sectionIndex = -1;

        /**
        * @brief 解釈元の GeometryTrace
        */
        int traceIndex = -1;

        GeometryTraceDirection direction = GeometryTraceDirection::Unknown;

        HoleTerminalKind kind = HoleTerminalKind::Unknown;

        GeometryRefs geometryRefs;

        /**
        * @brief 根拠になった GeometryTraceNode の index
        */
        std::vector<int> traceNodeIndices;

        std::string explanation;
    };

    /**
    * @brief HoleSection 同士の接続種別
    *
    * GeometryTrace の探索木を穴ドメインのルールで解釈し、
    * Section同士が同じ穴の一部として説明できる場合に作る。
    */
    enum class HoleConnectionKind
    {
        Unknown,
        Direct,
        ThroughTransition,
        SharedBoundary,
        Broken
    };

    /**
    * @brief HoleSection 同士の接続関係
    *
    * グラフ構造におけるエッジに相当する。
    */
    struct HoleConnection
    {
        int index = -1;

        int fromSectionIndex = -1;
        int toSectionIndex = -1;

        /**
         * @brief 接続解釈の根拠になった from 側 GeometryTrace
         */
        int fromTraceIndex = -1;

        /**
         * @brief 接続解釈の根拠になった to 側 GeometryTrace
         *
         * to 側Traceがまだ未解決の場合は -1。
         */
        int toTraceIndex = -1;

        GeometryTraceDirection fromDirection = GeometryTraceDirection::Unknown;
        GeometryTraceDirection toDirection = GeometryTraceDirection::Unknown;

        HoleConnectionKind kind = HoleConnectionKind::Unknown;

        GeometryRefs geometryRefs;

        /**
         * @brief 根拠になった GeometryTraceNode の index
         */
        std::vector<int> traceNodeIndices;

        std::string explanation;
    };

    /**
     * @brief GeometryTrace を穴ドメインで解釈した結果
     *
     * HoleGeometryInterpreter が生成する想定。
     *
     * GeometryTrace は生データ寄り。
     * HoleTerminal / HoleConnection は穴ルールで説明した結果。
     */
    struct HoleInterpretation
    {
        std::vector<HoleTerminal> terminals;
        std::vector<HoleConnection> connections;
    };

    /**
     * @brief 穴として扱えそうな意味単位
     *
     * HoleConnection によって、複数の HoleSection を
     * 1つの穴らしいジオメトリ群として束ねたもの。
     *
     * グラフ構造における連結成分に近い。
     * まだ最終分類済みの正式 HoleFeature ではない。
     */
    struct HoleAssembly
    {
        int index = -1;
        bool isValid = false;

        std::vector<int> sectionIndices;
        std::vector<int> connectionIndices;
        std::vector<int> terminalIndices;

        GeometryRefs geometryRefs;

        std::string explanation;
    };
}

#endif // HOLERECOGNITIONMODEL_H
