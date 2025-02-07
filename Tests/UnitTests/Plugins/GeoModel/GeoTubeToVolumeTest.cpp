// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>

// In order to avoid conflicts with declarations in Geomodel that is fixed in
// v3.5
//  clang-formal off
#include "Acts/Plugins/GeoModel/GeoModelDetectorObjectFactory.hpp"
// clang-formal on
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Plugins/GeoModel/GeoModelConverters.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Surfaces/TrapezoidBounds.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"

#include <GeoModelKernel/GeoFullPhysVol.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoMaterial.h>
#include <GeoModelKernel/GeoTrap.h>
#include <GeoModelKernel/GeoTube.h>
#include <GeoModelKernel/GeoVPhysVol.h>

BOOST_AUTO_TEST_SUITE(GeoModelPlugin)

// GeoBox conversion test case
BOOST_AUTO_TEST_CASE(GeoBoxToSensitiveConversion) {
  auto material = make_intrusive<GeoMaterial>("Material", 1.0);
  // Let's create a GeoFullPhysVol object

  // (BOX object) - XY
  std::vector<double> dims = {5, 6, 50};
  auto tube = new GeoTube(dims[0], dims[1], dims[2]);
  auto logTube = new GeoLogVol("Tube", tube, material);
  auto physTube = make_intrusive<GeoFullPhysVol>(logTube);

  // create pars for conversion
  Acts::GeoModelDetectorObjectFactory::Config gmConfig;
  gmConfig.convertBox = {"Tube"};
  Acts::GeometryContext gContext;
  Acts::GeoModelDetectorObjectFactory::Cache gmCache;

  // create factory instance
  Acts::GeoModelDetectorObjectFactory factory(gmConfig);

  factory.convertFpv("Tube", physTube, gmCache, gContext);
  std::shared_ptr<Acts::Experimental::DetectorVolume> volumeTube =
      gmCache.boundingBoxes[0];
  const auto* bounds = dynamic_cast<const Acts::CylinderVolumeBounds*>(
      &volumeTube->volumeBounds());
  std::vector<double> convDims = bounds->values();
  for (std::size_t i = 0; i < dims.size(); i++) {
    BOOST_CHECK(dims[i] == convDims[i]);
  }
}

BOOST_AUTO_TEST_SUITE_END()
