// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Material/Material.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Material/detail/AverageMaterials.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Tests/CommonHelpers/PredefinedMaterials.hpp"

#include <cmath>
#include <limits>

namespace {

using Acts::detail::combineSlabs;

constexpr auto eps = std::numeric_limits<float>::epsilon();

// vacuum w/ different thickness
const Acts::MaterialSlab zeroVacuum = Acts::MaterialSlab(0.0f);
const Acts::MaterialSlab unitVacuum = Acts::MaterialSlab(1.0f);
// same material corresponding to 0%, 1% and 100% radiation/interaction length
const Acts::MaterialSlab zero(Acts::Test::makeSilicon(), 0.0f);
const Acts::MaterialSlab percent = Acts::Test::makePercentSlab();
const Acts::MaterialSlab unit = Acts::Test::makeUnitSlab();

}  // namespace

BOOST_AUTO_TEST_SUITE(AverageMaterials)

// average two identical slabs

BOOST_AUTO_TEST_CASE(CombineSlabsVacuum) {
  // vacuum with zero thickness
  {
    auto slab = combineSlabs(zeroVacuum, zeroVacuum);
    BOOST_CHECK(!slab.isValid());
    BOOST_CHECK(!slab.material().isValid());
    BOOST_CHECK_EQUAL(slab.thickness(), 0.0f);
    BOOST_CHECK_EQUAL(slab.thicknessInX0(), 0.0f);
    BOOST_CHECK_EQUAL(slab.thicknessInL0(), 0.0f);
  }
  // vacuum with unit thickness
  {
    auto slab = combineSlabs(unitVacuum, unitVacuum);
    BOOST_CHECK(!slab.isValid());
    BOOST_CHECK(!slab.material().isValid());
    BOOST_CHECK_EQUAL(slab.thickness(), 2.0f);
    BOOST_CHECK_EQUAL(slab.thicknessInX0(), 0.0f);
    BOOST_CHECK_EQUAL(slab.thicknessInL0(), 0.0f);
  }
}

BOOST_AUTO_TEST_CASE(CombineSlabsPercent) {
  auto slab = combineSlabs(percent, percent);
  // combining two identical slabs must give the same average material
  BOOST_CHECK(slab.isValid());
  BOOST_CHECK(slab.material().isValid());
  BOOST_CHECK_EQUAL(slab.material(), percent.material());
  // thickness-like properties must double
  BOOST_CHECK_EQUAL(slab.thickness(), 2 * percent.thickness());
  BOOST_CHECK_EQUAL(slab.thicknessInX0(), 2 * percent.thicknessInX0());
  BOOST_CHECK_EQUAL(slab.thicknessInL0(), 2 * percent.thicknessInL0());
}

BOOST_AUTO_TEST_CASE(CombineSlabsUnit) {
  auto slab = combineSlabs(unit, unit);
  // combining two identical slabs must give the same average material
  BOOST_CHECK(slab.isValid());
  BOOST_CHECK(slab.material().isValid());
  BOOST_CHECK_EQUAL(slab.material(), unit.material());
  // thickness-like properties must double
  BOOST_CHECK_EQUAL(slab.thickness(), 2 * unit.thickness());
  BOOST_CHECK_EQUAL(slab.thicknessInX0(), 2 * unit.thicknessInX0());
  BOOST_CHECK_EQUAL(slab.thicknessInL0(), 2 * unit.thicknessInL0());
}

// average a non-vacuum slab and a zero-thickness vacuum slab

BOOST_AUTO_TEST_CASE(CombineSlabsPercentZeroVacuum) {
  {
    auto slab = combineSlabs(percent, zeroVacuum);
    BOOST_CHECK(slab.isValid());
    BOOST_CHECK(slab.material().isValid());
    BOOST_CHECK_EQUAL(slab.material(), percent.material());
    BOOST_CHECK_EQUAL(slab.thickness(), percent.thickness());
    BOOST_CHECK_EQUAL(slab.thicknessInX0(), percent.thicknessInX0());
    BOOST_CHECK_EQUAL(slab.thicknessInL0(), percent.thicknessInL0());
  }
  // reverse input order
  {
    auto slab = combineSlabs(zeroVacuum, percent);
    BOOST_CHECK(slab.isValid());
    BOOST_CHECK(slab.material().isValid());
    BOOST_CHECK_EQUAL(slab.material(), percent.material());
    BOOST_CHECK_EQUAL(slab.thickness(), percent.thickness());
    BOOST_CHECK_EQUAL(slab.thicknessInX0(), percent.thicknessInX0());
    BOOST_CHECK_EQUAL(slab.thicknessInL0(), percent.thicknessInL0());
  }
}

