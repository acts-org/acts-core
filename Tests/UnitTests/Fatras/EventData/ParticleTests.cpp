// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/PdgParticle.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "ActsFatras/EventData/Barcode.hpp"
#include "ActsFatras/EventData/Particle.hpp"

#include <cmath>
#include <limits>

using Acts::PdgParticle;
using ActsFatras::Barcode;
using ActsFatras::Particle;
using namespace Acts::UnitLiterals;

namespace {
constexpr auto eps = std::numeric_limits<Particle::Scalar>::epsilon();
}

BOOST_AUTO_TEST_SUITE(FatrasParticle)

BOOST_AUTO_TEST_CASE(Construct) {
  const auto pid = Barcode().setVertexPrimary(1).setParticle(42);
  const auto particle = Particle(pid, PdgParticle::eProton, 1_e, 1_GeV);

  BOOST_CHECK_EQUAL(particle.particleId(), pid);
  BOOST_CHECK_EQUAL(particle.pdg(), PdgParticle::eProton);
  // particle is at rest at the origin
  BOOST_CHECK_EQUAL(particle.fourPosition(), Particle::Vector4::Zero());
  BOOST_CHECK_EQUAL(particle.position(), Particle::Vector3::Zero());
  BOOST_CHECK_EQUAL(particle.time(), Particle::Scalar{0});
  BOOST_CHECK_EQUAL(particle.fourPosition().x(), particle.position().x());
  BOOST_CHECK_EQUAL(particle.fourPosition().y(), particle.position().y());
  BOOST_CHECK_EQUAL(particle.fourPosition().z(), particle.position().z());
  BOOST_CHECK_EQUAL(particle.fourPosition().w(), particle.time());
  // particle direction is undefined, but must be normalized
  CHECK_CLOSE_REL(particle.direction().norm(), 1, eps);
  BOOST_CHECK_EQUAL(particle.transverseMomentum(), Particle::Scalar{0});
  BOOST_CHECK_EQUAL(particle.absoluteMomentum(), Particle::Scalar{0});
  // particle is created at rest and thus not alive
  BOOST_CHECK(!particle.isAlive());
}

BOOST_AUTO_TEST_CASE(CorrectEnergy) {
  const auto pid = Barcode().setVertexPrimary(1).setParticle(42);
  auto particle = Particle(pid, PdgParticle::eProton, 1_e, 1_GeV)
                      .setDirection(Particle::Vector3::UnitX())
                      .setAbsoluteMomentum(2_GeV);

  BOOST_CHECK_EQUAL(particle.mass(), 1_GeV);
  // check that the particle has some input energy
  BOOST_CHECK_EQUAL(particle.fourMomentum().x(), 2_GeV);
  BOOST_CHECK_EQUAL(particle.fourMomentum().y(), 0_GeV);
  BOOST_CHECK_EQUAL(particle.fourMomentum().z(), 0_GeV);
  BOOST_CHECK_EQUAL(particle.fourMomentum().w(), std::hypot(1_GeV, 2_GeV));
  BOOST_CHECK_EQUAL(particle.transverseMomentum(), 2_GeV);
  BOOST_CHECK_EQUAL(particle.absoluteMomentum(), 2_GeV);
  BOOST_CHECK_EQUAL(particle.energy(), std::hypot(1_GeV, 2_GeV));
  // particle direction must be normalized
  CHECK_CLOSE_REL(particle.direction().norm(), 1, eps);

  // loose some energy
  particle.correctEnergy(-100_MeV);
  BOOST_CHECK_LT(particle.transverseMomentum(), 2_GeV);
  BOOST_CHECK_LT(particle.absoluteMomentum(), 2_GeV);
  BOOST_CHECK_EQUAL(particle.energy(),
                    Particle::Scalar{std::hypot(1_GeV, 2_GeV) - 100_MeV});
  CHECK_CLOSE_REL(particle.direction().norm(), 1, eps);
  // particle is still alive
  BOOST_CHECK(particle.isAlive());

  // loose some more energy
  particle.correctEnergy(-200_MeV);
  BOOST_CHECK_LT(particle.transverseMomentum(), 2_GeV);
  BOOST_CHECK_LT(particle.absoluteMomentum(), 2_GeV);
  BOOST_CHECK_EQUAL(particle.energy(),
                    Particle::Scalar{std::hypot(1_GeV, 2_GeV) - 300_MeV});
  CHECK_CLOSE_REL(particle.direction().norm(), 1, eps);
  // particle is still alive
  BOOST_CHECK(particle.isAlive());

  // loose a lot of energy
  particle.correctEnergy(-3_GeV);
  BOOST_CHECK_EQUAL(particle.transverseMomentum(), Particle::Scalar{0});
  BOOST_CHECK_EQUAL(particle.absoluteMomentum(), Particle::Scalar{0});
  BOOST_CHECK_EQUAL(particle.energy(), particle.mass());
  CHECK_CLOSE_REL(particle.direction().norm(), 1, eps);
  // particle is not alive anymore
  BOOST_CHECK(!particle.isAlive());

  // lossing even more energy does nothing
  particle.correctEnergy(-10_GeV);
  BOOST_CHECK_EQUAL(particle.transverseMomentum(), Particle::Scalar{0});
  BOOST_CHECK_EQUAL(particle.absoluteMomentum(), Particle::Scalar{0});
  BOOST_CHECK_EQUAL(particle.energy(), particle.mass());
  CHECK_CLOSE_REL(particle.direction().norm(), 1, eps);
  // particle is still not alive
  BOOST_CHECK(!particle.isAlive());
}

BOOST_AUTO_TEST_SUITE_END()
