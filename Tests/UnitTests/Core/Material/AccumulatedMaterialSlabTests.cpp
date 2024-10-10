// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Material/AccumulatedMaterialSlab.hpp"
#include "Acts/Material/Material.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Tests/CommonHelpers/PredefinedMaterials.hpp"

#include <limits>
#include <utility>

namespace {

using Acts::AccumulatedMaterialSlab;
using Acts::Material;
using Acts::MaterialSlab;
using Acts::Test::makeSilicon;
using Acts::Test::makeUnitSlab;

constexpr auto eps = std::numeric_limits<float>::epsilon();

}  // namespace

BOOST_AUTO_TEST_SUITE(MaterialAccumulatedMaterialSlab)

BOOST_AUTO_TEST_CASE(Nothing) {
  AccumulatedMaterialSlab a;
  auto [average, trackCount] = a.totalAverage();
  // material is vacuum
  BOOST_CHECK(!average.isValid());
  BOOST_CHECK_EQUAL(trackCount, 0u);
}

// average three empty tracks which are ignored by default
BOOST_AUTO_TEST_CASE(EmptyTracksIgnored) {
  AccumulatedMaterialSlab a;
  a.trackAverage();
  a.trackAverage();
  a.trackAverage();
  auto [average, trackCount] = a.totalAverage();
  BOOST_CHECK(!average.isValid());
  BOOST_CHECK_EQUAL(trackCount, 0u);
}

// average three empty tracks and do not ignore them
BOOST_AUTO_TEST_CASE(EmptyTracks) {
  AccumulatedMaterialSlab a;
  a.trackAverage(true);
  a.trackAverage(true);
  a.trackAverage(true);
  auto [average, trackCount] = a.totalAverage();
  BOOST_CHECK(!average.isValid());
  BOOST_CHECK_EQUAL(trackCount, 3u);
}

BOOST_AUTO_TEST_CASE(MultipleIdenticalThicknessTrackSteps) {
  MaterialSlab unit = makeUnitSlab();
  AccumulatedMaterialSlab a;
  // accumulate three identical steps for one track
  {
    a.accumulate(unit);
    a.accumulate(unit);
    a.accumulate(unit);
    a.trackAverage();
    auto [average, trackCount] = a.totalAverage();
    BOOST_CHECK_EQUAL(trackCount, 1u);
    BOOST_CHECK_EQUAL(average.material(), unit.material());
    BOOST_CHECK_EQUAL(average.thickness(), 3 * unit.thickness());
    BOOST_CHECK_EQUAL(average.thicknessInX0(), 3.0f);
    BOOST_CHECK_EQUAL(average.thicknessInL0(), 3.0f);
  }
  // accumulate three identical steps for one additional track
  {
    a.accumulate(unit);
    a.accumulate(unit);
    a.accumulate(unit);
    a.trackAverage();
    auto [average, trackCount] = a.totalAverage();
    BOOST_CHECK_EQUAL(trackCount, 2u);
    // averages must stay the same since we added the same material again
    BOOST_CHECK_EQUAL(average.material(), unit.material());
    BOOST_CHECK_EQUAL(average.thickness(), 3 * unit.thickness());
    BOOST_CHECK_EQUAL(average.thicknessInX0(), 3.0f);
    BOOST_CHECK_EQUAL(average.thicknessInL0(), 3.0f);
  }
}

