// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Material/InterpolatedMaterialMap.hpp"
#include "Acts/Material/Material.hpp"
#include "Acts/Material/MaterialMapUtils.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"

#include <array>
#include <cstddef>
#include <vector>

namespace Acts::Test {

BOOST_AUTO_TEST_CASE(materialmap_creation) {
  // Create grid values
  std::vector<double> rPos = {0., 1., 2.};
  std::vector<double> xPos = {0., 1., 2.};
  std::vector<double> yPos = {0., 1., 2.};
  std::vector<double> zPos = {0., 1., 2.};

  // Create material association in rz
  std::vector<Material> material_rz;
  for (int i = 0; i < 9; i++) {
    material_rz.push_back(Material::fromMolarDensity(i, i, i, i, i));
  }

  auto localToGlobalBin_rz = [](std::array<std::size_t, 2> binsRZ,
                                std::array<std::size_t, 2> nBinsRZ) {
    return (binsRZ.at(1) * nBinsRZ.at(0) + binsRZ.at(0));
  };
  // Create material mapper in rz
  auto mapper_rz =
      materialMapperRZ(localToGlobalBin_rz, rPos, zPos, material_rz);
  // check number of bins, minima & maxima
  std::vector<std::size_t> nBins_rz = {rPos.size(), zPos.size()};
  std::vector<double> minima_rz = {0., 0.};
  std::vector<double> maxima_rz = {3., 3.};
  BOOST_CHECK(mapper_rz.getNBins() == nBins_rz);
  // Check minimum (should be first value because bin values are always
  // assigned to the left boundary)
  BOOST_CHECK(mapper_rz.getMin() == minima_rz);
  // Check maximum (should be last value + 1 step because bin values are
  // always assigned to the left boundary)
  BOOST_CHECK(mapper_rz.getMax() == maxima_rz);

  // Create map in xyz
  std::vector<Material> material_xyz;
  for (int i = 0; i < 27; i++) {
    material_xyz.push_back(Material::fromMolarDensity(i, i, i, i, i));
  }

  auto localToGlobalBin_xyz = [](std::array<std::size_t, 3> binsXYZ,
                                 std::array<std::size_t, 3> nBinsXYZ) {
    return (binsXYZ.at(0) * (nBinsXYZ.at(1) * nBinsXYZ.at(2)) +
            binsXYZ.at(1) * nBinsXYZ.at(2) + binsXYZ.at(2));
  };

  // Create material mapper in xyz
  auto mapper_xyz =
      materialMapperXYZ(localToGlobalBin_xyz, xPos, yPos, zPos, material_xyz);
  // Check number of bins, minima & maxima
  std::vector<std::size_t> nBins_xyz = {xPos.size(), yPos.size(), zPos.size()};
  std::vector<double> minima_xyz = {0., 0., 0.};
  std::vector<double> maxima_xyz = {3., 3., 3.};
  BOOST_CHECK(mapper_xyz.getNBins() == nBins_xyz);
  // Check minimum (should be first value because bin values are always
  // assigned to the left boundary)
  BOOST_CHECK(mapper_xyz.getMin() == minima_xyz);
  // Check maximum (should be last value + 1 step because bin values are
  // always assigned to the left boundary)
  BOOST_CHECK(mapper_xyz.getMax() == maxima_xyz);

  // Check if filled value is expected value in rz
  Vector3 pos0_rz(0., 0., 0.);
  Vector3 pos1_rz(1., 0., 1.);
  Vector3 pos2_rz(0., 2., 2.);
  auto value0_rz = mapper_rz.getMaterial(pos0_rz);
  auto value1_rz = mapper_rz.getMaterial(pos1_rz);
  auto value2_rz = mapper_rz.getMaterial(pos2_rz);
  // Calculate what the value should be at this point
  Material mat0_rz = material_rz.at(
      localToGlobalBin_rz({{0, 0}}, {{rPos.size(), zPos.size()}}));
  Material mat1_rz = material_rz.at(
      localToGlobalBin_rz({{1, 1}}, {{rPos.size(), zPos.size()}}));
  Material mat2_rz = material_rz.at(
      localToGlobalBin_rz({{2, 2}}, {{rPos.size(), zPos.size()}}));

  // Check the value
  // in rz case material is phi symmetric (check radius)
  CHECK_CLOSE_ABS(value0_rz.parameters(), mat0_rz.parameters(), 1e-9);
  CHECK_CLOSE_ABS(value1_rz.parameters(), mat1_rz.parameters(), 1e-9);
  CHECK_CLOSE_ABS(value2_rz.parameters(), mat2_rz.parameters(), 1e-9);

  // Check if filled value is expected value in xyz
  Vector3 pos0_xyz(0., 0., 0.);
  Vector3 pos1_xyz(1., 1., 1.);
  Vector3 pos2_xyz(2., 2., 2.);
  auto value0_xyz = mapper_xyz.getMaterial(pos0_xyz);
  auto value1_xyz = mapper_xyz.getMaterial(pos1_xyz);
  auto value2_xyz = mapper_xyz.getMaterial(pos2_xyz);
  // Calculate what the value should be at this point
  Material mat0_xyz = material_xyz.at(localToGlobalBin_xyz(
      {{0, 0, 0}}, {{xPos.size(), yPos.size(), zPos.size()}}));
  Material mat1_xyz = material_xyz.at(localToGlobalBin_xyz(
      {{1, 1, 1}}, {{xPos.size(), yPos.size(), zPos.size()}}));
  Material mat2_xyz = material_xyz.at(localToGlobalBin_xyz(
      {{2, 2, 2}}, {{xPos.size(), yPos.size(), zPos.size()}}));

  // Check the value
  // in xyz case material is phi symmetric (check radius)
  CHECK_CLOSE_ABS(value0_xyz.parameters(), mat0_xyz.parameters(), 1e-9);
  CHECK_CLOSE_ABS(value1_xyz.parameters(), mat1_xyz.parameters(), 1e-9);
  CHECK_CLOSE_ABS(value2_xyz.parameters(), mat2_xyz.parameters(), 1e-9);
}
}  // namespace Acts::Test
