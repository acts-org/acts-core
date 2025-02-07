// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Plugins/Geant4/Geant4DetectorElement.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"

#include <memory>
#include <utility>

/// Mockup class
class G4VPhysicalVolume {};

Acts::GeometryContext tContext;

BOOST_AUTO_TEST_SUITE(Geant4Plugin)

BOOST_AUTO_TEST_CASE(Geant4DetectorElement_construction) {
  // G4 phys volume
  auto g4physVol = std::make_shared<G4VPhysicalVolume>();

  // A surface
  auto rBounds = std::make_shared<Acts::RectangleBounds>(10., 10.);
  auto rTransform = Acts::Transform3::Identity();
  rTransform.pretranslate(Acts::Vector3(0., 0., 10));
  auto rSurface = Acts::Surface::makeShared<Acts::PlaneSurface>(
      rTransform, std::move(rBounds));
  // A detector element
  Acts::Geant4DetectorElement g4DetElement(rSurface, *g4physVol.get(),
                                           rTransform, 0.1);

  BOOST_CHECK_EQUAL(g4DetElement.thickness(), 0.1);
  BOOST_CHECK_EQUAL(&g4DetElement.surface(), rSurface.get());
  BOOST_CHECK_EQUAL(&g4DetElement.g4PhysicalVolume(), g4physVol.get());
  BOOST_CHECK(
      g4DetElement.transform(tContext).isApprox(rSurface->transform(tContext)));
}

BOOST_AUTO_TEST_SUITE_END()
