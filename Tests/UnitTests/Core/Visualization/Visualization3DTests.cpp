// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/tools/output_test_stream.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Visualization/IVisualization3D.hpp"
#include "Acts/Visualization/ObjVisualization3D.hpp"
#include "Acts/Visualization/PlyVisualization3D.hpp"

#include <iostream>
#include <string>
#include <vector>

#include "Visualization3DTester.hpp"

namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(Visualization)

BOOST_AUTO_TEST_CASE(Visualization3DTesterObj) {
  // Test the tester
  std::string validObj = R"(# obj test file
mtllib material.mtl
usemtl material_a
g rectangle
vn 0 0 1
vt 0 0 1
v -15 -15 0
v 15 -15 0
v 15 15 0
v -15 15 0
f 1 2 3 4
l 1 2
l 2 3
l 3 4
l 4 1
)";

  // Valid obj
  auto objErrors = testObjString(validObj);
  BOOST_CHECK(objErrors.empty());

  // Valid obj, but triangular mesh is requested
  objErrors = testObjString(validObj, true);
  BOOST_CHECK_EQUAL(objErrors.size(), 1);
  for (const auto& objerr : objErrors) {
    std::cout << objerr << std::endl;
  }

  // Non-valid obj - it has 4 errors
  std::string invalidObj = R"(# obj test file
mtllib material.mtl
usemtl material_a
x whatever
g rectangle
vn 0 0 1
vt 0 0 1
v -15 -15 0 23
v 15 -15 0
v 15. 15 0
v -15 15. 0
f 1 2 3. 4
l 0 2
l 2 3
l 3 4
l 4 1
)";

  objErrors = testObjString(invalidObj);
  BOOST_CHECK_EQUAL(objErrors.size(), 4);
  for (const auto& objerr : objErrors) {
    std::cout << objerr << std::endl;
  }
}

BOOST_AUTO_TEST_CASE(Visualization3DTesterPly) {
  // Test the tester
  std::string validPly = R"(ply
format ascii 1.0
comment made by Greg Turk
comment this file is a cube
element vertex 8
property float x
property float y
property float z
element face 6
property list uchar int vertex_indices
end_header
0 0 0
0 0 1
0 1 1
0 1 0
1 0 0
1 0 1
1 1 1
1 1 0
4 0 1 2 3
4 7 6 5 4
4 0 4 5 1
4 1 5 6 2
4 2 6 7 3
4 3 7 4 0
)";

  // Valid ply
  auto plyErrors = testPlyString(validPly);
  BOOST_CHECK(plyErrors.empty());

  // Valid ply, but triangular mesh is requested
  plyErrors = testPlyString(validPly, true);
  BOOST_CHECK(plyErrors.empty());
  for (const auto& plyerr : plyErrors) {
    std::cout << plyerr << std::endl;
  }

  // Test the tester - contains 3 errors
  std::string invalidPly = R"(ply
format ascii 1.0
comment made by Greg Turk
comment this file is a cube
element vertex 8
property float x
property float y
property float z
element face 6
property list uchar int vertex_indices
whatever i write here
end_header
0 0 0 0
0 0 1
0 1 1
0 1 0
1 0 0
1 0 1
1 1 1
1 1 0
4 0 1 2 3
4 7 6 5 4
4 0 4 5 1
4 1 5 6
4 2 6 7 3
4 3 7 4 0
)";

  // Valid ply, but triangular mesh is requested
  plyErrors = testPlyString(invalidPly);
  BOOST_CHECK_EQUAL(plyErrors.size(), 3);
  for (const auto& plyerr : plyErrors) {
    std::cout << plyerr << std::endl;
  }
}

BOOST_AUTO_TEST_CASE(Visualization3DConstruction) {
  // this doesn't really test anything, other than conformance to the
  // IVisualization3D interface
  PlyVisualization3D ply;
  ObjVisualization3D obj;

  IVisualization3D* vis = nullptr;
  vis = &ply;
  std::cout << *vis << std::endl;
  vis = &obj;
  std::cout << *vis << std::endl;
}

