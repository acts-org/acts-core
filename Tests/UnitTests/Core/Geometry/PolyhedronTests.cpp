// This file is part of the Acts project.
//
// Copyright (C) 2017-2018 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <boost/test/data/test_case.hpp>
#include <boost/test/tools/output_test_stream.hpp>
#include <boost/test/unit_test.hpp>

// Helper
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"

// The class to test
#include <fstream>
#include "Acts/Geometry/Polyhedron.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "Acts/Utilities/Units.hpp"
#include "Acts/Visualization/ObjVisualization.hpp"

namespace Acts {

using namespace UnitLiterals;

namespace Test {

BOOST_AUTO_TEST_SUITE(Geometry)

/// Unit tests for Polyderon construction & operator +=
BOOST_AUTO_TEST_CASE(PolyhedronTest) {
  std::vector<Vector3D> tvertices = {Vector3D(-1, -1, 0.), Vector3D(1., -1, 0.),
                                     Vector3D(0., 1., 0.)};
  std::vector<std::vector<size_t>> tfaces = {{0, 1, 2}};

  Polyhedron triangle(tvertices, tfaces, tfaces);
  BOOST_CHECK(tvertices == triangle.vertices);
  BOOST_CHECK(tfaces == triangle.faces);
  BOOST_CHECK(tfaces == triangle.triangularMesh);

  std::ofstream tStream;
  tStream.open("PolyhedronTriangle.obj");
  ObjVisualization objtH;
  triangle.draw(objtH);
  objtH.write(tStream);
  tStream.close();

  std::vector<Vector3D> rvertices = {Vector3D(-1, -2, 0.), Vector3D(1., -2, 0.),
                                     Vector3D(1., -1., 0.),
                                     Vector3D(-1., -1., 0.)};
  std::vector<std::vector<size_t>> rfaces = {{0, 1, 2, 3}};
  std::vector<std::vector<size_t>> rmesh = {{0, 1, 2}, {2, 3, 0}};
  Polyhedron rectangle(rvertices, rfaces, rmesh);
  BOOST_CHECK(rvertices == rectangle.vertices);
  BOOST_CHECK(rfaces == rectangle.faces);
  BOOST_CHECK(rmesh == rectangle.triangularMesh);

  std::ofstream rStream;
  rStream.open("PolyhedronRectangle.obj");
  ObjVisualization objrH;
  rectangle.draw(objrH);
  objrH.write(rStream);
  rStream.close();

  // Now add them
  Polyhedron tr;
  tr.merge(triangle);
  BOOST_CHECK(tr.vertices == triangle.vertices);
  BOOST_CHECK(tr.faces == triangle.faces);
  BOOST_CHECK(tr.triangularMesh == triangle.triangularMesh);

  tr.merge(rectangle);

  std::ofstream trStream;
  trStream.open("PolyhedronTriangleRectangle.obj");
  ObjVisualization objtrH;
  tr.draw(objtrH);
  objtrH.write(trStream);
  trStream.close();
}

/// Unit tests for Polyderon construction & operator +=
BOOST_AUTO_TEST_CASE(PolyhedronExtent) {
  // Test a rectangle in x-y plane (at z == 0)
  std::vector<Vector3D> rvertices = {Vector3D(-1, -2, 0.), Vector3D(1., -2, 0.),
                                     Vector3D(1., -1., 0.),
                                     Vector3D(-1., -1., 0.)};

  std::vector<std::vector<size_t>> rfaces = {{0, 1, 2, 3}};
  std::vector<std::vector<size_t>> rmesh = {{0, 1, 2}, {2, 3, 0}};
  Polyhedron rectangle(rvertices, rfaces, rmesh);

  auto rExtent = rectangle.extent();
  CHECK_CLOSE_ABS(rExtent.min(binX), -1., 1e-6);
  CHECK_CLOSE_ABS(rExtent.max(binX), 1., 1e-6);
  CHECK_CLOSE_ABS(rExtent.min(binY), -2., 1e-6);
  CHECK_CLOSE_ABS(rExtent.max(binY), -1., 1e-6);
  CHECK_CLOSE_ABS(rExtent.min(binZ), 0., 1e-6);
  CHECK_CLOSE_ABS(rExtent.max(binZ), 0., 1e-6);
  CHECK_CLOSE_ABS(rExtent.min(binR), 1., 1e-6);
  CHECK_CLOSE_ABS(rExtent.max(binR), VectorHelpers::perp(rvertices[0]), 1e-6);
  CHECK_CLOSE_ABS(rExtent.min(binPhi), VectorHelpers::phi(rvertices[3]), 1e-6);
  CHECK_CLOSE_ABS(rExtent.max(binPhi), VectorHelpers::phi(rvertices[2]), 1e-6);

  // Now shift the Extent
  Vector3D shift(-1., 0., 1.);
  Transform3D shiftedTransform = Transform3D::Identity();
  shiftedTransform.pretranslate(shift);
  rExtent = rectangle.extent(shiftedTransform);
  CHECK_CLOSE_ABS(rExtent.min(binX), -2., 1e-6);
  CHECK_CLOSE_ABS(rExtent.max(binX), 0., 1e-6);
  CHECK_CLOSE_ABS(rExtent.min(binY), -2., 1e-6);
  CHECK_CLOSE_ABS(rExtent.max(binY), -1., 1e-6);
  CHECK_CLOSE_ABS(rExtent.min(binZ), 1., 1e-6);
  CHECK_CLOSE_ABS(rExtent.max(binZ), 1., 1e-6);

  // Test a rectangle in yz - pane (at x == 3)
  rvertices = {Vector3D(3_mm, -5_mm, -10_mm), Vector3D(3_mm, 5_mm, -10_mm),
               Vector3D(3_mm, 5_mm, 10_mm), Vector3D(3_mm, -5_mm, 10_mm)};

  rectangle = Polyhedron(rvertices, rfaces, rmesh);
  rExtent = rectangle.extent();
  CHECK_CLOSE_ABS(rExtent.min(binX), 3., 1e-6);
  CHECK_CLOSE_ABS(rExtent.max(binX), 3., 1e-6);
  CHECK_CLOSE_ABS(rExtent.min(binY), -5., 1e-6);
  CHECK_CLOSE_ABS(rExtent.max(binY), 5., 1e-6);
  CHECK_CLOSE_ABS(rExtent.min(binZ), -10., 1e-6);
  CHECK_CLOSE_ABS(rExtent.max(binZ), 10., 1e-6);
  CHECK_CLOSE_ABS(rExtent.min(binR), 3., 1e-6);
  CHECK_CLOSE_ABS(rExtent.max(binR), std::sqrt(9. + 25.), 1e-6);

  // Check for radial extent
  auto radialDistance = [&](const Vector3D& pos1,
                            const Vector3D& pos2) -> double {
    Vector2D p1(pos1.x(), pos1.y());
    Vector2D p2(pos2.x(), pos2.y());

    Vector2D O(0, 0);
    Vector2D p1p2 = (p2 - p1);
    double L = p1p2.norm();
    Vector2D p1O = (O - p1);

    // don't do division if L is very small
    if (L < 1e-7) {
      return std::numeric_limits<double>::max();
    }
    double f = p1p2.dot(p1O) / L;

    // clamp to [0, |p1p2|]
    f = std::min(L, std::max(0., f));

    Vector2D closest = f * p1p2.normalized() + p1;
    double dist = (closest - O).norm();

    return dist;
  };

  std::cout << "R-dist = " << radialDistance(rvertices[0], rvertices[1])
            << std::endl;
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Test

}  // namespace Acts