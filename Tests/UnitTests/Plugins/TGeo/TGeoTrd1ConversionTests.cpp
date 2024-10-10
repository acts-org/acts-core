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
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Surfaces/TrapezoidBounds.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Visualization/GeometryView3D.hpp"
#include "Acts/Visualization/ObjVisualization3D.hpp"
#include "Acts/Visualization/ViewConfig.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "TGeoManager.h"
#include "TGeoMaterial.h"
#include "TGeoMatrix.h"
#include "TGeoMedium.h"
#include "TGeoTrd1.h"
#include "TGeoVolume.h"
#include "TView.h"

namespace Acts::Test {

GeometryContext tgContext = GeometryContext();

ViewConfig red{.color = {200, 0, 0}};
ViewConfig green{.color = {0, 200, 0}};
ViewConfig blue{.color = {0, 0, 200}};

/// @brief Unit test to convert a TGeoTrd1 into a Plane
///
/// * The TGeoTrd1 can only have (x/X)(z/Z) orientation
BOOST_AUTO_TEST_CASE(TGeoTrd1_to_PlaneSurface) {
  ObjVisualization3D objVis;

  double hxmin = 10.;
  double hxmax = 30.;
  double ht = 1.;  // half thickness
  double hy = 40.;

  new TGeoManager("trd1", "poza9");
  TGeoMaterial *mat = new TGeoMaterial("Al", 26.98, 13, 2.7);
  TGeoMedium *med = new TGeoMedium("MED", 1, mat);
  TGeoVolume *top = gGeoManager->MakeBox("TOP", med, 100, 100, 100);
  gGeoManager->SetTopVolume(top);
  TGeoVolume *vol = gGeoManager->MakeTrd1("Trd1", med, hxmin, hxmax, ht, hy);
  gGeoManager->CloseGeometry();

  // Check the 4 possible ways
  std::vector<std::string> allowedAxes = {"XZ*", "xZ*", "xz*", "Xz*"};

  std::size_t itrd = 0;
  for (const auto &axes : allowedAxes) {
    auto [plane, thickness] = TGeoSurfaceConverter::toSurface(
        *vol->GetShape(), *gGeoIdentity, axes, 1);
    BOOST_REQUIRE_NE(plane, nullptr);
    BOOST_CHECK_EQUAL(plane->type(), Surface::Plane);
    CHECK_CLOSE_ABS(thickness, 2 * ht, s_epsilon);

    auto bounds = dynamic_cast<const TrapezoidBounds *>(&(plane->bounds()));
    BOOST_REQUIRE_NE(bounds, nullptr);
    double hXminY = bounds->get(TrapezoidBounds::eHalfLengthXnegY);
    double hXmaxY = bounds->get(TrapezoidBounds::eHalfLengthXposY);
    double hY = bounds->get(TrapezoidBounds::eHalfLengthY);

    CHECK_CLOSE_ABS(hxmin, std::min(hXminY, hXmaxY), s_epsilon);
    CHECK_CLOSE_ABS(hxmax, std::max(hXminY, hXmaxY), s_epsilon);
    CHECK_CLOSE_ABS(hy, hY, s_epsilon);

    // Check if the surface is the (negative) identity
    auto transform = plane->transform(tgContext);
    auto rotation = transform.rotation();
    const Vector3 offset{(-5.5 + (itrd++) * 2.5) * hxmax, 0., 0.};
    GeometryView3D::drawSurface(objVis, *plane, tgContext,
                                Translation3{offset} * Transform3::Identity());
    const Vector3 center = plane->center(tgContext) + offset;
    GeometryView3D::drawArrowForward(
        objVis, center, center + 1.2 * (hXminY + hXmaxY) * rotation.col(0), 4.,
        2.5, red);
    GeometryView3D::drawArrowForward(
        objVis, center, center + 1.2 * hY * rotation.col(1), 4., 2.5, green);
    GeometryView3D::drawArrowForward(
        objVis, center, center + 2 * rotation.col(2), 4., 2.5, blue);
  }

  // Check exceptions for not allowed axis definition
  std::vector<std::string> notAllowed = {"XY*", "YZ*", "ZX*", "ZY*"};
  for (const auto &naxis : notAllowed) {
    BOOST_CHECK_THROW(TGeoSurfaceConverter::toSurface(*vol->GetShape(),
                                                      *gGeoIdentity, naxis, 1),
                      std::invalid_argument);
  }
  objVis.write("TGeoConversion_TGeoTrd1_PlaneSurface");
}

}  // namespace Acts::Test