BOOST_AUTO_TEST_CASE(CombineSlabsUnitZeroVacuum) {
  {
    auto slab = combineSlabs(unit, zeroVacuum);
    BOOST_CHECK(slab.isValid());
    BOOST_CHECK(slab.material().isValid());
    BOOST_CHECK_EQUAL(slab.material(), unit.material());
    BOOST_CHECK_EQUAL(slab.thickness(), unit.thickness());
    BOOST_CHECK_EQUAL(slab.thicknessInX0(), unit.thicknessInX0());
    BOOST_CHECK_EQUAL(slab.thicknessInL0(), unit.thicknessInL0());
  }
  // reverse input order
  {
    auto slab = combineSlabs(zeroVacuum, unit);
    BOOST_CHECK(slab.isValid());
    BOOST_CHECK(slab.material().isValid());
    BOOST_CHECK_EQUAL(slab.material(), unit.material());
    BOOST_CHECK_EQUAL(slab.thickness(), unit.thickness());
    BOOST_CHECK_EQUAL(slab.thicknessInX0(), unit.thicknessInX0());
    BOOST_CHECK_EQUAL(slab.thicknessInL0(), unit.thicknessInL0());
  }
}

// average two non-vacuum slabs with the same material but different thickness

BOOST_AUTO_TEST_CASE(CombineSlabsPercentUnit) {
  // the two slabs have the same material -> average should be identical
  {
    auto slab = combineSlabs(percent, unit);
    BOOST_CHECK(slab.isValid());
    BOOST_CHECK(slab.material().isValid());
    BOOST_CHECK_EQUAL(slab.material(), percent.material());
    BOOST_CHECK_EQUAL(slab.thickness(), percent.thickness() + unit.thickness());
    BOOST_CHECK_EQUAL(slab.thicknessInX0(),
                      percent.thicknessInX0() + unit.thicknessInX0());
    BOOST_CHECK_EQUAL(slab.thicknessInL0(),
                      percent.thicknessInL0() + unit.thicknessInL0());
  }
  // reverse input order
  {
    auto slab = combineSlabs(unit, percent);
    BOOST_CHECK(slab.isValid());
    BOOST_CHECK(slab.material().isValid());
    BOOST_CHECK_EQUAL(slab.material(), unit.material());
    BOOST_CHECK_EQUAL(slab.thickness(), unit.thickness() + percent.thickness());
    BOOST_CHECK_EQUAL(slab.thicknessInX0(),
                      percent.thicknessInX0() + unit.thicknessInX0());
    BOOST_CHECK_EQUAL(slab.thicknessInL0(),
                      percent.thicknessInL0() + unit.thicknessInL0());
  }
}

// average two non-vacuum slabs where one has zero thickness

BOOST_AUTO_TEST_CASE(CombineSlabsUnitZero) {
  // the two slabs have the same material -> average should be identical
  {
    auto slab = combineSlabs(unit, zero);
    BOOST_CHECK(slab.isValid());
    BOOST_CHECK(slab.material().isValid());
    BOOST_CHECK_EQUAL(slab.material(), unit.material());
    BOOST_CHECK_EQUAL(slab.thickness(), unit.thickness());
    BOOST_CHECK_EQUAL(slab.thicknessInX0(), unit.thicknessInX0());
    BOOST_CHECK_EQUAL(slab.thicknessInL0(), unit.thicknessInL0());
  }
  // reverse input order
  {
    auto slab = combineSlabs(zero, unit);
    BOOST_CHECK(slab.isValid());
    BOOST_CHECK(slab.material().isValid());
    BOOST_CHECK_EQUAL(slab.material(), unit.material());
    BOOST_CHECK_EQUAL(slab.thickness(), unit.thickness());
    BOOST_CHECK_EQUAL(slab.thicknessInX0(), unit.thicknessInX0());
    BOOST_CHECK_EQUAL(slab.thicknessInL0(), unit.thicknessInL0());
  }
}

// average a non-vacuum and a vacuum slab w/ equal thickness

