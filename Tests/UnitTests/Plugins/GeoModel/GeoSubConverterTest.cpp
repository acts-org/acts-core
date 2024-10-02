// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

//clang-format off
#include "Acts/Plugins/GeoModel/GeoModelDetectorObjectFactory.hpp"
//clang-format on
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Plugins/GeoModel/GeoModelConverters.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Surfaces/TrapezoidBounds.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"

#include <GeoModelKernel/GeoFullPhysVol.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoMaterial.h>
#include <GeoModelKernel/GeoShapeSubtraction.h>
#include <GeoModelKernel/GeoTrap.h>
#include <GeoModelKernel/GeoTrd.h>
#include <GeoModelKernel/GeoVPhysVol.h>

Acts::GeometryContext tContext;
Acts::RotationMatrix3 idRotation = Acts::RotationMatrix3::Identity();
Acts::Transform3 idTransform = Acts::Transform3::Identity();

BOOST_AUTO_TEST_SUITE(GeoModelPlugin)

// GeoBox conversion test case
BOOST_AUTO_TEST_CASE(GeoSubToSensitiveConversion) {
  GeoIntrusivePtr<GeoMaterial> material(new GeoMaterial("Material", 1.0));

  // BOX object
  double hlX = 200, hlY = 100, hlZ = 2;
  GeoIntrusivePtr<GeoBox> shapeA(new GeoBox(hlX, hlY, hlZ));
  // Trapezoid object
  GeoIntrusivePtr<GeoTrd> shapeB(new GeoTrd(2, 2, 50, 80, 60));

  // create subtraction
  GeoIntrusivePtr<GeoShapeSubtraction> geoSub(
      new GeoShapeSubtraction(shapeA, shapeB));
  GeoIntrusivePtr<GeoLogVol> logSub(
      new GeoLogVol("LogVolume", geoSub, material));
  auto fphysSub = make_intrusive<GeoFullPhysVol>(logSub);

  // create pars for conversion
  Acts::GeoModelDetectorObjectFactory::Config gmConfig;
  Acts::GeometryContext gContext;
  Acts::GeoModelDetectorObjectFactory::Cache subCache;

  // create factory instance
  Acts::GeoModelDetectorObjectFactory factory(gmConfig);

  // convert GeoFullPhysVol (to surfaces)
  factory.convertFpv("Sub", fphysSub, subCache, gContext);

  Acts::GeoModelSensitiveSurface subSensSurface = subCache.sensitiveSurfaces[0];
  std::shared_ptr<Acts::Surface> subSurface = std::get<1>(subSensSurface);
  const auto* subBounds =
      dynamic_cast<const Acts::RectangleBounds*>(&subSurface->bounds());
  BOOST_CHECK(subBounds->halfLengthX() == hlX);
  BOOST_CHECK(subBounds->halfLengthY() == hlY);
}
BOOST_AUTO_TEST_SUITE_END()
