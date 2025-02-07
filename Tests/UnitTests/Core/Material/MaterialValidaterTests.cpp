// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Material/HomogeneousSurfaceMaterial.hpp"
#include "Acts/Material/MaterialInteraction.hpp"
#include "Acts/Material/MaterialMapper.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Material/MaterialValidater.hpp"
#include "Acts/Material/interface/IAssignmentFinder.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"

#include <limits>
#include <numbers>

namespace Acts::Test {

auto tContext = GeometryContext();

/// @brief Interface for the material mapping that seeks the possible
/// assignment candidates for the material interactiosn
class IntersectSurfacesFinder : public IAssignmentFinder {
 public:
  std::vector<const Surface*> surfaces;

  /// @brief Interface method for generating assignment candidates for the
  /// material interaction assignment to surfaces or volumes
  ///
  /// @param gctx is the geometry context
  /// @param mctx is the magnetic field context
  /// @param position is the position of the initial ray
  /// @param direction is the direction of initial ray
  ///
  /// @return a vector of Surface Assignments and Volume Assignments
  std::pair<std::vector<IAssignmentFinder::SurfaceAssignment>,
            std::vector<IAssignmentFinder::VolumeAssignment>>
  assignmentCandidates(const GeometryContext& gctx,
                       const MagneticFieldContext& /*ignored*/,
                       const Vector3& position,
                       const Vector3& direction) const override {
    std::vector<IAssignmentFinder::SurfaceAssignment> surfaceAssignments;
    std::vector<IAssignmentFinder::VolumeAssignment> volumeAssignments;
    // Intersect the surfaces
    for (auto& surface : surfaces) {
      // Get the intersection
      auto sMultiIntersection = surface->intersect(gctx, position, direction,
                                                   BoundaryTolerance::None());
      // One solution, take it
      if (sMultiIntersection.size() == 1u &&
          sMultiIntersection[0u].status() >=
              Acts::IntersectionStatus::reachable &&
          sMultiIntersection[0u].pathLength() >= 0.0) {
        surfaceAssignments.push_back(
            {surface, sMultiIntersection[0u].position(), direction});
        continue;
      }
      if (sMultiIntersection.size() > 1u) {
        // Multiple intersections, take the closest
        auto closestForward = sMultiIntersection.closestForward();
        if (closestForward.status() >= Acts::IntersectionStatus::reachable &&
            closestForward.pathLength() > 0.0) {
          surfaceAssignments.push_back(
              {surface, closestForward.position(), direction});
          continue;
        }
      }
    }
    return {surfaceAssignments, volumeAssignments};
  }
};

BOOST_AUTO_TEST_SUITE(MAterialValidatorTestSuite)

BOOST_AUTO_TEST_CASE(MaterialValidaterFlowTest) {
  auto cylinder0 =
      Surface::makeShared<CylinderSurface>(Transform3::Identity(), 20, 100);
  auto cylinder1 =
      Surface::makeShared<CylinderSurface>(Transform3::Identity(), 40, 100);
  auto cylinder2 =
      Surface::makeShared<CylinderSurface>(Transform3::Identity(), 60, 100);

  auto material0 = std::make_shared<HomogeneousSurfaceMaterial>(MaterialSlab(
      Acts::Material::fromMolarDensity(21.0, 22.0, 23.0, 24.0, 25.0), 2.0));
  auto material1 = std::make_shared<HomogeneousSurfaceMaterial>(MaterialSlab(
      Acts::Material::fromMolarDensity(41.0, 42.0, 43.0, 44.0, 45.0), 4.0));
  auto material2 = std::make_shared<HomogeneousSurfaceMaterial>(MaterialSlab(
      Acts::Material::fromMolarDensity(61.0, 62.0, 63.0, 64.0, 65.0), 6.0));

  cylinder0->assignSurfaceMaterial(material0);
  cylinder1->assignSurfaceMaterial(material1);
  cylinder2->assignSurfaceMaterial(material2);

  auto materialAssinger = std::make_shared<IntersectSurfacesFinder>();
  materialAssinger->surfaces = {cylinder0.get(), cylinder1.get(),
                                cylinder2.get()};

  MaterialValidater::Config mvConfig;
  mvConfig.materialAssigner = materialAssinger;

  auto materialValidater = MaterialValidater(
      mvConfig, getDefaultLogger("MaterialValidater", Logging::VERBOSE));

  // Test one central ray
  auto [posDir, rMaterial] = materialValidater.recordMaterial(
      tContext, MagneticFieldContext(), Vector3(0, 0, 0), Vector3(1, 0, 0));

  BOOST_CHECK(posDir.first == Vector3(0, 0, 0));
  BOOST_CHECK(posDir.second == Vector3(1, 0, 0));
  CHECK_CLOSE_ABS(rMaterial.materialInX0, 2. / 21. + 4. / 41. + 6. / 61., 1e-6);
  CHECK_CLOSE_ABS(rMaterial.materialInL0, 2. / 22. + 4. / 42. + 6. / 62., 1e-6);
  BOOST_CHECK_EQUAL(rMaterial.materialInteractions.size(), 3u);

  // Test a ray at 45 degrees
  auto [posDir2, rMaterial2] = materialValidater.recordMaterial(
      tContext, MagneticFieldContext(), Vector3(0, 0, 0),
      Vector3(1, 0, 1).normalized());

  double pathCorrection = std::numbers::sqrt2;
  CHECK_CLOSE_ABS(rMaterial2.materialInX0,
                  pathCorrection * (2. / 21. + 4. / 41. + 6. / 61.), 1e-6);
  CHECK_CLOSE_ABS(rMaterial2.materialInL0,
                  pathCorrection * (2. / 22. + 4. / 42. + 6. / 62.), 1e-6);
  BOOST_CHECK_EQUAL(rMaterial2.materialInteractions.size(), 3u);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
