// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Plugins/TGeo/TGeoSurfaceConverter.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Visualization/GeometryView3D.hpp"
#include "Acts/Visualization/ObjVisualization3D.hpp"
#include "Acts/Visualization/ViewConfig.hpp"

#include <memory>
#include <utility>

#include "TGeoBBox.h"
#include "TGeoManager.h"
#include "TGeoMaterial.h"
#include "TGeoMatrix.h"
#include "TGeoMedium.h"
#include "TGeoVolume.h"
#include "TView.h"

namespace Acts::Test {

GeometryContext tgContext = GeometryContext();

ViewConfig red{.color = {200, 0, 0}};
ViewConfig green{.color = {0, 200, 0}};
ViewConfig blue{.color = {0, 0, 200}};

/// @brief Unit test to convert a Bbox into a Plane
///
/// This test also tests:
/// * the "(x/X)(y/Y)(z/Z)" orientations
/// * the scaling functionality
BOOST_AUTO_TEST_CASE(TGeoBBox_to_PlaneSurface) {
  ObjVisualization3D objVis;

  // BBox is defined [-dX,dX] x [-dY,dY] x [-dZ,dZ]
  double dX = 10.;
  double dY = 30.;
  double dZ = 1.;

  new TGeoManager("box", "poza1");
  TGeoMaterial *mat = new TGeoMaterial("Al", 26.98, 13, 2.7);
  TGeoMedium *med = new TGeoMedium("MED", 1, mat);
  TGeoVolume *top = gGeoManager->MakeBox("TOP", med, 100, 100, 100);
  gGeoManager->SetTopVolume(top);
  TGeoVolume *vol = gGeoManager->MakeBox("BOX", med, dX, dY, dZ);
  vol->SetLineWidth(2);
  top->AddNode(vol, 1);
  gGeoManager->CloseGeometry();

  // Upper case ---------------------------------
  auto [plane_XYZ, thickness_XYZ] = TGeoSurfaceConverter::toSurface(
      *vol->GetShape(), *gGeoIdentity, "XY*", 1);
  BOOST_REQUIRE_NE(plane_XYZ, nullptr);
  BOOST_CHECK_EQUAL(plane_XYZ->type(), Surface::Plane);
  CHECK_CLOSE_ABS(thickness_XYZ, 2 * dZ, s_epsilon);

  auto bounds_XYZ =
      dynamic_cast<const RectangleBounds *>(&(plane_XYZ->bounds()));
  BOOST_REQUIRE_NE(bounds_XYZ, nullptr);
  double maxX = bounds_XYZ->get(RectangleBounds::eMaxX);
  double minX = bounds_XYZ->get(RectangleBounds::eMinX);
  double maxY = bounds_XYZ->get(RectangleBounds::eMaxY);
  double minY = bounds_XYZ->get(RectangleBounds::eMinY);
  CHECK_CLOSE_ABS(maxX - minX, 2 * dX, s_epsilon);
  CHECK_CLOSE_ABS(maxY - minY, 2 * dY, s_epsilon);

  // Check if the surface is the (negative) identity
  auto transform_XYZ = plane_XYZ->transform(tgContext);
  auto rotation_XYZ = transform_XYZ.rotation();
  BOOST_CHECK(transform_XYZ.isApprox(Transform3::Identity()));

  const Vector3 offset_XYZ{-5.5 * dX, 0., 0.};
  GeometryView3D::drawSurface(objVis, *plane_XYZ, tgContext,
                              Transform3(Translation3{offset_XYZ}));
  const Vector3 center_XYZ = plane_XYZ->center(tgContext) + offset_XYZ;
  GeometryView3D::drawArrowForward(
      objVis, center_XYZ,
      center_XYZ + 0.6 * (maxX - minX) * rotation_XYZ.col(0), 4., 2.5, red);
  GeometryView3D::drawArrowForward(
      objVis, center_XYZ,
      center_XYZ + 0.6 * (maxY - minY) * rotation_XYZ.col(1), 4., 2.5, green);
  GeometryView3D::drawArrowForward(
      objVis, center_XYZ, center_XYZ + 2 * rotation_XYZ.col(2), 4., 2.5, blue);

  // Lower case ---------------------------------
  auto [plane_xyz, thickness_xyz] = TGeoSurfaceConverter::toSurface(
      *vol->GetShape(), *gGeoIdentity, "xy*", 1);
  BOOST_CHECK_NE(plane_xyz, nullptr);
  BOOST_CHECK_EQUAL(plane_xyz->type(), Surface::Plane);
  CHECK_CLOSE_ABS(thickness_xyz, 2 * dZ, s_epsilon);

  auto bounds_xyz =
      dynamic_cast<const RectangleBounds *>(&(plane_XYZ->bounds()));
  BOOST_REQUIRE_NE(bounds_xyz, nullptr);
  BOOST_CHECK_EQUAL(bounds_xyz, bounds_XYZ);
  auto transform_xyz = plane_xyz->transform(tgContext);
  auto rotation_xyz = transform_xyz.rotation();
  BOOST_CHECK(rotation_xyz.col(0).isApprox(-1 * rotation_XYZ.col(0)));
  BOOST_CHECK(rotation_xyz.col(1).isApprox(-1 * rotation_XYZ.col(1)));
  BOOST_CHECK(rotation_xyz.col(2).isApprox(rotation_XYZ.col(2)));

  const Vector3 offset_xyz{-2 * dX, 0., 0.};
  GeometryView3D::drawSurface(objVis, *plane_xyz, tgContext,
                              Transform3(Translation3{offset_xyz}));
  const Vector3 center_xyz = plane_xyz->center(tgContext) + offset_xyz;
  GeometryView3D::drawArrowForward(
      objVis, center_xyz,
      center_xyz + 0.6 * (maxX - minX) * rotation_xyz.col(0), 4., 2.5, red);
  GeometryView3D::drawArrowForward(
      objVis, center_xyz,
      center_xyz + 0.6 * (maxY - minY) * rotation_xyz.col(1), 4., 2.5, green);
  GeometryView3D::drawArrowForward(
      objVis, center_xyz, center_xyz + 2 * rotation_xyz.col(2), 4., 2.5, blue);

  // Mixed case ---------------------------------
  auto [plane_xYz, thickness_xYz] = TGeoSurfaceConverter::toSurface(
      *vol->GetShape(), *gGeoIdentity, "xY*", 1);
  BOOST_REQUIRE_NE(plane_xYz, nullptr);
  BOOST_CHECK_EQUAL(plane_xYz->type(), Surface::Plane);
  CHECK_CLOSE_ABS(thickness_xYz, 2 * dZ, s_epsilon);

  auto bounds_xYz =
      dynamic_cast<const RectangleBounds *>(&(plane_xYz->bounds()));
  BOOST_CHECK_NE(bounds_xYz, nullptr);
  BOOST_CHECK_EQUAL(bounds_xYz, bounds_xYz);
  auto transform_xYz = plane_xYz->transform(tgContext);
  auto rotation_xYz = transform_xYz.rotation();
  BOOST_CHECK(rotation_xYz.col(0).isApprox(-1 * rotation_XYZ.col(0)));
  BOOST_CHECK(rotation_xYz.col(1).isApprox(rotation_XYZ.col(1)));
  BOOST_CHECK(rotation_xYz.col(2).isApprox(-1. * rotation_XYZ.col(2)));

  const Vector3 offset_xYz{2 * dX, 0., 0.};
  GeometryView3D::drawSurface(
      objVis, *plane_xYz, tgContext,
      Translation3{offset_xYz} * Transform3::Identity());
  const Vector3 center_xYz = plane_xYz->center(tgContext) + offset_xYz;
  GeometryView3D::drawArrowForward(
      objVis, center_xYz,
      center_xYz + 0.6 * (maxX - minX) * rotation_xYz.col(0), 4., 2.5, red);
  GeometryView3D::drawArrowForward(
      objVis, center_xYz,
      center_xYz + 0.6 * (maxY - minY) * rotation_xYz.col(1), 4., 2.5, green);
  GeometryView3D::drawArrowForward(
      objVis, center_xYz, center_xYz + 2 * rotation_xYz.col(2), 4., 2.5, blue);

  // Swap case --------------------------------- (x/y) here
  auto [plane_YXz, thickness_YXz] = TGeoSurfaceConverter::toSurface(
      *vol->GetShape(), *gGeoIdentity, "YX*", 1);
  BOOST_REQUIRE_NE(plane_YXz, nullptr);
  BOOST_CHECK_EQUAL(plane_YXz->type(), Surface::Plane);
  CHECK_CLOSE_ABS(thickness_YXz, 2 * dZ, s_epsilon);

  auto bounds_YXz =
      dynamic_cast<const RectangleBounds *>(&(plane_YXz->bounds()));
  maxX = bounds_YXz->get(RectangleBounds::eMaxX);
  minX = bounds_YXz->get(RectangleBounds::eMinX);
  maxY = bounds_YXz->get(RectangleBounds::eMaxY);
  minY = bounds_YXz->get(RectangleBounds::eMinY);
  CHECK_CLOSE_ABS(maxX - minX, 2 * dY, s_epsilon);
  CHECK_CLOSE_ABS(maxY - minY, 2 * dX, s_epsilon);

  auto transform_YXz = plane_YXz->transform(tgContext);
  auto rotation_YXz = transform_YXz.rotation();
  BOOST_CHECK(rotation_YXz.col(0).isApprox(rotation_XYZ.col(1)));
  BOOST_CHECK(rotation_YXz.col(1).isApprox(rotation_XYZ.col(0)));
  BOOST_CHECK(rotation_YXz.col(2).isApprox(-1. * rotation_XYZ.col(2)));

  const Vector3 offset_YXz{5.5 * dX, 0., 0.};
  GeometryView3D::drawSurface(objVis, *plane_YXz, tgContext,
                              Transform3(Translation3{offset_YXz}));
  const Vector3 center_YXz = plane_YXz->center(tgContext) + offset_YXz;
  GeometryView3D::drawArrowForward(
      objVis, center_YXz,
      center_YXz + 0.6 * (maxX - minX) * rotation_YXz.col(0), 4., 2.5, red);
  GeometryView3D::drawArrowForward(
      objVis, center_YXz,
      center_YXz + 0.6 * (maxY - minY) * rotation_YXz.col(1), 4., 2.5, green);
  GeometryView3D::drawArrowForward(
      objVis, center_YXz, center_YXz + 2 * rotation_YXz.col(2), 4., 2.5, blue);

  // Scaling test ---------------------------------
  auto [plane_XYZ10, thickness_XYZ10] = TGeoSurfaceConverter::toSurface(
      *vol->GetShape(), *gGeoIdentity, "xY*", 10);
  BOOST_CHECK_NE(plane_XYZ10, nullptr);
  CHECK_CLOSE_ABS(thickness_XYZ10, 20 * dZ, s_epsilon);

  auto bounds_XYZ10 =
      dynamic_cast<const RectangleBounds *>(&(plane_XYZ10->bounds()));
  double maxX10 = bounds_XYZ10->get(RectangleBounds::eMaxX);
  double minX10 = bounds_XYZ10->get(RectangleBounds::eMinX);
  double maxY10 = bounds_XYZ10->get(RectangleBounds::eMaxY);
  double minY10 = bounds_XYZ10->get(RectangleBounds::eMinY);
  CHECK_CLOSE_ABS(maxX10 - minX10, 20 * dX, s_epsilon);
  CHECK_CLOSE_ABS(maxY10 - minY10, 20 * dY, s_epsilon);

  objVis.write("TGeoConversion_TGeoBBox_PlaneSurface");
}

}  // namespace Acts::Test