BOOST_AUTO_TEST_CASE(PlyOutputTest) {
  PlyVisualization3D ply;
  boost::test_tools::output_test_stream output;

  ply.vertex({0, 0, 0});

  std::string exp = R"(ply
format ascii 1.0
element vertex 1
property float x
property float y
property float z
property uchar red
property uchar green
property uchar blue
element face 0
property list uchar int vertex_index
element edge 0
property int vertex1
property int vertex2
property uchar red
property uchar green
property uchar blue
end_header
0 0 0 120 120 120
)";

  output << ply;
  BOOST_CHECK(output.is_equal(exp));

  ply.clear();
  ply.vertex({0, 1, 0});

  exp = R"(ply
format ascii 1.0
element vertex 1
property float x
property float y
property float z
property uchar red
property uchar green
property uchar blue
element face 0
property list uchar int vertex_index
element edge 0
property int vertex1
property int vertex2
property uchar red
property uchar green
property uchar blue
end_header
0 1 0 120 120 120
)";

  output << ply;
  BOOST_CHECK(output.is_equal(exp));

  ply.clear();
  ply.line({0, 0, 1}, {1, 0, 0});

  output << ply;

  exp = R"(ply
format ascii 1.0
element vertex 2
property float x
property float y
property float z
property uchar red
property uchar green
property uchar blue
element face 0
property list uchar int vertex_index
element edge 1
property int vertex1
property int vertex2
property uchar red
property uchar green
property uchar blue
end_header
0 0 1 120 120 120
1 0 0 120 120 120
0 1 120 120 120
)";

  BOOST_CHECK(output.is_equal(exp));

  ply.clear();
  ply.face({{1, 0, 0}, {1, 1, 0}, {0, 1, 0}});

  output << ply;

  exp = R"(ply
format ascii 1.0
element vertex 3
property float x
property float y
property float z
property uchar red
property uchar green
property uchar blue
element face 1
property list uchar int vertex_index
element edge 0
property int vertex1
property int vertex2
property uchar red
property uchar green
property uchar blue
end_header
1 0 0 120 120 120
1 1 0 120 120 120
0 1 0 120 120 120
3 0 1 2
)";

  BOOST_CHECK(output.is_equal(exp));
}

BOOST_AUTO_TEST_CASE(ObjOutputTest) {
  ObjVisualization3D obj;

  boost::test_tools::output_test_stream output;

  obj.vertex({1, 0, 0});

  output << obj;

  std::string exp = R"(usemtl material_120_120_120
v 1 0 0
)";

  BOOST_CHECK(output.is_equal(exp));

  obj.clear();
  obj.face({{1, 0, 0}, {1, 1, 0}, {0, 1, 0}});
  output << obj;

  exp = R"(usemtl material_120_120_120
v 1 0 0
v 1 1 0
v 0 1 0
usemtl material_120_120_120
f 1 2 3
)";

  BOOST_CHECK(output.is_equal(exp));
}

BOOST_AUTO_TEST_CASE(ColorTests) {
  Color red{"#ff0000"};
  BOOST_CHECK_EQUAL(red, Color(255, 0, 0));

  Color green{"#00ff00"};
  BOOST_CHECK_EQUAL(green, Color(0, 255, 0));

  Color blue{"#0000ff"};
  BOOST_CHECK_EQUAL(blue, Color(0, 0, 255));

  Color grey{"#808080"};
  BOOST_CHECK_EQUAL(grey, Color(128, 128, 128));
  BOOST_CHECK_EQUAL(grey, Color(std::array{128, 128, 128}));
  BOOST_CHECK_EQUAL(grey,
                    Color(std::array{128 / 255.0, 128 / 255.0, 128 / 255.0}));
  BOOST_CHECK_EQUAL(grey, Color(128 / 255.0, 128 / 255.0, 128 / 255.0));

  static_assert(Color{"#0000ff"} == Color(0, 0, 255));
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
