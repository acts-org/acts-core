// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Material/HomogeneousVolumeMaterial.hpp"
#include "Acts/Material/Material.hpp"

#include <utility>

namespace Acts::Test {

/// Test the constructors
BOOST_AUTO_TEST_CASE(HomogeneousVolumeMaterial_construction_test) {
  // construct the material properties from arguments
  Material mat = Material::fromMolarDensity(1., 2., 3., 4., 5.);

  // Constructor from arguments
  HomogeneousVolumeMaterial hsm(mat);
  // Copy constructor
  HomogeneousVolumeMaterial hsmCopy(hsm);
  // Test equality of the copy
  BOOST_CHECK_EQUAL(hsm, hsmCopy);
  // Copy move constructor
  HomogeneousVolumeMaterial hsmCopyMoved(std::move(hsmCopy));
  // Test equality of the copy
  BOOST_CHECK_EQUAL(hsm, hsmCopyMoved);
  // Assignment constructor
  HomogeneousVolumeMaterial hsmAssigned = hsm;
  // Test equality of the assignment
  BOOST_CHECK_EQUAL(hsm, hsmAssigned);
  // Assignment move constructor
  HomogeneousVolumeMaterial hsmAssignedMoved(std::move(hsmAssigned));
  // Test equality of the copy
  BOOST_CHECK_EQUAL(hsm, hsmAssignedMoved);
}

// Test the Access
BOOST_AUTO_TEST_CASE(HomogeneousVolumeMaterial_access_test) {
  // construct the material properties from arguments
  Material mat = Material::fromMolarDensity(1., 2., 3., 4., 5.);

  // Constructor from arguments
  HomogeneousVolumeMaterial hsm(mat);

  auto mat3d = hsm.material(Vector3{0., 0., 0.});

  // Test equality of the copy
  BOOST_CHECK_EQUAL(mat, mat3d);
}
}  // namespace Acts::Test
