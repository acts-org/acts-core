// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Material/BinnedSurfaceMaterial.hpp"
#include "Acts/Material/Material.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/BinningType.hpp"

#include <utility>
#include <vector>

namespace Acts::Test {

/// Test the constructors
BOOST_AUTO_TEST_CASE(BinnedSurfaceMaterial_construction_test) {
  BinUtility xyBinning(2, -1., 1., open, BinningValue::binX);
  xyBinning += BinUtility(3, -3., 3., open, BinningValue::binY);

  // Constructor a few material properties
  MaterialSlab a00(Material::fromMolarDensity(1., 2., 3., 4., 5.), 6.);
  MaterialSlab a01(Material::fromMolarDensity(2., 3., 4., 5., 6.), 7.);
  MaterialSlab a02(Material::fromMolarDensity(3., 4., 5., 6., 7.), 8.);
  MaterialSlab a10(Material::fromMolarDensity(4., 5., 6., 7., 8.), 9.);
  MaterialSlab a11(Material::fromMolarDensity(5., 6., 7., 8., 9.), 10.);
  MaterialSlab a12(Material::fromMolarDensity(6., 7., 8., 9., 10.), 11.);

  // Prepare the matrix
  std::vector<MaterialSlab> l0 = {a00, a10};
  std::vector<MaterialSlab> l1 = {a01, a11};
  std::vector<MaterialSlab> l2 = {a02, a12};

  // Build the matrix
  std::vector<std::vector<MaterialSlab>> m = {std::move(l0), std::move(l1),
                                              std::move(l2)};

  // Create the material
  BinnedSurfaceMaterial bsm(xyBinning, std::move(m));

  // Copy the material
  BinnedSurfaceMaterial bsmCopy(bsm);

  // Assignment operation
  BinnedSurfaceMaterial bsmAssigned = bsm;

  // Move constructor
  BinnedSurfaceMaterial bsmMoved(std::move(bsmCopy));

  // Move assigned
  BinnedSurfaceMaterial bsmMoveAssigned(std::move(bsmAssigned));
}

}  // namespace Acts::Test
