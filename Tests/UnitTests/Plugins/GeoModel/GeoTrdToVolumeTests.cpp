// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

// switching format off to avoid conflicting declaration in GeoModel
// needed until Acts GeoModel bumps to 6.5
//clang-format off
#include "Acts/Plugins/GeoModel/GeoModelDetectorObjectFactory.hpp"
//clang-format on
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/TrapezoidVolumeBounds.hpp"
#include "Acts/Plugins/GeoModel/GeoModelConverters.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Surfaces/TrapezoidBounds.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"

#include <GeoModelKernel/GeoFullPhysVol.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoMaterial.h>
#include <GeoModelKernel/GeoTrd.h>

BOOST_AUTO_TEST_SUITE(GeoModelPlugin)

// GeoBox conversion test case
BOOST_AUTO_TEST_CASE(GeoTrdToVolumeConversion) {
  auto material = new GeoMaterial("Material", 1.0);
  // Let's create a GeoFullPhysVol object
  double geoHlX1 = 2, geoHlX2 = 2, geoHlY1 = 50, geoHlY2 = 80, geoHlZ = 60;
  auto trd = new GeoTrd(geoHlX1, geoHlX1, geoHlY1, geoHlY2, geoHlZ);
  auto logTrd = new GeoLogVol("Trd", trd, material);
  auto physTrd = make_intrusive<GeoFullPhysVol>(logTrd);

  // this should produce an error while converting
  auto errTrd = new GeoTrd(2, 3, 25, 40, 30);
  auto errLogTrd = new GeoLogVol("Trd", errTrd, material);
  auto errPhysTrd = make_intrusive<GeoFullPhysVol>(errLogTrd);

  // create pars for conversion
  Acts::GeoModelDetectorObjectFactory::Config gmConfig;
  gmConfig.convertBox = {"Trd"};
  Acts::GeometryContext gContext;
  Acts::GeoModelDetectorObjectFactory::Cache gmCache;
  Acts::GeoModelDetectorObjectFactory::Cache errCache;

  // create factory instance
  Acts::GeoModelDetectorObjectFactory factory(gmConfig);

  // test error case
  BOOST_CHECK_THROW(factory.convertFpv("Trd", errPhysTrd, errCache, gContext),
                    std::runtime_error);
  factory.convertFpv("Trd", physTrd, gmCache, gContext);
  std::shared_ptr<Acts::Experimental::DetectorVolume> volumeTrd =
      gmCache.boundingBoxes[0];
  const auto* bounds = dynamic_cast<const Acts::TrapezoidVolumeBounds*>(
      &volumeTrd->volumeBounds());
  std::vector<Acts::ActsScalar> convHls = bounds->values();
  // note: GeoTrd and Acts use different coordinates
  BOOST_CHECK(geoHlX1 == convHls[3]);
  BOOST_CHECK(geoHlX2 == convHls[3]);
  BOOST_CHECK(geoHlY1 == convHls[0]);
  BOOST_CHECK(geoHlY2 == convHls[1]);
  BOOST_CHECK(geoHlZ == convHls[2]);
}

BOOST_AUTO_TEST_SUITE_END()
