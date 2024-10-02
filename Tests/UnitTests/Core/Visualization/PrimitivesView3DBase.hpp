// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Visualization/EventDataView3D.hpp"
#include "Acts/Visualization/GeometryView3D.hpp"
#include "Acts/Visualization/IVisualization3D.hpp"
#include "Acts/Visualization/ObjVisualization3D.hpp"
#include "Acts/Visualization/PlyVisualization3D.hpp"

#include <fstream>
#include <sstream>
#include <string>

namespace Acts::PrimitivesView3DTest {

// Test on a plane
auto identity = Transform3::Identity();
auto rectangle = std::make_shared<RectangleBounds>(10., 10.);
auto plane = Surface::makeShared<PlaneSurface>(identity, rectangle);

// Test context
GeometryContext gctx = GeometryContext();

/// Helper method to visualize all types of surfaces
///
/// @param helper The visualization helper
///
/// @return an overall string including all written output
static inline std::string run(IVisualization3D& helper) {
  std::stringstream ss;

  ViewConfig lineView{.color = {0, 0, 255}};
  lineView.lineThickness = 0.1;

  // Line visualization ------------------------------------------------
  Vector3 start = {1., 1., 1.};
  Vector3 end = {4., 4., 4.};
  Acts::GeometryView3D::drawSegment(helper, start, end);
  helper.write("Primitives_Line");
  helper.write(ss);
  helper.clear();

  // Arrows visualization ------------------------------------------------
  start = {1., 0., 0.};
  end = {4., 0., 0.};
  Acts::GeometryView3D::drawArrowForward(helper, start, end, 3., 2., lineView);

  start = {1., 2., 0.};
  end = {4., 2., 0.};
  Acts::GeometryView3D::drawArrowBackward(helper, start, end, 3., 2., lineView);

  start = {1., 4., 0.};
  end = {4., 4., 0.};
  Acts::GeometryView3D::drawArrowsBoth(helper, start, end, 3., 2., lineView);

  helper.write("Primitives_Arrows");
  helper.write(ss);
  helper.clear();

  // Error visualization: local ---------------------------------------------
  Acts::GeometryView3D::drawSurface(helper, *plane, gctx);

  ViewConfig errorVis{.color = {250, 0, 0}};
  errorVis.lineThickness = 0.025;

  SquareMatrix2 cov = SquareMatrix2::Identity();
  double s0 = 0.75;
  double s1 = 1.99;
  double r01 = 0.78;
  cov << s0 * s0, r01 * s0 * s1, r01 * s0 * s1, s1 * s1;

  Vector2 lcentered{0., 0.};
  Acts::EventDataView3D::drawCovarianceCartesian(
      helper, lcentered, cov, plane->transform(gctx), 1.0, errorVis);

  helper.write("Primitives_CartesianError");
  helper.write(ss);
  helper.clear();

  // Error visualization: angular ---------------------------------------------
  Acts::GeometryView3D::drawSurface(helper, *plane, gctx);
  cov = SquareMatrix2::Identity();
  s0 = 0.08;
  s1 = 0.035;
  r01 = 0.3;
  cov << s0 * s0, r01 * s0 * s1, r01 * s0 * s1, s1 * s1;

  Vector3 origin{0., 0., 0.};
  Vector3 direction = Vector3(1., 3., 15.).normalized();

  double directionScale = 5.;

  Acts::EventDataView3D::drawCovarianceAngular(helper, origin, direction, cov,
                                               directionScale, 10., errorVis);

  Acts::GeometryView3D::drawArrowForward(
      helper, origin + 0.5 * directionScale * direction,
      origin + 1.2 * directionScale * direction, 3., 2., errorVis);

  helper.write("Primitives_AngularError");
  helper.write(ss);
  helper.clear();

  return ss.str();
}

}  // namespace Acts::PrimitivesView3DTest
