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
#include "ActsFatras/Kernel/InteractionList.hpp"

#include <cstdint>
#include <limits>
#include <random>
#include <utility>
#include <vector>

using namespace Acts::UnitLiterals;
using namespace ActsFatras;

using Acts::MaterialSlab;
using Scalar = Particle::Scalar;

namespace {

/// Continuous process that does not trigger a break
struct SterileContinuousProcess {
  template <typename generator_t>
  bool operator()(generator_t & /*generator*/, const MaterialSlab & /*slab*/,
                  Particle & /*particle*/,
                  std::vector<Particle> & /*generated*/) const {
    return false;
  }
};

static_assert(detail::ContinuousProcessConcept<SterileContinuousProcess>,
              "Is not a continuous process");
static_assert(!detail::PointLikeProcessConcept<SterileContinuousProcess>,
              "Is a point-like process");

/// Continuous process that DOES trigger a break
struct FatalContinuousProcess {
  template <typename generator_t>
  bool operator()(generator_t & /*generator*/, const MaterialSlab & /*slab*/,
                  Particle & /*particle*/,
                  std::vector<Particle> & /*generated*/) const {
    return true;
  }
};
static_assert(detail::ContinuousProcessConcept<FatalContinuousProcess>,
              "Is not a continuous process");
static_assert(!detail::PointLikeProcessConcept<FatalContinuousProcess>,
              "Is a point-like process");

/// EM-like point-like process that triggers on X0 and keeps the particle alive.
///
/// Each run call creates one descendant particle.
struct X0PointLikeProcess {
  template <typename generator_t>
  std::pair<Scalar, Scalar> generatePathLimits(
      generator_t & /*generator*/, const Particle & /*particle*/) const {
    return {0.5, std::numeric_limits<Scalar>::infinity()};
  }

  template <typename generator_t>
  bool run(generator_t & /*generator*/, Particle &particle,
           std::vector<Particle> &generated) const {
    auto pid0 = particle.particleId().makeDescendant(0);
    generated.emplace_back(particle.withParticleId(pid0));
    return false;
  }
};

static_assert(!detail::ContinuousProcessConcept<X0PointLikeProcess>,
              "Is a continuous process");
static_assert(detail::PointLikeProcessConcept<X0PointLikeProcess>,
              "Is not a point-like process");

/// Nuclear-like point-like process that triggers on L0 and kills the particle.
///
/// Each run call creates two descendant particles.
struct L0PointLikeProcess {
  template <typename generator_t>
  std::pair<Scalar, Scalar> generatePathLimits(
      generator_t & /*generator*/, const Particle & /*particle*/) const {
    return {std::numeric_limits<Scalar>::infinity(), 1.5};
  }

  template <typename generator_t>
  bool run(generator_t & /*generator*/, Particle &particle,
           std::vector<Particle> &generated) const {
    auto pid0 = particle.particleId().makeDescendant(0);
    auto pid1 = particle.particleId().makeDescendant(1);
    generated.emplace_back(particle.withParticleId(pid0));
    generated.emplace_back(particle.withParticleId(pid1));
    return true;
  }
};

static_assert(!detail::ContinuousProcessConcept<L0PointLikeProcess>,
              "Is a continuous process");
static_assert(detail::PointLikeProcessConcept<L0PointLikeProcess>,
              "Is not a point-like process");

struct Fixture {
  std::ranlux48 rng{23};
  Acts::MaterialSlab slab =
      Acts::MaterialSlab(Acts::Test::makeBeryllium(), 1_mm);
  Particle incoming;
  std::vector<Particle> outgoing;
};

}  // namespace

BOOST_AUTO_TEST_SUITE(FatrasInteractionList)

