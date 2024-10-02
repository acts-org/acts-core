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
#include "Acts/Geometry/CuboidVolumeBounds.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Plugins/GeoModel/GeoModelConverters.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Surfaces/TrapezoidBounds.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoFullPhysVol.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoMaterial.h>
#include <GeoModelKernel/GeoTrap.h>
#include <GeoModelKernel/GeoTrd.h>
#include <GeoModelKernel/GeoVPhysVol.h>

BOOST_AUTO_TEST_SUITE(GeoModelPlugin)

// GeoBox conversion test case
BOOST_AUTO_TEST_CASE(GeoBoxToSensitiveConversion) {
  auto material = make_intrusive<GeoMaterial>("Material", 1.0);
  // Let's create a GeoFullPhysVol object

  // (BOX object) - XY
  std::vector<double> hls = {100, 200, 2};
  auto box = new GeoBox(hls[0], hls[1], hls[2]);
  auto logBox = new GeoLogVol("Box", box, material);
  auto physBox = make_intrusive<GeoFullPhysVol>(logBox);

  // create pars for conversion
  Acts::GeoModelDetectorObjectFactory::Config gmConfig;
  gmConfig.convertBox = {"Box"};
  Acts::GeometryContext gContext;
  Acts::GeoModelDetectorObjectFactory::Cache gmCache;

  // create factory instance
  Acts::GeoModelDetectorObjectFactory factory(gmConfig);

  factory.convertFpv("Box", physBox, gmCache, gContext);
  std::shared_ptr<Acts::Experimental::DetectorVolume> volumeBox =
      gmCache.boundingBoxes[0];
  const auto* bounds =
      dynamic_cast<const Acts::CuboidVolumeBounds*>(&volumeBox->volumeBounds());
  std::vector<Acts::ActsScalar> convHls = bounds->values();
  for (long unsigned int i = 0; i < hls.size(); i++) {
    BOOST_CHECK(hls[i] == convHls[i]);
  }
}

BOOST_AUTO_TEST_SUITE_END()
