#include "Feature/HoleElementBuilder.h"

#include "Geometry/GeometryModel.h"
#include "Geometry/GeometryTypes.h"
#include "Geometry/SurfaceQuery.h"
#include "Geometry/TopologyQuery.h"

#include <algorithm>
#include <vector>

namespace OccQtCore::Feature
{
    std::vector<HoleElement> HoleElementBuilder::build(const GeometryModel& model) const
    {
        std::vector<HoleElement> elements;

        const auto cylinderFaceIndices = collectCylinderFaceIndices(model);

        const auto components = buildCylinderComponents(model, cylinderFaceIndices);

        elements.reserve(components.size());

        for (const auto& component : components)
        {
            if (component.empty())
            {
                continue;
            }

            HoleElement element;

            element.geometryKind = HoleElementGeometryKind::Cylindrical;

            element.kind = HoleElementKind::Unknown;

            element.faceIndices = component;

            elements.push_back(std::move(element));
        }

        return elements;
    }

    std::vector<int> HoleElementBuilder::collectCylinderFaceIndices(const GeometryModel& model) const
    {
        std::vector<int> faceIndices;

        faceIndices.reserve(static_cast<std::size_t>(model.faceCount()));

        for (int faceIndex = 0; faceIndex < model.faceCount(); ++faceIndex)
        {
            const auto* face = model.faceAt(faceIndex);

            if (face == nullptr)
            {
                continue;
            }

            if (face->info.kind != SurfaceKind::Cylinder)
            {
                continue;
            }

            if (!face->info.cylinder.has_value())
            {
                continue;
            }

            faceIndices.push_back(faceIndex);
        }

        return faceIndices;
    }

    std::vector<std::vector<int>> HoleElementBuilder::buildCylinderComponents(const GeometryModel& model, const std::vector<int>& cylinderFaceIndices) const
    {
        std::vector<std::vector<int>> components;

        // cylinderFaceIndices内のローカル位置を管理する。
        // FaceIndexそのものを添え字にしないことで、モデル全体のFace数に依存しない
        std::vector<bool> visited(cylinderFaceIndices.size(), false);

        for (std::size_t seedLocalIndex = 0; seedLocalIndex < cylinderFaceIndices.size(); ++seedLocalIndex)
        {
            if (visited[seedLocalIndex])
            {
                continue;
            }

            std::vector<int> component;
            std::vector<std::size_t> stack;

            stack.push_back(seedLocalIndex);
            visited[seedLocalIndex] = true;

            while (!stack.empty())
            {
                const std::size_t currentLocalIndex = stack.back();

                stack.pop_back();

                const int currentFaceIndex = cylinderFaceIndices[currentLocalIndex];

                component.push_back(currentFaceIndex);

                for (std::size_t candidateLocalIndex = 0; candidateLocalIndex < cylinderFaceIndices.size(); ++candidateLocalIndex)
                {
                    if (visited[candidateLocalIndex])
                    {
                        continue;
                    }

                    const int candidateFaceIndex = cylinderFaceIndices[candidateLocalIndex];

                    if (!areConnectedCylinderFaces(model, currentFaceIndex, candidateFaceIndex))
                    {
                        continue;
                    }

                    visited[candidateLocalIndex] = true;
                    stack.push_back(candidateLocalIndex);
                }
            }

            std::sort(component.begin(), component.end());

            components.push_back(std::move(component));
        }

        return components;
    }

    bool HoleElementBuilder::areConnectedCylinderFaces(const GeometryModel& model, int lhsFaceIndex, int rhsFaceIndex) const
    {
        if (lhsFaceIndex == rhsFaceIndex)
        {
            return false;
        }

        if (!TopologyQuery::isValidFaceIndex(model, lhsFaceIndex) ||
            !TopologyQuery::isValidFaceIndex(model, rhsFaceIndex))
        {
            return false;
        }

        // 幾何条件：同じ支持円筒上に存在する。
        // トポロジー条件：共有エッジを持ち、直接接続している
        return SurfaceQuery::areOnSameCylinder(model, lhsFaceIndex, rhsFaceIndex) && TopologyQuery::hasSharedEdge(model, lhsFaceIndex, rhsFaceIndex);
    }
}

