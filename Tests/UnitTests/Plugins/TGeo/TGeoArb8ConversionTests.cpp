// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Plugins/TGeo/TGeoSurfaceConverter.hpp"
#include "Acts/Surfaces/ConvexPolygonBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Visualization/GeometryView3D.hpp"
#include "Acts/Visualization/ObjVisualization3D.hpp"
#include "Acts/Visualization/ViewConfig.hpp"

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "TGeoArb8.h"
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

/// @brief Unit test to convert a TGeoTrd2 into a Plane
///
/// * The TGeoTrd2 has x/z orientation
BOOST_AUTO_TEST_CASE(TGeoArb8_to_PlaneSurface) {
  ObjVisualization3D objVis;

  new TGeoManager("arb8", "poza12");
  TGeoMaterial *mat = new TGeoMaterial("Al", 26.98, 13, 2.7);
  TGeoMedium *med = new TGeoMedium("MED", 1, mat);
  TGeoVolume *top = gGeoManager->MakeBox("TOP", med, 100, 100, 100);
  gGeoManager->SetTopVolume(top);
  // The one parameter at construction is dZ, when the
  // TGeoArb8 is spanning from -dZ to +dZ:
  // - hence, the thickness is 2 * dZ
  ActsScalar dZ = 1.;
  TGeoArb8 *arb = new TGeoArb8(dZ);
  arb->SetVertex(0, -30, -25);
  arb->SetVertex(1, -25, 25);
  arb->SetVertex(2, 5, 25);
  arb->SetVertex(3, 25, -25);
  arb->SetVertex(4, -30, -25);
  arb->SetVertex(5, -25, 25);
  arb->SetVertex(6, 5, 25);
  arb->SetVertex(7, 25, -25);
  TGeoVolume *vol = new TGeoVolume("ARB8", arb, med);
  top->AddNode(vol, 1);
  gGeoManager->CloseGeometry();

  // Check the 4 possible ways
  std::vector<std::string> allowedAxes = {"XY*", "xy*", "Xy*", "xY*",
                                          "YX*", "yx*", "Yx*", "yX*"};

  std::size_t iarb8 = 0;
  for (const auto &axes : allowedAxes) {
    auto [plane, thickness] = TGeoSurfaceConverter::toSurface(
        *vol->GetShape(), *gGeoIdentity, axes, 1);
    BOOST_REQUIRE_NE(plane, nullptr);
    BOOST_CHECK_EQUAL(plane->type(), Surface::Plane);
    BOOST_CHECK_EQUAL(thickness, dZ * 2.);

    auto bounds =
        dynamic_cast<const ConvexPolygonBounds<4> *>(&(plane->bounds()));
    BOOST_CHECK_NE(bounds, nullptr);

    // Check if the surface is the (negative) identity
    auto transform = plane->transform(tgContext);
    auto rotation = transform.rotation();
    GeometryView3D::drawSurface(objVis, *plane, tgContext);
    const Vector3 center = plane->center(tgContext);
    GeometryView3D::drawArrowForward(
        objVis, center, center + 30 * rotation.col(0), 4., 2.5, red);
    GeometryView3D::drawArrowForward(
        objVis, center, center + 30 * rotation.col(1), 4., 2.5, green);
    GeometryView3D::drawArrowForward(
        objVis, center, center + 2 * rotation.col(2), 4., 2.5, blue);

    objVis.write("TGeoConversion_TGeoArb8_PlaneSurface_" +
                 std::to_string(iarb8++));
    objVis.clear();
  }

  // Check exceptions for not allowed axis definition
  std::vector<std::string> notAllowed = {
      "XZ*", "xz*", "xZ*", "Xz*", "ZX*", "zx*", "zX*", "Zx*",
      "YZ*", "yz*", "yZ*", "Yz*", "ZY*", "zy*", "Zy*", "zY*"};
  for (const auto &naxis : notAllowed) {
    BOOST_CHECK_THROW(TGeoSurfaceConverter::toSurface(*vol->GetShape(),
                                                      *gGeoIdentity, naxis, 1),
                      std::invalid_argument);
  }
}

}  // namespace Acts::Test
