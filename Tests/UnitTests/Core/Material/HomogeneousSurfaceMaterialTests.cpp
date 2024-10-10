// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Common.hpp"
#include "Acts/Definitions/Direction.hpp"
#include "Acts/Material/HomogeneousSurfaceMaterial.hpp"
#include "Acts/Material/Material.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"

#include <utility>

namespace Acts::Test {

/// Test the constructors
BOOST_AUTO_TEST_CASE(HomogeneousSurfaceMaterial_construction_test) {
  // construct the material properties from arguments
  Material mat = Material::fromMolarDensity(1., 2., 3., 4., 5.);
  MaterialSlab mp(mat, 0.1);

  // Constructor from arguments
  HomogeneousSurfaceMaterial hsm(mp, 1.);
  // Copy constructor
  HomogeneousSurfaceMaterial hsmCopy(hsm);
  // Test equality of the copy
  BOOST_CHECK_EQUAL(hsm, hsmCopy);
  // Copy move constructor
  HomogeneousSurfaceMaterial hsmCopyMoved(std::move(hsmCopy));
  // Test equality of the copy
  BOOST_CHECK_EQUAL(hsm, hsmCopyMoved);
  // Assignment constructor
  HomogeneousSurfaceMaterial hsmAssigned = hsm;
  // Test equality of the assignment
  BOOST_CHECK_EQUAL(hsm, hsmAssigned);
  // Assignment move constructor
  HomogeneousSurfaceMaterial hsmAssignedMoved(std::move(hsmAssigned));
  // Test equality of the copy
  BOOST_CHECK_EQUAL(hsm, hsmAssignedMoved);
}

// Test the Scaling
BOOST_AUTO_TEST_CASE(HomogeneousSurfaceMaterial_scaling_test) {
  MaterialSlab mat(Material::fromMolarDensity(1., 2., 3., 4., 5.), 0.1);
  MaterialSlab matHalf = mat;
  matHalf.scaleThickness(0.5);

  HomogeneousSurfaceMaterial hsm(mat, 1.);
  hsm *= 0.5;

  auto matBin = hsm.materialSlab(Vector3(0., 0., 0.));

  BOOST_CHECK_EQUAL(matBin, matHalf);
  BOOST_CHECK_NE(matBin, mat);
}

// Test the Access
BOOST_AUTO_TEST_CASE(HomogeneousSurfaceMaterial_access_test) {
  // construct the material properties from arguments
  MaterialSlab mat(Material::fromMolarDensity(1., 2., 3., 4., 5.), 0.1);
  MaterialSlab matHalf = mat;
  matHalf.scaleThickness(0.5);

  MaterialSlab vacuum = MaterialSlab();

  // Constructor from arguments
  HomogeneousSurfaceMaterial hsmfwd(mat, 1.);
  HomogeneousSurfaceMaterial hsmhalf(mat, 0.5);
  HomogeneousSurfaceMaterial hsmbwd(mat, 0.);

  auto mat2d = hsmfwd.materialSlab(Vector2{0., 0.});
  auto mat3d = hsmfwd.materialSlab(Vector3{0., 0., 0.});

  // Test equality of the copy
  BOOST_CHECK_EQUAL(mat, mat2d);
  BOOST_CHECK_EQUAL(mat, mat3d);

  Direction fDir = Direction::Forward;
  Direction bDir = Direction::Backward;

  MaterialUpdateStage pre = MaterialUpdateStage::PreUpdate;
  MaterialUpdateStage full = MaterialUpdateStage::FullUpdate;
  MaterialUpdateStage post = MaterialUpdateStage::PostUpdate;

  // (a) Forward factor material test
  BOOST_CHECK_EQUAL(hsmfwd.factor(fDir, full), 1.);
  BOOST_CHECK_EQUAL(hsmfwd.factor(fDir, pre), 0.);
  BOOST_CHECK_EQUAL(hsmfwd.factor(fDir, post), 1.);

  BOOST_CHECK_EQUAL(hsmfwd.factor(bDir, full), 1.);
  BOOST_CHECK_EQUAL(hsmfwd.factor(bDir, pre), 1.);
  BOOST_CHECK_EQUAL(hsmfwd.factor(bDir, post), 0.);

  auto matFwdFull = hsmfwd.materialSlab(Vector3{0., 0., 0.}, fDir, full);
  auto matBwdFull = hsmfwd.materialSlab(Vector3{0., 0., 0.}, bDir, full);

  auto matFwdPost = hsmfwd.materialSlab(Vector3{0., 0., 0.}, fDir, post);
  auto matBwdPost = hsmfwd.materialSlab(Vector3{0., 0., 0.}, bDir, post);

  auto matFwdPre = hsmfwd.materialSlab(Vector3{0., 0., 0.}, fDir, pre);
  auto matBwdPre = hsmfwd.materialSlab(Vector3{0., 0., 0.}, bDir, pre);

  BOOST_CHECK_EQUAL(mat, matFwdFull);
  BOOST_CHECK_EQUAL(mat, matBwdFull);

  BOOST_CHECK_EQUAL(mat, matFwdPost);
  BOOST_CHECK_EQUAL(vacuum, matBwdPost);

  BOOST_CHECK_EQUAL(vacuum, matFwdPre);
  BOOST_CHECK_EQUAL(mat, matBwdPre);

  // (b) Split factor material test
  BOOST_CHECK_EQUAL(hsmhalf.factor(fDir, full), 1.);
  CHECK_CLOSE_REL(hsmhalf.factor(fDir, pre), 0.5, 1e-6);
  CHECK_CLOSE_REL(hsmhalf.factor(fDir, post), 0.5, 1e-6);

  BOOST_CHECK_EQUAL(hsmhalf.factor(bDir, full), 1.);
  CHECK_CLOSE_REL(hsmhalf.factor(bDir, pre), 0.5, 1e-6);
  CHECK_CLOSE_REL(hsmhalf.factor(bDir, post), 0.5, 1e-6);

  matFwdFull = hsmhalf.materialSlab(Vector3{0., 0., 0.}, fDir, full);
  matBwdFull = hsmhalf.materialSlab(Vector3{0., 0., 0.}, bDir, full);

  matFwdPost = hsmhalf.materialSlab(Vector3{0., 0., 0.}, fDir, post);
  matBwdPost = hsmhalf.materialSlab(Vector3{0., 0., 0.}, bDir, post);

  matFwdPre = hsmhalf.materialSlab(Vector3{0., 0., 0.}, fDir, pre);
  matBwdPre = hsmhalf.materialSlab(Vector3{0., 0., 0.}, bDir, pre);

  BOOST_CHECK_EQUAL(mat, matFwdFull);
  BOOST_CHECK_EQUAL(mat, matBwdFull);

  BOOST_CHECK_EQUAL(matHalf, matFwdPost);
  BOOST_CHECK_EQUAL(matHalf, matBwdPost);

  BOOST_CHECK_EQUAL(matHalf, matFwdPre);
  BOOST_CHECK_EQUAL(matHalf, matBwdPre);

  // c) Forward factor material test
  BOOST_CHECK_EQUAL(hsmbwd.factor(fDir, full), 1.);
  BOOST_CHECK_EQUAL(hsmbwd.factor(fDir, pre), 1.);
  BOOST_CHECK_EQUAL(hsmbwd.factor(fDir, post), 0.);

  BOOST_CHECK_EQUAL(hsmbwd.factor(bDir, full), 1.);
  BOOST_CHECK_EQUAL(hsmbwd.factor(bDir, pre), 0.);
  BOOST_CHECK_EQUAL(hsmbwd.factor(bDir, post), 1.);

  matFwdFull = hsmbwd.materialSlab(Vector3{0., 0., 0.}, fDir, full);
  matBwdFull = hsmbwd.materialSlab(Vector3{0., 0., 0.}, bDir, full);

  matFwdPost = hsmbwd.materialSlab(Vector3{0., 0., 0.}, fDir, post);
  matBwdPost = hsmbwd.materialSlab(Vector3{0., 0., 0.}, bDir, post);

  matFwdPre = hsmbwd.materialSlab(Vector3{0., 0., 0.}, fDir, pre);
  matBwdPre = hsmbwd.materialSlab(Vector3{0., 0., 0.}, bDir, pre);

  BOOST_CHECK_EQUAL(mat, matFwdFull);
  BOOST_CHECK_EQUAL(mat, matBwdFull);

  BOOST_CHECK_EQUAL(vacuum, matFwdPost);
  BOOST_CHECK_EQUAL(mat, matBwdPost);

  BOOST_CHECK_EQUAL(mat, matFwdPre);
  BOOST_CHECK_EQUAL(vacuum, matBwdPre);
}
}  // namespace Acts::Test
