// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Material/AccumulatedMaterialSlab.hpp"
#include "Acts/Material/AccumulatedSurfaceMaterial.hpp"
#include "Acts/Material/Material.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/BinningType.hpp"

#include <array>
#include <cstddef>
#include <memory>
#include <vector>

namespace Acts::Test {

/// Test the constructors
BOOST_AUTO_TEST_CASE(AccumulatedSurfaceMaterial_construction_test) {
  // Test:
  // HomogeneousSurfaceMaterial accumulation
  AccumulatedSurfaceMaterial material0D{};
  auto accMat0D = material0D.accumulatedMaterial();
  BOOST_CHECK_EQUAL(accMat0D.size(), 1u);
  BOOST_CHECK_EQUAL(accMat0D[0].size(), 1u);
  BOOST_CHECK_EQUAL(material0D.splitFactor(), 0.);

  // Test:
  // BinsSurfaceMaterial accumulation - 1D
  BinUtility binUtility1D(10, -5., 5., open, BinningValue::binX);
  AccumulatedSurfaceMaterial material1D{binUtility1D};
  auto accMat1D = material1D.accumulatedMaterial();
  BOOST_CHECK_EQUAL(accMat1D.size(), 1u);
  BOOST_CHECK_EQUAL(accMat1D[0].size(), 10u);

  // Test:
  // BinsSurfaceMaterial accumulation - 2D
  BinUtility binUtility2D(10, -5., 5., open, BinningValue::binX);
  binUtility2D += BinUtility(20, -10., 10., open, BinningValue::binY);
  AccumulatedSurfaceMaterial material2D{binUtility2D};
  auto accMat2D = material2D.accumulatedMaterial();
  BOOST_CHECK_EQUAL(accMat2D.size(), 20u);
  for (std::size_t ib = 0; ib < accMat2D.size(); ++ib) {
    BOOST_CHECK_EQUAL(accMat2D[ib].size(), 10u);
  }
}

/// Test the filling and conversion
BOOST_AUTO_TEST_CASE(AccumulatedSurfaceMaterial_fill_convert_0D) {
  Material mat = Material::fromMolarDensity(1., 1., 1., 1., 1.);
  MaterialSlab one(mat, 1.);
  MaterialSlab two(mat, 2.);

  AccumulatedSurfaceMaterial material0D{};
  const std::vector<std::array<std::size_t, 3>> bin;
  // assign 2 one steps
  material0D.accumulate(Vector2{0., 0.}, one);
  material0D.accumulate(Vector2{0., 0.}, one);
  material0D.trackVariance(bin, one);
  material0D.trackAverage();
  // assign 1 double step
  material0D.accumulate(Vector3(0., 0., 0.), two);
  material0D.trackVariance(bin, one);
  material0D.trackAverage();
  // get the single matrix
  auto accMat0D = material0D.accumulatedMaterial();
  auto accMatProp0D = accMat0D[0][0];
  auto [matProp0D, trackCount] = accMatProp0D.totalAverage();
  auto [matVar0D, trackCount2] = accMatProp0D.totalVariance();

  BOOST_CHECK_EQUAL(matVar0D, 0.0);
  BOOST_CHECK_EQUAL(matProp0D.thicknessInX0(), two.thicknessInX0());
  BOOST_CHECK_EQUAL(trackCount, trackCount2);
  BOOST_CHECK_EQUAL(trackCount, 2u);
}

/// Test the filling and conversion
BOOST_AUTO_TEST_CASE(AccumulatedSurfaceMaterial_fill_convert_1D) {
  Material mat = Material::fromMolarDensity(1., 1., 1., 1., 1.);
  MaterialSlab one(mat, 1.);
  MaterialSlab two(mat, 2.);
  MaterialSlab three(mat, 3.);
  MaterialSlab four(mat, 4.);

  // BinsSurfaceMaterial accumulation - 2D
  BinUtility binUtility2D(2, -1., 1., open, BinningValue::binX);
  binUtility2D += BinUtility(2, -1., 1., open, BinningValue::binY);
  AccumulatedSurfaceMaterial material2D{binUtility2D};
  const std::vector<std::array<std::size_t, 3>> bin;

  // assign in the different bins
  // event 0
  material2D.accumulate(Vector2{-0.5, -0.5}, one);
  material2D.accumulate(Vector2{0.5, -0.5}, two);
  material2D.accumulate(Vector2{-0.5, 0.5}, three);
  material2D.accumulate(Vector2{0.5, 0.5}, four);
  material2D.trackVariance(bin, one);
  material2D.trackAverage();
  // event 1
  material2D.accumulate(Vector2{0.5, -0.5}, two);
  material2D.accumulate(Vector2{-0.5, 0.5}, three);
  material2D.accumulate(Vector2{0.5, 0.5}, four);
  material2D.trackVariance(bin, one);
  material2D.trackAverage();
  // event 2
  material2D.accumulate(Vector2{-0.5, 0.5}, three);
  material2D.accumulate(Vector2{0.5, 0.5}, four);
  material2D.trackVariance(bin, one);
  material2D.trackAverage();
  // event 2
  material2D.accumulate(Vector2{0.5, 0.5}, four);
  material2D.trackVariance(bin, one);
  material2D.trackAverage();
  // get the single matrix
  auto accMat2D = material2D.accumulatedMaterial();
  // the accumulated properties
  auto [accMatProp00, trackCount00] = accMat2D[0][0].totalAverage();
  auto [accMatProp01, trackCount01] = accMat2D[0][1].totalAverage();
  auto [accMatProp10, trackCount10] = accMat2D[1][0].totalAverage();
  auto [accMatProp11, trackCount11] = accMat2D[1][1].totalAverage();

  auto [matVar00, trackCount200] = accMat2D[0][0].totalVariance();
  auto [matVar01, trackCount201] = accMat2D[0][1].totalVariance();
  auto [matVar10, trackCount210] = accMat2D[1][0].totalVariance();
  auto [matVar11, trackCount211] = accMat2D[1][1].totalVariance();

  BOOST_CHECK_EQUAL(accMatProp00.thicknessInX0(), one.thicknessInX0());
  BOOST_CHECK_EQUAL(accMatProp01.thicknessInX0(), two.thicknessInX0());
  BOOST_CHECK_EQUAL(accMatProp10.thicknessInX0(), three.thicknessInX0());
  BOOST_CHECK_EQUAL(accMatProp11.thicknessInX0(), four.thicknessInX0());

  BOOST_CHECK_EQUAL(matVar11, 0.0);
  BOOST_CHECK_EQUAL(matVar11, 0.0);
  BOOST_CHECK_EQUAL(matVar11, 0.0);
  BOOST_CHECK_EQUAL(matVar11, 0.0);

  BOOST_CHECK_EQUAL(trackCount00, trackCount200);
  BOOST_CHECK_EQUAL(trackCount01, trackCount201);
  BOOST_CHECK_EQUAL(trackCount10, trackCount210);
  BOOST_CHECK_EQUAL(trackCount11, trackCount211);

  BOOST_CHECK_EQUAL(trackCount00, 1u);
  BOOST_CHECK_EQUAL(trackCount01, 2u);
  BOOST_CHECK_EQUAL(trackCount10, 3u);
  BOOST_CHECK_EQUAL(trackCount11, 4u);
}

/// Test the variance
BOOST_AUTO_TEST_CASE(AccumulatedSurfaceMaterial_variance_0D) {
  Material mat1 = Material::fromMolarDensity(1.0 / 3, 1., 1., 1., 1.);
  Material mat2 = Material::fromMolarDensity(1, 1., 1., 1., 1.);
  Material matAvg = Material::fromMolarDensity(0.5, 1., 1., 1., 1.);

  MaterialSlab one(mat1, 1.);
  MaterialSlab two(mat2, 1.);
  MaterialSlab avg(matAvg, 1.);

  AccumulatedSurfaceMaterial material0D{};
  const std::vector<std::array<std::size_t, 3>> bin;
  // assign 2 one steps
  material0D.accumulate(Vector2{0., 0.}, one);
  material0D.trackVariance(bin, avg);
  material0D.trackAverage();
  // assign 1 double step
  material0D.accumulate(Vector3(0., 0., 0.), two);
  material0D.trackVariance(bin, avg);
  material0D.trackAverage();
  // get the single matrix
  auto accMat0D = material0D.accumulatedMaterial();
  auto accMatProp0D = accMat0D[0][0];
  auto [matProp0D, trackCount] = accMatProp0D.totalAverage();
  auto [matVar0D, trackCount2] = accMatProp0D.totalVariance();

  BOOST_CHECK_EQUAL(matVar0D, 1.0);
  BOOST_CHECK_EQUAL(matProp0D.thicknessInX0(), avg.thicknessInX0());
  BOOST_CHECK_EQUAL(trackCount, trackCount2);
  BOOST_CHECK_EQUAL(trackCount, 2u);
}

}  // namespace Acts::Test
