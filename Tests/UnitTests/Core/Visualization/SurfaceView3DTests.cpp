// This file is part of the ACTS project.
//
// Copyright (C) 2020-2024 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Visualization/ObjVisualization3D.hpp"
#include "Acts/Visualization/PlyVisualization3D.hpp"

#include <iostream>
#include <string>
#include <vector>

#include "SurfaceView3DBase.hpp"
#include "Visualization3DTester.hpp"

namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(Visualization)

/// This tests if the corresponding obj output is well formatted
BOOST_AUTO_TEST_CASE(SurfaceView3DObj) {
  ObjVisualization3D obj;
  // Standard test
  bool triangulate = false;
  auto objTest = SurfaceView3DTest::run(obj, triangulate, "");
  auto objErrors = testObjString(objTest, triangulate);
  std::cout << "Surfaces Obj Test    : " << objTest.size()
            << " characters written with " << objErrors.size() << " errors."
            << std::endl;
  BOOST_CHECK(objErrors.empty());
  for (const auto& objerr : objErrors) {
    std::cout << objerr << std::endl;
  }
  // Triangular mesh test
  triangulate = true;
  auto objTest3M = SurfaceView3DTest::run(obj, triangulate, "_3M");
  auto objErrors3M = testObjString(objTest3M, triangulate);
  std::cout << "Surfaces Obj Test 3M : " << objTest3M.size()
            << " characters written with " << objErrors3M.size() << " errors."
            << std::endl;
  BOOST_CHECK(objErrors3M.empty());
  for (const auto& objerr : objErrors3M) {
    std::cout << objerr << std::endl;
  }
}

/// This tests if the corresponding ply output is well formatted
BOOST_AUTO_TEST_CASE(SurfaceView3DPly) {
  PlyVisualization3D ply;
  // Standard test
  bool triangulate = false;
  auto plyTest = SurfaceView3DTest::run(ply, triangulate, "");
  auto plyErrors = testPlyString(plyTest, triangulate);
  std::cout << "Surfaces Ply Test    : " << plyTest.size()
            << " characters written with " << plyErrors.size() << " errors."
            << std::endl;
  BOOST_CHECK(plyErrors.empty());
  for (const auto& plyerr : plyErrors) {
    std::cout << plyerr << std::endl;
  }
  // Triangular mesh test
  triangulate = true;
  auto plyTest3M = SurfaceView3DTest::run(ply, triangulate, "_3M");
  auto plyErrors3M = testPlyString(plyTest3M, triangulate);
  std::cout << "Surfaces Ply Test 3M : " << plyTest3M.size()
            << " characters written with " << plyErrors3M.size() << " errors."
            << std::endl;
  BOOST_CHECK(plyErrors3M.empty());
  for (const auto& plyerr : plyErrors3M) {
    std::cout << plyerr << std::endl;
  }
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
