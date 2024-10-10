// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include <cmath>
#include <cstddef>
#include <vector>

// Helper
#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Geometry/Polyhedron.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Visualization/GeometryView3D.hpp"
#include "Acts/Visualization/ObjVisualization3D.hpp"

using namespace Acts::UnitLiterals;

namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(Geometry)

/// Unit tests for Polyderon construction & operator +=
BOOST_AUTO_TEST_CASE(PolyhedronTest) {
  std::vector<Vector3> tvertices = {Vector3(-1, -1, 0.), Vector3(1., -1, 0.),
                                    Vector3(0., 1., 0.)};
  std::vector<std::vector<std::size_t>> tfaces = {{0, 1, 2}};

  Polyhedron triangle(tvertices, tfaces, tfaces);
  BOOST_CHECK(tvertices == triangle.vertices);
  BOOST_CHECK(tfaces == triangle.faces);
  BOOST_CHECK(tfaces == triangle.triangularMesh);

  ObjVisualization3D objVis;
  GeometryView3D::drawPolyhedron(objVis, triangle);
  objVis.write("Polyhedron_Triangle");
  objVis.clear();

  std::vector<Vector3> rvertices = {Vector3(-1, -2, 0.), Vector3(1., -2, 0.),
                                    Vector3(1., -1., 0.),
                                    Vector3(-1., -1., 0.)};
  std::vector<std::vector<std::size_t>> rfaces = {{0, 1, 2, 3}};
  std::vector<std::vector<std::size_t>> rmesh = {{0, 1, 2}, {2, 3, 0}};
  Polyhedron rectangle(rvertices, rfaces, rmesh);
  BOOST_CHECK(rvertices == rectangle.vertices);
  BOOST_CHECK(rfaces == rectangle.faces);
  BOOST_CHECK(rmesh == rectangle.triangularMesh);

  GeometryView3D::drawPolyhedron(objVis, rectangle);
  objVis.write("Polyhedron_Rectangle");
  objVis.clear();

  // Now add them
  Polyhedron tr;
  tr.merge(triangle);
  BOOST_CHECK(tr.vertices == triangle.vertices);
  BOOST_CHECK(tr.faces == triangle.faces);
  BOOST_CHECK(tr.triangularMesh == triangle.triangularMesh);
  tr.merge(rectangle);

  GeometryView3D::drawPolyhedron(objVis, tr);
  objVis.write("Polyhedron_TriangleRectangle");
  objVis.clear();
}

/// Unit tests for Polyderon construction & operator +=
BOOST_AUTO_TEST_CASE(PolyhedronExtent) {
  // Test a rectangle in x-y plane (at z == 0)
  std::vector<Vector3> rvertices = {Vector3(-1, -2, 0.), Vector3(1., -2, 0.),
                                    Vector3(1., -1., 0.),
                                    Vector3(-1., -1., 0.)};

  std::vector<std::vector<std::size_t>> rfaces = {{0, 1, 2, 3}};
  std::vector<std::vector<std::size_t>> rmesh = {{0, 1, 2}, {2, 3, 0}};
  Polyhedron rectangle(rvertices, rfaces, rmesh);

  auto rExtent = rectangle.extent();
  CHECK_CLOSE_ABS(rExtent.min(BinningValue::binX), -1., 1e-6);
  CHECK_CLOSE_ABS(rExtent.max(BinningValue::binX), 1., 1e-6);
  CHECK_CLOSE_ABS(rExtent.min(BinningValue::binY), -2., 1e-6);
  CHECK_CLOSE_ABS(rExtent.max(BinningValue::binY), -1., 1e-6);
  CHECK_CLOSE_ABS(rExtent.min(BinningValue::binZ), 0., 1e-6);
  CHECK_CLOSE_ABS(rExtent.max(BinningValue::binZ), 0., 1e-6);
  CHECK_CLOSE_ABS(rExtent.min(BinningValue::binR), 1., 1e-6);
  CHECK_CLOSE_ABS(rExtent.max(BinningValue::binR),
                  VectorHelpers::perp(rvertices[0]), 1e-6);
  CHECK_CLOSE_ABS(rExtent.min(BinningValue::binPhi),
                  VectorHelpers::phi(rvertices[3]), 1e-6);
  CHECK_CLOSE_ABS(rExtent.max(BinningValue::binPhi),
                  VectorHelpers::phi(rvertices[2]), 1e-6);

  // Now shift the Extent
  Vector3 shift(-1., 0., 1.);
  Transform3 shiftedTransform = Transform3::Identity();
  shiftedTransform.pretranslate(shift);
  rExtent = rectangle.extent(shiftedTransform);
  CHECK_CLOSE_ABS(rExtent.min(BinningValue::binX), -2., 1e-6);
  CHECK_CLOSE_ABS(rExtent.max(BinningValue::binX), 0., 1e-6);
  CHECK_CLOSE_ABS(rExtent.min(BinningValue::binY), -2., 1e-6);
  CHECK_CLOSE_ABS(rExtent.max(BinningValue::binY), -1., 1e-6);
  CHECK_CLOSE_ABS(rExtent.min(BinningValue::binZ), 1., 1e-6);
  CHECK_CLOSE_ABS(rExtent.max(BinningValue::binZ), 1., 1e-6);

  // Test a rectangle in yz - pane (at x == 3)
  rvertices = {Vector3(3_mm, -5_mm, -10_mm), Vector3(3_mm, 5_mm, -10_mm),
               Vector3(3_mm, 5_mm, 10_mm), Vector3(3_mm, -5_mm, 10_mm)};

  rectangle = Polyhedron(rvertices, rfaces, rmesh);
  rExtent = rectangle.extent();
  CHECK_CLOSE_ABS(rExtent.min(BinningValue::binX), 3., 1e-6);
  CHECK_CLOSE_ABS(rExtent.max(BinningValue::binX), 3., 1e-6);
  CHECK_CLOSE_ABS(rExtent.min(BinningValue::binY), -5., 1e-6);
  CHECK_CLOSE_ABS(rExtent.max(BinningValue::binY), 5., 1e-6);
  CHECK_CLOSE_ABS(rExtent.min(BinningValue::binZ), -10., 1e-6);
  CHECK_CLOSE_ABS(rExtent.max(BinningValue::binZ), 10., 1e-6);
  CHECK_CLOSE_ABS(rExtent.min(BinningValue::binR), 3., 1e-6);
  CHECK_CLOSE_ABS(rExtent.max(BinningValue::binR), std::sqrt(9. + 25.), 1e-6);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