BOOST_AUTO_TEST_CASE(Empty) {
  Fixture f;
  InteractionList<> l;

  // w/o processes the list should never abort
  BOOST_CHECK(!l.runContinuous(f.rng, f.slab, f.incoming, f.outgoing));

  // w/o processes there should be no selection
  auto sel = l.armPointLike(f.rng, f.incoming);
  BOOST_CHECK_EQUAL(sel.x0Limit, std::numeric_limits<Scalar>::infinity());
  BOOST_CHECK_EQUAL(sel.l0Limit, std::numeric_limits<Scalar>::infinity());
  BOOST_CHECK_EQUAL(sel.x0Process, std::numeric_limits<std::size_t>::max());
  BOOST_CHECK_EQUAL(sel.l0Process, std::numeric_limits<std::size_t>::max());

  // running with an invalid process index should do nothing
  // interaction list is empty and 0 should already be invalid
  BOOST_CHECK(!l.runPointLike(f.rng, 0u, f.incoming, f.outgoing));
  BOOST_CHECK_EQUAL(f.outgoing.size(), 0u);
  // std::numeric_limits<std::size_t>::max() should always be an invalid index
  BOOST_CHECK(!l.runPointLike(f.rng, std::numeric_limits<std::size_t>::max(),
                              f.incoming, f.outgoing));
  BOOST_CHECK_EQUAL(f.outgoing.size(), 0u);
}

BOOST_AUTO_TEST_CASE(ContinuousSterile) {
  Fixture f;
  InteractionList<SterileContinuousProcess> l;

  // sterile process should never abort
  BOOST_CHECK(!l.runContinuous(f.rng, f.slab, f.incoming, f.outgoing));
}

BOOST_AUTO_TEST_CASE(ContinuousFatal) {
  Fixture f;
  InteractionList<FatalContinuousProcess> l;

  // fatal process must always abort
  BOOST_CHECK(l.runContinuous(f.rng, f.slab, f.incoming, f.outgoing));
}

BOOST_AUTO_TEST_CASE(ContinuousSterileFatal) {
  Fixture f;
  InteractionList<SterileContinuousProcess, FatalContinuousProcess> physicsList;

  // the contained fatal process must always abort
  BOOST_CHECK(physicsList.runContinuous(f.rng, f.slab, f.incoming, f.outgoing));
  // with the fatal process disabled, it should go through again
  physicsList.disable<FatalContinuousProcess>();
  BOOST_CHECK(
      !physicsList.runContinuous(f.rng, f.slab, f.incoming, f.outgoing));
}

BOOST_AUTO_TEST_CASE(PointLikeX0) {
  Fixture f;
  InteractionList<X0PointLikeProcess> l;

  // w/o processes the list should never abort
  auto sel = l.armPointLike(f.rng, f.incoming);
  BOOST_CHECK_EQUAL(sel.x0Limit, 0.5);
  BOOST_CHECK_EQUAL(sel.l0Limit, std::numeric_limits<Scalar>::infinity());
  BOOST_CHECK_EQUAL(sel.x0Process, 0u);
  BOOST_CHECK_EQUAL(sel.l0Process, std::numeric_limits<std::size_t>::max());

  // valid index, X0Process leaves the particle alive
  BOOST_CHECK(!l.runPointLike(f.rng, 0u, f.incoming, f.outgoing));
  BOOST_CHECK_EQUAL(f.outgoing.size(), 1u);
  // invalid index, should do nothing
  BOOST_CHECK(!l.runPointLike(f.rng, std::numeric_limits<std::size_t>::max(),
                              f.incoming, f.outgoing));
  BOOST_CHECK_EQUAL(f.outgoing.size(), 1u);
}

BOOST_AUTO_TEST_CASE(PointLikeL0) {
  Fixture f;
  InteractionList<L0PointLikeProcess> l;

  // w/o processes the list should never abort
  auto sel = l.armPointLike(f.rng, f.incoming);
  BOOST_CHECK_EQUAL(sel.x0Limit, std::numeric_limits<Scalar>::infinity());
  BOOST_CHECK_EQUAL(sel.l0Limit, 1.5);
  BOOST_CHECK_EQUAL(sel.x0Process, std::numeric_limits<std::size_t>::max());
  BOOST_CHECK_EQUAL(sel.l0Process, 0u);

  // valid index, L0Process kills the particles and creates 2 descendants
  BOOST_CHECK(l.runPointLike(f.rng, 0u, f.incoming, f.outgoing));
  BOOST_CHECK_EQUAL(f.outgoing.size(), 2u);
  // invalid index, should do nothing
  BOOST_CHECK(!l.runPointLike(f.rng, std::numeric_limits<std::size_t>::max(),
                              f.incoming, f.outgoing));
  BOOST_CHECK_EQUAL(f.outgoing.size(), 2u);
}

