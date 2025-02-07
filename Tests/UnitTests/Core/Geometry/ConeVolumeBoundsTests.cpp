// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/ConeVolumeBounds.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Surfaces/Surface.hpp"

#include <cmath>
#include <memory>
#include <numbers>
#include <utility>
#include <vector>

using namespace Acts::UnitLiterals;

namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(VolumeBounds)

BOOST_AUTO_TEST_CASE(ConeVolumeBoundsTests) {
  // Single solid Cone
  ConeVolumeBounds solidCone(0., 0., 0.45, 50_mm, 50_mm, 0., std::numbers::pi);

  // Test correct parameter return
  BOOST_CHECK_EQUAL(solidCone.get(ConeVolumeBounds::eInnerAlpha), 0.);
  BOOST_CHECK_EQUAL(solidCone.get(ConeVolumeBounds::eInnerOffsetZ), 0.);
  BOOST_CHECK_EQUAL(solidCone.get(ConeVolumeBounds::eOuterAlpha), 0.45);
  BOOST_CHECK_EQUAL(solidCone.get(ConeVolumeBounds::eOuterOffsetZ), 50.);
  BOOST_CHECK_EQUAL(solidCone.get(ConeVolumeBounds::eHalfLengthZ), 50.);
  BOOST_CHECK_EQUAL(solidCone.get(ConeVolumeBounds::eAveragePhi), 0.);
  BOOST_CHECK_EQUAL(solidCone.get(ConeVolumeBounds::eHalfPhiSector),
                    std::numbers::pi);
  // Derived quantities
  BOOST_CHECK_EQUAL(solidCone.innerTanAlpha(), 0.);
  BOOST_CHECK_EQUAL(solidCone.innerRmin(), 0.);
  BOOST_CHECK_EQUAL(solidCone.innerRmax(), 0.);
  BOOST_CHECK_EQUAL(solidCone.outerTanAlpha(), std::tan(0.45));

  double outerRmax = 100_mm * solidCone.outerTanAlpha();
  BOOST_CHECK_EQUAL(solidCone.outerRmin(), 0.);
  BOOST_CHECK_EQUAL(solidCone.outerRmax(), outerRmax);

  auto solidConeSurfaces = solidCone.orientedSurfaces();
  BOOST_CHECK_EQUAL(solidConeSurfaces.size(), 2);

  // Single solid Cone - with cut off
  ConeVolumeBounds cutOffCone(0., 0., 0.45, 80_mm, 50_mm, 0., std::numbers::pi);
  auto cutOffConeSurfaces = cutOffCone.orientedSurfaces();
  BOOST_CHECK_EQUAL(cutOffConeSurfaces.size(), 3);

  // Cone - Cone inlay
  ConeVolumeBounds cutOffHollowCone(0.35, 70_mm, 0.45, 80_mm, 50_mm, 0.,
                                    std::numbers::pi);
  auto cutOffHollowConeSurfaces = cutOffHollowCone.orientedSurfaces();
  BOOST_CHECK_EQUAL(cutOffHollowConeSurfaces.size(), 4);

  // Sectoral Cone - Cone inlay
  ConeVolumeBounds cutOffHollowSectoralCone(0.35, 70_mm, 0.45, 80_mm, 50_mm, 0.,
                                            0.456);
  auto cutOffHollowSectoralConeSurfaces =
      cutOffHollowSectoralCone.orientedSurfaces();
  BOOST_CHECK_EQUAL(cutOffHollowSectoralConeSurfaces.size(), 6);

  // Sectoral Cone - Hollow Cone
  ConeVolumeBounds cutOffHollowCylCone(10_mm, 0.45, 80_mm, 50_mm, 0.,
                                       std::numbers::pi);
  auto cutOffHollowCylConeSurfaces = cutOffHollowCylCone.orientedSurfaces();
  BOOST_CHECK_EQUAL(cutOffHollowCylConeSurfaces.size(), 4);

  // Single Hollow Cylinder - Cone inlay
  ConeVolumeBounds cutOffHollowConeCyl(120_mm, 0.35, 70_mm, 50_mm, 0.,
                                       std::numbers::pi);
  auto cutOffHollowConeCylSurfaces = cutOffHollowConeCyl.orientedSurfaces();
  BOOST_CHECK_EQUAL(cutOffHollowConeCylSurfaces.size(), 4);
}

BOOST_AUTO_TEST_CASE(ConeVolumeBoundsSurfaceOrientation) {
  ConeVolumeBounds hcone(10_mm, 0.45, 80_mm, 50_mm, 0., std::numbers::pi);

  auto cvbOrientedSurfaces = hcone.orientedSurfaces(Transform3::Identity());
  BOOST_CHECK_EQUAL(cvbOrientedSurfaces.size(), 4);

  auto geoCtx = GeometryContext();
  Vector3 xaxis(1., 0., 0.);
  Vector3 yaxis(0., 1., 0.);
  Vector3 zaxis(0., 0., 1.);

  for (auto& os : cvbOrientedSurfaces) {
    // Test the orientation of the boundary surfaces
    auto rot = os.surface->transform(geoCtx).rotation();
    BOOST_CHECK(rot.col(0).isApprox(xaxis));
    BOOST_CHECK(rot.col(1).isApprox(yaxis));
    BOOST_CHECK(rot.col(2).isApprox(zaxis));
  }
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
