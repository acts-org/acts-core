// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>

#include "Acts/Plugins/GeoModel/GeoModelDetectorElement.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoFullPhysVol.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoMaterial.h>

BOOST_AUTO_TEST_SUITE(GeoModelPlugin)

BOOST_AUTO_TEST_CASE(GeoModelDetectorElementConstruction) {
  auto material = new GeoMaterial("Material", 1.0);
  // Let's create a GeoFullPhysVol object

  // (BOX object)
  auto boxXY = new GeoBox(100, 200, 2);
  auto logXY = new GeoLogVol("LogVolumeXY", boxXY, material);
  auto fphysXY = new GeoFullPhysVol(logXY);
  auto rBounds = std::make_shared<Acts::RectangleBounds>(100, 200);

  PVConstLink physXY{fphysXY};
  auto elementXY =
      Acts::GeoModelDetectorElement::createDetectorElement<Acts::PlaneSurface>(
          physXY, rBounds, Acts::Transform3::Identity(), 2.0);

  const Acts::Surface& surface = elementXY->surface();
  BOOST_CHECK(surface.type() == Acts::Surface::SurfaceType::Plane);
}

BOOST_AUTO_TEST_SUITE_END()
