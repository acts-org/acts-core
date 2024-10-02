// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

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

Acts::GeometryContext tContext;
Acts::RotationMatrix3 idRotation = Acts::RotationMatrix3::Identity();
Acts::Transform3 idTransform = Acts::Transform3::Identity();

BOOST_AUTO_TEST_SUITE(GeoModelPlugin)

// GeoBox conversion test case
BOOST_AUTO_TEST_CASE(GeoBoxToSensitiveConversion) {
  auto material = make_intrusive<GeoMaterial>("Material", 1.0);
  // Let's create a GeoFullPhysVol object

  // (BOX object) - XY
  auto boxXY = new GeoBox(100, 200, 2);
  auto logXY = new GeoLogVol("LogVolumeXY", boxXY, material);
  auto fphysXY = make_intrusive<GeoFullPhysVol>(logXY);

  PVConstLink physXY{make_intrusive<GeoFullPhysVol>(logXY)};

  auto converted = Acts::GeoBoxConverter{}.toSensitiveSurface(
      physXY, Acts::Transform3::Identity());

  BOOST_CHECK(converted.ok());

  auto [elementXY, surfaceXY] = converted.value();

  BOOST_CHECK(surfaceXY->type() == Acts::Surface::SurfaceType::Plane);

  // Check the bounds
  const Acts::RectangleBounds* rBoundsXY =
      dynamic_cast<const Acts::RectangleBounds*>(&(surfaceXY->bounds()));
  BOOST_CHECK(rBoundsXY != nullptr);
  CHECK_CLOSE_ABS(rBoundsXY->halfLengthX(), 100, 1e-6);
  CHECK_CLOSE_ABS(rBoundsXY->halfLengthY(), 200, 1e-6);

  // Check the transform -> should be identity transform
  const Acts::Transform3& transformXY = surfaceXY->transform(tContext);
  BOOST_CHECK(transformXY.isApprox(idTransform));

  // (BOX object) - YZ
  auto boxYZ = new GeoBox(2, 200, 300);
  auto logYZ = new GeoLogVol("LogVolumeYZ", boxYZ, material);
  auto fphysYZ = make_intrusive<GeoFullPhysVol>(logYZ);

  converted = Acts::GeoBoxConverter{}.toSensitiveSurface(
      fphysYZ, Acts::Transform3::Identity());

  BOOST_CHECK(converted.ok());

  auto [elementYZ, surfaceYZ] = converted.value();

  BOOST_CHECK(surfaceYZ->type() == Acts::Surface::SurfaceType::Plane);
  const Acts::RectangleBounds* rBoundsYZ =
      dynamic_cast<const Acts::RectangleBounds*>(&(surfaceYZ->bounds()));
  BOOST_CHECK(rBoundsYZ != nullptr);
  CHECK_CLOSE_ABS(rBoundsYZ->halfLengthX(), 200, 1e-6);
  CHECK_CLOSE_ABS(rBoundsYZ->halfLengthY(), 300, 1e-6);

  // Check the transform -> should be cyclic permutation of the identity
  const Acts::Transform3& transformYZ = surfaceYZ->transform(tContext);

  Acts::RotationMatrix3 rotationYZ = transformYZ.rotation();
  BOOST_CHECK(rotationYZ.col(0).isApprox(idRotation.col(1)));
  BOOST_CHECK(rotationYZ.col(1).isApprox(idRotation.col(2)));
  BOOST_CHECK(rotationYZ.col(2).isApprox(idRotation.col(0)));

  // (BOX object) - XZ
  auto boxXZ = new GeoBox(400, 2, 300);
  auto logXZ = new GeoLogVol("LogVolumeXZ", boxXZ, material);
  auto fphysXZ = make_intrusive<GeoFullPhysVol>(logXZ);

  converted = Acts::GeoBoxConverter{}.toSensitiveSurface(
      fphysXZ, Acts::Transform3::Identity());

  BOOST_CHECK(converted.ok());

  auto [elementXZ, surfaceXZ] = converted.value();

  BOOST_CHECK(surfaceXZ->type() == Acts::Surface::SurfaceType::Plane);

  // Check the bounds
  const Acts::RectangleBounds* rBoundsXZ =
      dynamic_cast<const Acts::RectangleBounds*>(&(surfaceXZ->bounds()));

  BOOST_CHECK(rBoundsXZ != nullptr);
  CHECK_CLOSE_ABS(rBoundsXZ->halfLengthX(), 300, 1e-6);
  CHECK_CLOSE_ABS(rBoundsXZ->halfLengthY(), 400, 1e-6);

  // Check the transform -> should be cyclic permutation of the identity
  const Acts::Transform3& transformXZ = surfaceXZ->transform(tContext);

  Acts::RotationMatrix3 rotationXZ = transformXZ.rotation();
  BOOST_CHECK(rotationXZ.col(0).isApprox(idRotation.col(2)));
  BOOST_CHECK(rotationXZ.col(1).isApprox(idRotation.col(0)));
  BOOST_CHECK(rotationXZ.col(2).isApprox(idRotation.col(1)));
}

BOOST_AUTO_TEST_SUITE_END()
