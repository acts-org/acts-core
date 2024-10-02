// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Visualization/ObjVisualization3D.hpp"
#include "Acts/Visualization/PlyVisualization3D.hpp"

#include <iostream>
#include <string>
#include <vector>

#include "Visualization3DTester.hpp"
#include "VolumeView3DBase.hpp"

namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(Visualization)

// Tests if the written obj output is well formatted
BOOST_AUTO_TEST_CASE(VolumeView3DObj) {
  ObjVisualization3D obj;

  // Standard test
  bool triangulate = false;
  auto objTest = VolumeView3DTest::run(obj, triangulate, "");
  auto objErrors = testObjString(objTest, triangulate);
  std::cout << "Volumes Obj Test    : " << objTest.size()
            << " characters written with " << objErrors.size() << " errors."
            << std::endl;
  BOOST_CHECK(objErrors.empty());
  for (const auto& objerr : objErrors) {
    std::cout << objerr << std::endl;
  }

  // Triangular mesh test
  triangulate = true;
  auto objTest3M = VolumeView3DTest::run(obj, triangulate, "_3M");
  auto objErrors3M = testObjString(objTest3M, triangulate);
  std::cout << "Volumes Obj Test 3M : " << objTest3M.size()
            << " characters written with " << objErrors3M.size() << " errors."
            << std::endl;
  BOOST_CHECK(objErrors3M.empty());
  for (const auto& objerr : objErrors3M) {
    std::cout << objerr << std::endl;
  }
}

// Tests if the written ply output is well formatted
BOOST_AUTO_TEST_CASE(VolumeView3DPly) {
  PlyVisualization3D ply;

  // Standard test
  bool triangulate = false;
  auto plyTest = VolumeView3DTest::run(ply, triangulate, "");
  auto plyErrors = testPlyString(plyTest, triangulate);
  std::cout << "Volumes Ply Test    : " << plyTest.size()
            << " characters written with " << plyErrors.size() << " errors."
            << std::endl;
  BOOST_CHECK(plyErrors.empty());
  for (const auto& plyerr : plyErrors) {
    std::cout << plyerr << std::endl;
  }

  // Triangular mesh test
  triangulate = true;
  auto plyTest3M = VolumeView3DTest::run(ply, triangulate, "_3M");
  auto plyErrors3M = testPlyString(plyTest3M, triangulate);
  std::cout << "Volumes Ply Test 3M : " << plyTest3M.size()
            << " characters written with " << plyErrors3M.size() << " errors."
            << std::endl;
  BOOST_CHECK(plyErrors3M.empty());
  for (const auto& plyerr : plyErrors3M) {
    std::cout << plyerr << std::endl;
  }
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