BOOST_AUTO_TEST_CASE(PointLikeX0L0) {
  Fixture f;
  InteractionList<X0PointLikeProcess, L0PointLikeProcess> l;

  // w/o processes the list should never abort
  auto sel = l.armPointLike(f.rng, f.incoming);
  BOOST_CHECK_EQUAL(sel.x0Limit, 0.5);
  BOOST_CHECK_EQUAL(sel.l0Limit, 1.5);
  BOOST_CHECK_EQUAL(sel.x0Process, 0u);
  BOOST_CHECK_EQUAL(sel.l0Process, 1u);

  // valid index, X0Process leaves the particle alive
  BOOST_CHECK(!l.runPointLike(f.rng, 0u, f.incoming, f.outgoing));
  BOOST_CHECK_EQUAL(f.outgoing.size(), 1u);
  // valid index, L0Process kills the particles and creates 2 descendants
  BOOST_CHECK(l.runPointLike(f.rng, 1u, f.incoming, f.outgoing));
  BOOST_CHECK_EQUAL(f.outgoing.size(), 3u);
  // invalid index, should do nothing
  BOOST_CHECK(!l.runPointLike(f.rng, std::numeric_limits<std::size_t>::max(),
                              f.incoming, f.outgoing));
  BOOST_CHECK_EQUAL(f.outgoing.size(), 3u);
}

// this tests both the disable functionality and an interaction list with both
// continuous and point-like processes.
BOOST_AUTO_TEST_CASE(Disable) {
  Fixture f;
  InteractionList<SterileContinuousProcess, FatalContinuousProcess,
                  X0PointLikeProcess, L0PointLikeProcess>
      l;

  // continuous should abort due to the fatal process
  BOOST_CHECK(l.runContinuous(f.rng, f.slab, f.incoming, f.outgoing));
  // unless we disable it
  l.disable<FatalContinuousProcess>();
  BOOST_CHECK(!l.runContinuous(f.rng, f.slab, f.incoming, f.outgoing));

  // disabled X0Process should not participate in arming procedure
  l.disable<X0PointLikeProcess>();
  {
    auto sel = l.armPointLike(f.rng, f.incoming);
    BOOST_CHECK_EQUAL(sel.x0Limit, std::numeric_limits<Scalar>::infinity());
    BOOST_CHECK_EQUAL(sel.l0Limit, 1.5);
    BOOST_CHECK_EQUAL(sel.x0Process, std::numeric_limits<std::size_t>::max());
    BOOST_CHECK_EQUAL(sel.l0Process, 3u);

    // index for X0Process, should do nothing since its disabled
    f.outgoing.clear();
    BOOST_CHECK(!l.runPointLike(f.rng, 2u, f.incoming, f.outgoing));
    BOOST_CHECK_EQUAL(f.outgoing.size(), 0u);
    // index for L0Process, should run and generate a break condition
    BOOST_CHECK(l.runPointLike(f.rng, 3u, f.incoming, f.outgoing));
    BOOST_CHECK_EQUAL(f.outgoing.size(), 2u);
  }

  // disabling L0Process is equivalent to an empty list (for arming)
  l.disable<L0PointLikeProcess>();
  {
    auto sel = l.armPointLike(f.rng, f.incoming);
    BOOST_CHECK_EQUAL(sel.x0Limit, std::numeric_limits<Scalar>::infinity());
    BOOST_CHECK_EQUAL(sel.l0Limit, std::numeric_limits<Scalar>::infinity());
    BOOST_CHECK_EQUAL(sel.x0Process, std::numeric_limits<std::size_t>::max());
    BOOST_CHECK_EQUAL(sel.l0Process, std::numeric_limits<std::size_t>::max());

    // index for X0Process, should do nothing since its disabled
    f.outgoing.clear();
    BOOST_CHECK(!l.runPointLike(f.rng, 2u, f.incoming, f.outgoing));
    BOOST_CHECK_EQUAL(f.outgoing.size(), 0u);
    // index for L0Process, should do nothing since its disabled
    BOOST_CHECK(!l.runPointLike(f.rng, 3u, f.incoming, f.outgoing));
    BOOST_CHECK_EQUAL(f.outgoing.size(), 0u);
  }

  // invalid index, should do nothing
  f.outgoing.clear();
  BOOST_CHECK(!l.runPointLike(f.rng, std::numeric_limits<std::size_t>::max(),
                              f.incoming, f.outgoing));
  BOOST_CHECK_EQUAL(f.outgoing.size(), 0u);
}

BOOST_AUTO_TEST_SUITE_END()