BOOST_AUTO_TEST_CASE(CombineSlabsEqualThicknessVacuum) {
  const auto mat = Acts::Test::makeSilicon();
  const auto slabMat = Acts::MaterialSlab(mat, 1.0f);
  const auto slabVac = Acts::MaterialSlab(Acts::Material(), 1.0f);
  {
    auto slab = combineSlabs(slabMat, slabVac);
    BOOST_CHECK(slab.isValid());
    BOOST_CHECK(slab.material().isValid());
    BOOST_CHECK_EQUAL(slab.thickness(), 2.0f);
    BOOST_CHECK_EQUAL(slab.thicknessInX0(), slabMat.thicknessInX0());
    BOOST_CHECK_EQUAL(slab.thicknessInL0(), slabMat.thicknessInL0());
    // atomic mass and nuclear charge are per atom, adding any amount vacuum
    // does not change the average atom properties only their density
    BOOST_CHECK_EQUAL(slab.material().Ar(), mat.Ar());
    BOOST_CHECK_EQUAL(slab.material().Z(), 0.5 * mat.Z());
    // we have the same type of interactions just spread over twice the length
    CHECK_CLOSE_REL(slab.material().X0(), 2.0f * mat.X0(), eps);
    CHECK_CLOSE_REL(slab.material().L0(), 2.0f * mat.L0(), eps);
    // we have the same atoms just spread over more volume
    BOOST_CHECK_EQUAL(slab.material().molarDensity(),
                      0.5f * mat.molarDensity());
  }
  // reverse input order
  {
    auto slab = combineSlabs(slabVac, slabMat);
    BOOST_CHECK(slab.isValid());
    BOOST_CHECK(slab.material().isValid());
    BOOST_CHECK_EQUAL(slab.thickness(), 2.0f);
    BOOST_CHECK_EQUAL(slab.thicknessInX0(), slabMat.thicknessInX0());
    BOOST_CHECK_EQUAL(slab.thicknessInL0(), slabMat.thicknessInL0());
    // atomic mass and nuclear charge are per atom, adding any amount vacuum
    // does not change the average atom properties only their density
    BOOST_CHECK_EQUAL(slab.material().Ar(), mat.Ar());
    BOOST_CHECK_EQUAL(slab.material().Z(), 0.5 * mat.Z());
    // we have the same type of interactions just spread over twice the length
    CHECK_CLOSE_REL(slab.material().X0(), 2.0f * mat.X0(), eps);
    CHECK_CLOSE_REL(slab.material().L0(), 2.0f * mat.L0(), eps);
    // we have the same atoms just spread over more volume
    BOOST_CHECK_EQUAL(slab.material().molarDensity(),
                      0.5f * mat.molarDensity());
  }
}

// average two non-vacuum slabs w/ different material and different thickness

BOOST_AUTO_TEST_CASE(CombineSlabs) {
  const auto mat0 = Acts::Material::fromMolarDensity(1, 1, 8, 12, 2);
  const auto mat1 = Acts::Material::fromMolarDensity(2, 2, 2, 6, 5);
  const auto slabMat0 = Acts::MaterialSlab(mat0, 0.5f);
  const auto slabMat1 = Acts::MaterialSlab(mat1, 1.0f);
  // verify derived quantities for the input slabs. these tests are not really
  // needed, but to show the input values for the tests below.
  BOOST_CHECK(slabMat0.isValid());
  BOOST_CHECK(slabMat0.material().isValid());
  BOOST_CHECK_EQUAL(slabMat0.thickness(), 0.5f);
  BOOST_CHECK_EQUAL(slabMat0.thicknessInX0(), 0.5f);
  BOOST_CHECK_EQUAL(slabMat0.thicknessInL0(), 0.5f);
  BOOST_CHECK(slabMat1.isValid());
  BOOST_CHECK(slabMat1.material().isValid());
  BOOST_CHECK_EQUAL(slabMat1.thickness(), 1.0f);
  BOOST_CHECK_EQUAL(slabMat1.thicknessInX0(), 0.5f);
  BOOST_CHECK_EQUAL(slabMat1.thicknessInL0(), 0.5f);
  // check combined slabs
  {
    auto slab = combineSlabs(slabMat0, slabMat1);
    BOOST_CHECK(slab.isValid());
    BOOST_CHECK(slab.material().isValid());
    BOOST_CHECK_EQUAL(slab.thickness(), 1.5f);
    BOOST_CHECK_EQUAL(slab.thicknessInX0(), 1.0f);
    BOOST_CHECK_EQUAL(slab.thicknessInL0(), 1.0f);
    BOOST_CHECK_EQUAL(slab.material().X0(), 1.5f);
    BOOST_CHECK_EQUAL(slab.material().L0(), 1.5f);
    BOOST_CHECK_EQUAL(slab.material().Ar(), 3.0f);
    BOOST_CHECK_EQUAL(slab.material().Z(),
                      static_cast<float>(
                          exp((0.5 / 1.5) * log(12.0) + (1.0 / 1.5) * log(6))));
    BOOST_CHECK_EQUAL(slab.material().molarDensity(), 4.0f);
  }
  // reverse input order
  {
    auto slab = combineSlabs(slabMat0, slabMat1);
    BOOST_CHECK(slab.isValid());
    BOOST_CHECK(slab.material().isValid());
    BOOST_CHECK_EQUAL(slab.thickness(), 1.5f);
    BOOST_CHECK_EQUAL(slab.thicknessInX0(), 1.0f);
    BOOST_CHECK_EQUAL(slab.thicknessInL0(), 1.0f);
    BOOST_CHECK_EQUAL(slab.material().X0(), 1.5f);
    BOOST_CHECK_EQUAL(slab.material().L0(), 1.5f);
    BOOST_CHECK_EQUAL(slab.material().Ar(), 3.0f);
    BOOST_CHECK_EQUAL(slab.material().Z(),
                      static_cast<float>(
                          exp((0.5 / 1.5) * log(12.0) + (1.0 / 1.5) * log(6))));
    BOOST_CHECK_EQUAL(slab.material().molarDensity(), 4.0f);
  }
}

BOOST_AUTO_TEST_SUITE_END()