// accumulate and average three tracks.
// each track contributes the same material but each in different steps.
BOOST_AUTO_TEST_CASE(MultipleDifferentThicknessTrackSteps) {
  MaterialSlab unit = makeUnitSlab();
  AccumulatedMaterialSlab a;
  // accumulate three identical steps
  {
    a.accumulate(unit);
    a.accumulate(unit);
    a.accumulate(unit);
    a.trackAverage();
    auto [average, trackCount] = a.totalAverage();
    BOOST_CHECK_EQUAL(trackCount, 1u);
    BOOST_CHECK_EQUAL(average.material(), unit.material());
    BOOST_CHECK_EQUAL(average.thickness(), 3 * unit.thickness());
    BOOST_CHECK_EQUAL(average.thicknessInX0(), 3.0f);
    BOOST_CHECK_EQUAL(average.thicknessInL0(), 3.0f);
  }
  // accumulate one step with thickness 1, one with thickness 2
  {
    MaterialSlab twice = unit;
    twice.scaleThickness(2);
    a.accumulate(unit);
    a.accumulate(twice);
    a.trackAverage();
    auto [average, trackCount] = a.totalAverage();
    BOOST_CHECK_EQUAL(trackCount, 2u);
    // averages must stay the same
    BOOST_CHECK_EQUAL(average.material(), unit.material());
    BOOST_CHECK_EQUAL(average.thickness(), 3 * unit.thickness());
    BOOST_CHECK_EQUAL(average.thicknessInX0(), 3.0f);
    BOOST_CHECK_EQUAL(average.thicknessInL0(), 3.0f);
  }
  // accumulate one step with thickness 3
  {
    MaterialSlab thrice = unit;
    thrice.scaleThickness(3);
    a.accumulate(thrice);
    a.trackAverage();
    auto [average, trackCount] = a.totalAverage();
    BOOST_CHECK_EQUAL(trackCount, 3u);
    // averages must stay the same
    BOOST_CHECK_EQUAL(average.material(), unit.material());
    BOOST_CHECK_EQUAL(average.thickness(), 3 * unit.thickness());
    BOOST_CHECK_EQUAL(average.thicknessInX0(), 3.0f);
    BOOST_CHECK_EQUAL(average.thicknessInL0(), 3.0f);
  }
}

// average multiple tracks w/ one step each but different materials
BOOST_AUTO_TEST_CASE(MultipleDifferentTracks) {
  MaterialSlab unit = makeUnitSlab();
  AccumulatedMaterialSlab a;
  // add material w/ given thickness
  {
    a.accumulate(unit);
    a.trackAverage();
    auto [average, trackCount] = a.totalAverage();
    BOOST_CHECK_EQUAL(trackCount, 1u);
    BOOST_CHECK_EQUAL(average.material(), unit.material());
    BOOST_CHECK_EQUAL(average.thickness(), unit.thickness());
    BOOST_CHECK_EQUAL(average.thicknessInX0(), unit.thicknessInX0());
    BOOST_CHECK_EQUAL(average.thicknessInL0(), unit.thicknessInL0());
  }
  // add material w/ given three times the initial thickness
  {
    MaterialSlab three = unit;
    three.scaleThickness(3);
    a.accumulate(three);
    a.trackAverage();
    auto [average, trackCount] = a.totalAverage();
    BOOST_CHECK_EQUAL(trackCount, 2);
    // average thickness should now be 2*initial, average material unchanged
    BOOST_CHECK_EQUAL(average.material(), unit.material());
    BOOST_CHECK_EQUAL(average.thickness(), 2 * unit.thickness());
    BOOST_CHECK_EQUAL(average.thicknessInX0(), 2 * unit.thicknessInX0());
    BOOST_CHECK_EQUAL(average.thicknessInL0(), 2 * unit.thicknessInL0());
  }
  // add vacuum w/ given the same thickness as the current average
  {
    MaterialSlab vac(2 * unit.thickness());
    // add vacuum twice to counteract the existing two tracks stored
    a.accumulate(vac);
    a.trackAverage();
    a.accumulate(vac);
    a.trackAverage();
    auto [average, trackCount] = a.totalAverage();
    BOOST_CHECK_EQUAL(trackCount, 4u);
    // average material density halved
    CHECK_CLOSE_REL(average.material().X0(), 2 * unit.material().X0(), eps);
    CHECK_CLOSE_REL(average.material().L0(), 2 * unit.material().L0(), eps);
    CHECK_CLOSE_REL(average.material().molarDensity(),
                    0.5f * unit.material().molarDensity(), eps);
    // average atom is still the same species
    CHECK_CLOSE_REL(average.material().Ar(), unit.material().Ar(), eps);
    // average atomic number proportional to the thickness
    CHECK_CLOSE_REL(average.material().Z(), 0.5 * unit.material().Z(), eps);
    // thickness in x0/l0 depends on density and thus halved as well
    BOOST_CHECK_EQUAL(average.thicknessInX0(), 1 * unit.thicknessInX0());
    BOOST_CHECK_EQUAL(average.thicknessInL0(), 1 * unit.thicknessInL0());
    // average real thickness stays the same
    BOOST_CHECK_EQUAL(average.thickness(), 2 * unit.thickness());
  }
}

BOOST_AUTO_TEST_SUITE_END()
