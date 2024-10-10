// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Units.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Tests/CommonHelpers/PredefinedMaterials.hpp"
#include "ActsFatras/EventData/Barcode.hpp"
#include "ActsFatras/EventData/Particle.hpp"
#include "ActsFatras/Kernel/ContinuousProcess.hpp"

#include <algorithm>
#include <array>
#include <iterator>
#include <random>
#include <vector>

using namespace Acts::UnitLiterals;
using namespace ActsFatras;

namespace {

/// A mock process that leaves the particle as-is and creates four daughter
/// particles with momenta 1,2,3,4 GeV.
struct MockMakeChildren {
  template <typename generator_t>
  std::array<ActsFatras::Particle, 4> operator()(
      generator_t & /*generator*/, const Acts::MaterialSlab & /*slab*/,
      ActsFatras::Particle & /*particle*/) const {
    // create daughter particles
    return {
        Particle().setAbsoluteMomentum(1_GeV),
        Particle().setAbsoluteMomentum(2_GeV),
        Particle().setAbsoluteMomentum(3_GeV),
        Particle().setAbsoluteMomentum(4_GeV),
    };
  }
};

/// Mock particle selector that selects everything
struct MockEverything {
  bool operator()(const Particle & /*particle*/) const { return true; }
};

/// Mock particle selector for particles with momenta equal or above 10GeV.
struct MockHighP {
  double minP = 10_GeV;

  bool operator()(const ActsFatras::Particle &particle) const {
    return (minP <= particle.absoluteMomentum());
  }
};

struct Fixture {
  std::default_random_engine generator;
  Acts::MaterialSlab slab{Acts::Test::makeBeryllium(), 1_mm};
  Particle parent = Particle().setAbsoluteMomentum(10_GeV);
  std::vector<Particle> children;
};

}  // namespace

BOOST_AUTO_TEST_SUITE(FatrasContinuousProcess)

BOOST_AUTO_TEST_CASE(NoSelectors) {
  Fixture f;
  ContinuousProcess<MockMakeChildren, MockEverything, MockEverything> process{};

  // process should not abort
  BOOST_CHECK(!process(f.generator, f.slab, f.parent, f.children));
  BOOST_CHECK_EQUAL(f.children.size(), 4u);
}

BOOST_AUTO_TEST_CASE(WithInputSelector) {
  Fixture f;
  ContinuousProcess<MockMakeChildren, MockHighP, MockEverything> process;
  process.selectInputParticle.minP = 10_GeV;

  // above threshold should not abort
  f.parent.setAbsoluteMomentum(20_GeV);
  BOOST_CHECK(!process(f.generator, f.slab, f.parent, f.children));
  BOOST_CHECK_EQUAL(f.children.size(), 4u);
  // on threshold should still not abort
  f.parent.setAbsoluteMomentum(10_GeV);
  BOOST_CHECK(!process(f.generator, f.slab, f.parent, f.children));
  BOOST_CHECK_EQUAL(f.children.size(), 8u);
  // below threshold should abort and not run the process at all
  f.parent.setAbsoluteMomentum(2_GeV);
  BOOST_CHECK(!process(f.generator, f.slab, f.parent, f.children));
  // process did not run -> no new children
  BOOST_CHECK_EQUAL(f.children.size(), 8u);
}

BOOST_AUTO_TEST_CASE(WithOutputSelector) {
  Fixture f;
  // explicit child selector so it does not default to the output selector
  ContinuousProcess<MockMakeChildren, MockEverything, MockHighP, MockEverything>
      process;
  process.selectOutputParticle.minP = 10_GeV;

  // above threshold should not abort
  f.parent.setAbsoluteMomentum(20_GeV);
  BOOST_CHECK(!process(f.generator, f.slab, f.parent, f.children));
  BOOST_CHECK_EQUAL(f.children.size(), 4u);
  // on threshold should still not abort
  f.parent.setAbsoluteMomentum(10_GeV);
  BOOST_CHECK(!process(f.generator, f.slab, f.parent, f.children));
  BOOST_CHECK_EQUAL(f.children.size(), 8u);
  // below threshold should abort but only after running the process
  f.parent.setAbsoluteMomentum(2_GeV);
  BOOST_CHECK(process(f.generator, f.slab, f.parent, f.children));
  // process did still run -> new children
  BOOST_CHECK_EQUAL(f.children.size(), 12u);
}

BOOST_AUTO_TEST_CASE(WithChildSelector) {
  Fixture f;
  ContinuousProcess<MockMakeChildren, MockEverything, MockEverything, MockHighP>
      process;
  process.selectChildParticle.minP = 10_GeV;

  // all process should not abort regardless of child selection
  // select no daughters
  process.selectChildParticle.minP = 5_GeV;
  BOOST_CHECK(!process(f.generator, f.slab, f.parent, f.children));
  BOOST_CHECK_EQUAL(f.children.size(), 0u);
  // select highest daughter
  process.selectChildParticle.minP = 3.5_GeV;
  BOOST_CHECK(!process(f.generator, f.slab, f.parent, f.children));
  BOOST_CHECK_EQUAL(f.children.size(), 1u);
  // select all but the lowest daughter
  process.selectChildParticle.minP = 1.5_GeV;
  BOOST_CHECK(!process(f.generator, f.slab, f.parent, f.children));
  BOOST_CHECK_EQUAL(f.children.size(), 4u);
  // select all daughters
  process.selectChildParticle.minP = 0.5_GeV;
  BOOST_CHECK(!process(f.generator, f.slab, f.parent, f.children));
  BOOST_CHECK_EQUAL(f.children.size(), 8u);
}

BOOST_AUTO_TEST_SUITE_END()
