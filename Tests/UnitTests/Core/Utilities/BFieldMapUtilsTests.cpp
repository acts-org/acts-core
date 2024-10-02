// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/MagneticField/BFieldMapUtils.hpp"
#include "Acts/MagneticField/InterpolatedBFieldMap.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/Result.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"

#include <array>
#include <cstddef>
#include <random>
#include <utility>
#include <vector>

namespace bdata = boost::unit_test::data;

using Acts::VectorHelpers::perp;

using namespace Acts::detail;

namespace Acts::Test {

BOOST_AUTO_TEST_CASE(bfield_creation) {
  // create grid values
  std::vector<double> rPos = {0., 1., 2., 3.};
  std::vector<double> xPos = {0., 1., 2., 3.};
  std::vector<double> yPos = {0., 1., 2., 3.};
  std::vector<double> zPos = {0., 1., 2., 3.};

  // create b field in rz
  std::vector<Acts::Vector2> bField_rz;
  for (int i = 0; i < 27; i++) {
    bField_rz.push_back(Acts::Vector2(i, i));
  }

  auto localToGlobalBin_rz = [](std::array<std::size_t, 2> binsRZ,
                                std::array<std::size_t, 2> nBinsRZ) {
    return (binsRZ.at(1) * nBinsRZ.at(0) + binsRZ.at(0));
  };
  // create b field map in rz
  auto map_rz =
      Acts::fieldMapRZ(localToGlobalBin_rz, rPos, zPos, bField_rz, 1, 1, false);
  // check number of bins, minima & maxima
  std::vector<std::size_t> nBins_rz = {rPos.size(), zPos.size()};
  std::vector<double> minima_rz = {0., 0.};
  std::vector<double> maxima_rz = {3., 3.};
  BOOST_CHECK(map_rz.getNBins() == nBins_rz);
  // check minimum (should be first value because bin values are always
  // assigned to the left boundary)
  BOOST_CHECK(map_rz.getMin() == minima_rz);
  // check maximum (should be last value + 1 step because bin values are
  // always assigned to the left boundary)
  BOOST_CHECK(map_rz.getMax() == maxima_rz);

  auto localToGlobalBin_xyz = [](std::array<std::size_t, 3> binsXYZ,
                                 std::array<std::size_t, 3> nBinsXYZ) {
    return (binsXYZ.at(0) * (nBinsXYZ.at(1) * nBinsXYZ.at(2)) +
            binsXYZ.at(1) * nBinsXYZ.at(2) + binsXYZ.at(2));
  };

  rPos = {0., 1., 2., 3.};
  xPos = {0., 1., 2., 3.};
  yPos = {0., 1., 2., 3.};
  zPos = {0., 1., 2., 3.};

  // create b field in xyz
  std::vector<Acts::Vector3> bField_xyz;
  for (int i = 0; i < 81; i++) {
    bField_xyz.push_back(Acts::Vector3(i, i, i));
  }

  // create b field map in xyz
  auto map_xyz = Acts::fieldMapXYZ(localToGlobalBin_xyz, xPos, yPos, zPos,
                                   bField_xyz, 1, 1, false);
  // check number of bins, minima & maxima
  std::vector<std::size_t> nBins_xyz = {xPos.size(), yPos.size(), zPos.size()};
  std::vector<double> minima_xyz = {0., 0., 0.};
  std::vector<double> maxima_xyz = {3., 3., 3.};
  BOOST_CHECK(map_xyz.getNBins() == nBins_xyz);
  // check minimum (should be first value because bin values are always
  // assigned to the left boundary)
  BOOST_CHECK(map_xyz.getMin() == minima_xyz);
  // check maximum (should be last value + 1 step because bin values are
  // always assigned to the left boundary)
  BOOST_CHECK(map_xyz.getMax() == maxima_xyz);

  // check if filled value is expected value in rz
  Acts::Vector3 pos0_rz(0., 0., 0.);
  Acts::Vector3 pos1_rz(1., 0., 1.);
  Acts::Vector3 pos2_rz(0., 2., 2.);
  auto value0_rz = map_rz.getField(pos0_rz).value();
  auto value1_rz = map_rz.getField(pos1_rz).value();
  auto value2_rz = map_rz.getField(pos2_rz).value();
  // calculate what the value should be at this point
  auto b0_rz =
      bField_rz.at(localToGlobalBin_rz({{0, 0}}, {{rPos.size(), zPos.size()}}));
  auto b1_rz =
      bField_rz.at(localToGlobalBin_rz({{1, 1}}, {{rPos.size(), zPos.size()}}));
  auto b2_rz =
      bField_rz.at(localToGlobalBin_rz({{2, 2}}, {{rPos.size(), zPos.size()}}));
  Acts::Vector3 bField0_rz(b0_rz.x(), 0., b0_rz.y());
  Acts::Vector3 bField1_rz(b1_rz.x(), 0., b1_rz.y());
  Acts::Vector3 bField2_rz(0., b2_rz.x(), b2_rz.y());
  // check the value
  // in rz case field is phi symmetric (check radius)
  CHECK_CLOSE_ABS(perp(value0_rz), perp(bField0_rz), 1e-9);
  CHECK_CLOSE_ABS(value0_rz.z(), bField0_rz.z(), 1e-9);
  CHECK_CLOSE_ABS(perp(value1_rz), perp(bField1_rz), 1e-9);
  CHECK_CLOSE_ABS(value1_rz.z(), bField1_rz.z(), 1e-9);
  CHECK_CLOSE_ABS(perp(value2_rz), perp(bField2_rz), 1e-9);
  CHECK_CLOSE_ABS(value2_rz.z(), bField2_rz.z(), 1e-9);

  // check if filled value is expected value in rz
  Acts::Vector3 pos0_xyz(0., 0., 0.);
  Acts::Vector3 pos1_xyz(1., 1., 1.);
  Acts::Vector3 pos2_xyz(2., 2., 2.);
  auto value0_xyz = map_xyz.getField(pos0_xyz).value();
  auto value1_xyz = map_xyz.getField(pos1_xyz).value();
  auto value2_xyz = map_xyz.getField(pos2_xyz).value();
  // calculate what the value should be at this point
  auto b0_xyz = bField_xyz.at(localToGlobalBin_xyz(
      {{0, 0, 0}}, {{xPos.size(), yPos.size(), zPos.size()}}));
  auto b1_xyz = bField_xyz.at(localToGlobalBin_xyz(
      {{1, 1, 1}}, {{xPos.size(), yPos.size(), zPos.size()}}));
  auto b2_xyz = bField_xyz.at(localToGlobalBin_xyz(
      {{2, 2, 2}}, {{xPos.size(), yPos.size(), zPos.size()}}));
  // check the value
  BOOST_CHECK_EQUAL(value0_xyz, b0_xyz);
  BOOST_CHECK_EQUAL(value1_xyz, b1_xyz);
  BOOST_CHECK_EQUAL(value2_xyz, b2_xyz);

  // checkx-,y-,z-symmetry - need to check close (because of interpolation
  // there can be small differences)
  CHECK_CLOSE_ABS(value0_xyz, b0_xyz, 1e-9);
  CHECK_CLOSE_ABS(value1_xyz, b1_xyz, 1e-9);
  CHECK_CLOSE_ABS(value2_xyz, b2_xyz, 1e-9);
}

BOOST_AUTO_TEST_CASE(bfield_symmetry) {
  // create grid values
  std::vector<double> rPos = {0., 1., 2.};
  std::vector<double> xPos = {0., 1., 2.};
  std::vector<double> yPos = {0., 1., 2.};
  std::vector<double> zPos = {0., 1., 2.};
  // the bfield values in rz
  std::vector<Acts::Vector2> bField_rz;
  for (int i = 0; i < 9; i++) {
    bField_rz.push_back(Acts::Vector2(i, i));
  }
  // the field map in rz
  auto map_rz = Acts::fieldMapRZ(
      [](std::array<std::size_t, 2> binsRZ,
         std::array<std::size_t, 2> nBinsRZ) {
        return (binsRZ.at(1) * nBinsRZ.at(0) + binsRZ.at(0));
      },
      rPos, zPos, bField_rz, 1, 1, true);

  // check number of bins, minima & maxima
  std::vector<std::size_t> nBins_rz = {rPos.size(), 2 * zPos.size() - 1};
  std::vector<double> minima_rz = {0., -2.};
  std::vector<double> maxima_rz = {2., 2.};
  BOOST_CHECK(map_rz.getNBins() == nBins_rz);
  auto vec = map_rz.getNBins();
  auto vec0 = map_rz.getMin();
  // check minimum (should be first value because bin values are always
  // assigned to the left boundary)
  BOOST_CHECK(map_rz.getMin() == minima_rz);
  // check maximum (should be last value + 1 step because bin values are
  // always assigned to the left boundary)
  BOOST_CHECK(map_rz.getMax() == maxima_rz);

  // the bfield values in xyz
  std::vector<Acts::Vector3> bField_xyz;
  for (int i = 0; i < 27; i++) {
    bField_xyz.push_back(Acts::Vector3(i, i, i));
  }
  // the field map in xyz
  auto map_xyz = Acts::fieldMapXYZ(
      [](std::array<std::size_t, 3> binsXYZ,
         std::array<std::size_t, 3> nBinsXYZ) {
        return (binsXYZ.at(0) * (nBinsXYZ.at(1) * nBinsXYZ.at(2)) +
                binsXYZ.at(1) * nBinsXYZ.at(2) + binsXYZ.at(2));
      },
      xPos, yPos, zPos, bField_xyz, 1, 1, true);

  // check number of bins, minima & maxima
  std::vector<std::size_t> nBins_xyz = {
      2 * xPos.size() - 1, 2 * yPos.size() - 1, 2 * zPos.size() - 1};
  std::vector<double> minima_xyz = {-2., -2., -2.};
  std::vector<double> maxima_xyz = {2., 2., 2.};
  BOOST_CHECK(map_xyz.getNBins() == nBins_xyz);
  // check minimum (should be first value because bin values are always
  // assigned to the left boundary)
  BOOST_CHECK(map_xyz.getMin() == minima_xyz);
  // check maximum (should be last value + 1 step because bin values are
  // always assigned to the left boundary)
  BOOST_CHECK(map_xyz.getMax() == maxima_xyz);

  Acts::Vector3 pos0(1.2, 1.3, 1.4);
  Acts::Vector3 pos1(1.2, 1.3, -1.4);
  Acts::Vector3 pos2(-1.2, 1.3, 1.4);
  Acts::Vector3 pos3(1.2, -1.3, 1.4);
  Acts::Vector3 pos4(-1.2, -1.3, 1.4);

  auto value0_rz = map_rz.getField(pos0).value();
  auto value1_rz = map_rz.getField(pos1).value();
  auto value2_rz = map_rz.getField(pos2).value();
  auto value3_rz = map_rz.getField(pos3).value();
  auto value4_rz = map_rz.getField(pos4).value();

  auto value0_xyz = map_xyz.getField(pos0).value();
  auto value1_xyz = map_xyz.getField(pos1).value();
  auto value2_xyz = map_xyz.getField(pos2).value();
  auto value3_xyz = map_xyz.getField(pos3).value();
  auto value4_xyz = map_xyz.getField(pos4).value();

  // check z- and phi-symmetry
  CHECK_CLOSE_REL(perp(value0_rz), perp(value1_rz), 1e-10);
  CHECK_CLOSE_REL(value0_rz.z(), value1_rz.z(), 1e-10);
  CHECK_CLOSE_REL(perp(value0_rz), perp(value2_rz), 1e-10);
  CHECK_CLOSE_REL(value0_rz.z(), value2_rz.z(), 1e-10);
  CHECK_CLOSE_REL(perp(value0_rz), perp(value3_rz), 1e-10);
  CHECK_CLOSE_REL(value0_rz.z(), value3_rz.z(), 1e-10);
  CHECK_CLOSE_REL(perp(value0_rz), perp(value4_rz), 1e-10);
  CHECK_CLOSE_REL(value0_rz.z(), value4_rz.z(), 1e-10);

  // checkx-,y-,z-symmetry - need to check close (because of interpolation
  // there can be small differences)
  CHECK_CLOSE_REL(value0_xyz, value1_xyz, 1e-10);
  CHECK_CLOSE_REL(value0_xyz, value2_xyz, 1e-10);
  CHECK_CLOSE_REL(value0_xyz, value3_xyz, 1e-10);
  CHECK_CLOSE_REL(value0_xyz, value4_xyz, 1e-10);
}

/// Unit test for symmetric data
BOOST_DATA_TEST_CASE(
    bfield_symmetry_random,
    bdata::random((bdata::engine = std::mt19937(), bdata::seed = 0,
                   bdata::distribution =
                       std::uniform_real_distribution<double>(-10., 10.))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 0,
                       bdata::distribution =
                           std::uniform_real_distribution<double>(-10., 10.))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 0,
                       bdata::distribution =
                           std::uniform_real_distribution<double>(-20., 20.))) ^
        bdata::xrange(10),
    x, y, z, index) {
  (void)index;

  std::vector<double> rPos;
  std::vector<double> xPos;
  std::vector<double> yPos;
  std::vector<double> zPos;
  double maxR = 20.;
  double maxZ = 30.;
  double maxBr = 10.;
  double maxBz = 20.;
  std::size_t nBins = 10;
  double stepR = maxR / nBins;
  double stepZ = maxZ / nBins;
  double bStepR = maxBr / nBins;
  double bStepZ = maxBz / nBins;

  for (std::size_t i = 0; i < nBins; i++) {
    rPos.push_back(i * stepR);
    xPos.push_back(i * stepR);
    yPos.push_back(i * stepR);
    zPos.push_back(i * stepZ);
  }
  // bfield in rz
  std::vector<Acts::Vector2> bField_rz;
  for (std::size_t i = 0; i < nBins * nBins; i++) {
    bField_rz.push_back(Acts::Vector2(i * bStepR, i * bStepZ));
  }
  // the map in rz
  auto map_rz = Acts::fieldMapRZ(
      [](std::array<std::size_t, 2> binsRZ,
         std::array<std::size_t, 2> nBinsRZ) {
        return (binsRZ.at(1) * nBinsRZ.at(0) + binsRZ.at(0));
      },
      rPos, zPos, bField_rz, 1, 1, true);

  // check number of bins, minima & maxima
  std::vector<std::size_t> nBins_rz = {rPos.size(), 2 * zPos.size() - 1};
  std::vector<double> minima_rz = {0., -((nBins - 1) * stepZ)};
  std::vector<double> maxima_rz = {(nBins - 1) * stepR, (nBins - 1) * stepZ};
  BOOST_CHECK(map_rz.getNBins() == nBins_rz);
  // check minimum (should be first value because bin values are always
  // assigned to the left boundary)
  CHECK_CLOSE_ABS(map_rz.getMin(), minima_rz, 1e-10);
  // check maximum (should be last value + 1 step because bin values are
  // always assigned to the left boundary)
  CHECK_CLOSE_ABS(map_rz.getMax(), maxima_rz, 1e-10);

  // bfield in xyz
  std::vector<Acts::Vector3> bField_xyz;
  for (std::size_t i = 0; i < nBins * nBins * nBins; i++) {
    bField_xyz.push_back(Acts::Vector3(i * bStepR, i * bStepR, i * bStepZ));
  }
  // the map in xyz
  auto map_xyz = Acts::fieldMapXYZ(
      [](std::array<std::size_t, 3> binsXYZ,
         std::array<std::size_t, 3> nBinsXYZ) {
        return (binsXYZ.at(0) * (nBinsXYZ.at(1) * nBinsXYZ.at(2)) +
                binsXYZ.at(1) * nBinsXYZ.at(2) + binsXYZ.at(2));
      },
      xPos, yPos, zPos, bField_xyz, 1, 1, true);
  // check number of bins, minima & maxima
  std::vector<std::size_t> nBins_xyz = {
      2 * xPos.size() - 1, 2 * yPos.size() - 1, 2 * zPos.size() - 1};
  std::vector<double> minima_xyz = {
      -((nBins - 1) * stepR), -((nBins - 1) * stepR), -((nBins - 1) * stepZ)};
  std::vector<double> maxima_xyz = {(nBins - 1) * stepR, (nBins - 1) * stepR,
                                    (nBins - 1) * stepZ};
  BOOST_CHECK(map_xyz.getNBins() == nBins_xyz);
  // check minimum (should be first value because bin values are always
  // assigned to the left boundary)
  CHECK_CLOSE_REL(map_xyz.getMin(), minima_xyz, 1e-10);
  // check maximum (should be last value + 1 step because bin values are
  // always assigned to the left boundary)
  CHECK_CLOSE_REL(map_xyz.getMax(), maxima_xyz, 1e-10);

  Acts::Vector3 pos0(x, y, z);
  Acts::Vector3 pos1(x, y, -z);
  Acts::Vector3 pos2(-x, y, z);
  Acts::Vector3 pos3(x, -y, z);
  Acts::Vector3 pos4(-x, -y, z);

  auto value0_rz = map_rz.getField(pos0).value();
  auto value1_rz = map_rz.getField(pos1).value();
  auto value2_rz = map_rz.getField(pos2).value();
  auto value3_rz = map_rz.getField(pos3).value();
  auto value4_rz = map_rz.getField(pos4).value();

  // check z- and phi-symmetry
  CHECK_CLOSE_REL(perp(value0_rz), perp(value1_rz), 1e-10);
  CHECK_CLOSE_REL(value0_rz.z(), value1_rz.z(), 1e-10);
  CHECK_CLOSE_REL(perp(value0_rz), perp(value2_rz), 1e-10);
  CHECK_CLOSE_REL(value0_rz.z(), value2_rz.z(), 1e-10);
  CHECK_CLOSE_REL(perp(value0_rz), perp(value3_rz), 1e-10);
  CHECK_CLOSE_REL(value0_rz.z(), value3_rz.z(), 1e-10);
  CHECK_CLOSE_REL(perp(value0_rz), perp(value4_rz), 1e-10);
  CHECK_CLOSE_REL(value0_rz.z(), value4_rz.z(), 1e-10);

  auto value0_xyz = map_xyz.getField(pos0).value();
  auto value1_xyz = map_xyz.getField(pos1).value();
  auto value2_xyz = map_xyz.getField(pos2).value();
  auto value3_xyz = map_xyz.getField(pos3).value();
  auto value4_xyz = map_xyz.getField(pos4).value();

  // checkx-,y-,z-symmetry - need to check close (because of interpolation
  // there can be small differences)
  CHECK_CLOSE_REL(value0_xyz, value1_xyz, 1e-10);
  CHECK_CLOSE_REL(value0_xyz, value2_xyz, 1e-10);
  CHECK_CLOSE_REL(value0_xyz, value3_xyz, 1e-10);
  CHECK_CLOSE_REL(value0_xyz, value4_xyz, 1e-10);
}

}  // namespace Acts::Test
