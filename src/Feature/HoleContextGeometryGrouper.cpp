#include "Feature/HoleContextGeometryGrouper.h"

#include "Geometry/TopologyQuery.h"
#include "Geometry/GeometryModel.h"
#include "Feature/HoleContextPlanarGroupBuilder.h"
#include "Feature/HoleContextCylindricalGroupBuilder.h"

namespace OccQtCore::Feature
{
    HoleContextGeometryGrouper::HoleContextGeometryGrouper(
        const GeometryModel& model,
        HoleRecognitionWorkingData* workingData)
        : m_model(model)
        , m_workingData(workingData)
    {
    }

    std::vector<HoleContextGeometryGroup>
    HoleContextGeometryGrouper::group() const
    {
        HoleContextCylindricalGroupBuilder cylindricalBuilder(
            m_model,
            m_workingData);

        return cylindricalBuilder.build();
    }

    std::vector<HoleContextGeometryGroup>
    HoleContextGeometryGrouper::group(
        const GeometryRefs& geometryRefs) const
    {
        const auto buckets =
            bucketFacesBySurfaceKind(geometryRefs);

        std::vector<HoleContextGeometryGroup> groups;

        if (!buckets.planeFaceIndices.empty())
        {
            HoleContextPlanarGroupBuilder planarBuilder(m_model);

            appendGroups(
                groups,
                planarBuilder.build(
                    buckets.planeFaceIndices));
        }

        if (!buckets.cylinderFaceIndices.empty())
        {
            HoleContextCylindricalGroupBuilder cylindricalBuilder(
                m_model,
                m_workingData);

            appendGroups(
                groups,
                cylindricalBuilder.build(
                    buckets.cylinderFaceIndices));
        }

        return groups;
    }

    HoleContextGeometryGrouper::SurfaceKindBuckets
    HoleContextGeometryGrouper::bucketFacesBySurfaceKind(
        const GeometryRefs& geometryRefs) const
    {
        SurfaceKindBuckets buckets;

        for (const int faceIndex : geometryRefs.faceIndices)
        {
            if (!TopologyQuery::isValidFaceIndex(m_model, faceIndex))
            {
                continue;
            }

            const auto* face = m_model.faceAt(faceIndex);

            if (face == nullptr)
            {
                continue;
            }

            switch (face->info.kind)
            {
            case SurfaceKind::Plane:
                buckets.planeFaceIndices.push_back(faceIndex);
                break;

            case SurfaceKind::Cylinder:
                buckets.cylinderFaceIndices.push_back(faceIndex);
                break;

            case SurfaceKind::Cone:
                buckets.coneFaceIndices.push_back(faceIndex);
                break;

            case SurfaceKind::Torus:
                buckets.torusFaceIndices.push_back(faceIndex);
                break;

            default:
                buckets.otherFaceIndices.push_back(faceIndex);
                break;
            }
        }

        return buckets;
    }

    void HoleContextGeometryGrouper::appendGroups(
        std::vector<HoleContextGeometryGroup>& destination,
        std::vector<HoleContextGeometryGroup> source) const
    {
        destination.insert(
            destination.end(),
            source.begin(),
            source.end());
    }
}
