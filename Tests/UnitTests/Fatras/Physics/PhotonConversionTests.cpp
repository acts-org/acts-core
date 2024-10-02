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
#include "Acts/Definitions/Common.hpp"
#include "Acts/Definitions/PdgParticle.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "ActsFatras/EventData/Particle.hpp"
#include "ActsFatras/Physics/ElectroMagnetic/PhotonConversion.hpp"

#include <cmath>
#include <limits>
#include <memory>
#include <random>
#include <utility>
#include <vector>

#include "Dataset.hpp"

using Generator = std::ranlux48;

BOOST_AUTO_TEST_SUITE(FatrasPhotonConversion)

BOOST_DATA_TEST_CASE(NoPhoton, Dataset::parametersPhotonConversion, phi, theta,
                     seed) {
  using Scalar = ActsFatras::PhotonConversion::Scalar;
  using namespace Acts::UnitLiterals;

  Generator gen(seed);

  /// Produce not a photon
  ActsFatras::Particle particle =
      Dataset::makeParticle(Acts::PdgParticle::eElectron, phi, theta, 1_GeV);
  ActsFatras::Particle particleInit = particle;

  ActsFatras::PhotonConversion pc;

  // No limits should be set
  std::pair<Scalar, Scalar> limits;
  limits = pc.generatePathLimits(gen, particle);
  BOOST_CHECK_EQUAL(limits.first, std::numeric_limits<Scalar>::infinity());
  BOOST_CHECK_EQUAL(limits.second, std::numeric_limits<Scalar>::infinity());

  // No particles should be generated
  std::vector<ActsFatras::Particle> generated;
  BOOST_CHECK(!pc.run(gen, particle, generated));
  BOOST_CHECK(generated.empty());
  // Particle shouldn't be modified
  BOOST_CHECK_EQUAL(particle.fourPosition(), particleInit.fourPosition());
  BOOST_CHECK_EQUAL(particle.fourMomentum(), particleInit.fourMomentum());
  BOOST_CHECK_EQUAL(particle.process(), particleInit.process());
  BOOST_CHECK_EQUAL(particle.properTime(), particleInit.properTime());
  BOOST_CHECK_EQUAL(particle.pathInX0(), particleInit.pathInX0());
  BOOST_CHECK_EQUAL(particle.pathInL0(), particleInit.pathInL0());
}

BOOST_DATA_TEST_CASE(DeadPhoton, Dataset::parametersPhotonConversion, phi,
                     theta, seed) {
  using Scalar = ActsFatras::PhotonConversion::Scalar;
  using namespace Acts::UnitLiterals;

  Generator gen(seed);

  /// Produce a dead photon
  ActsFatras::Particle particle =
      Dataset::makeParticle(Acts::PdgParticle::eGamma, phi, theta, 0);
  ActsFatras::Particle particleInit = particle;

  ActsFatras::PhotonConversion pc;

  // No limits should be set - momentum too low
  std::pair<Scalar, Scalar> limits = pc.generatePathLimits(gen, particle);
  BOOST_CHECK_EQUAL(limits.first, std::numeric_limits<Scalar>::infinity());
  BOOST_CHECK_EQUAL(limits.second, std::numeric_limits<Scalar>::infinity());

  // No particles should be generated - momentum too low
  std::vector<ActsFatras::Particle> generated;
  BOOST_CHECK(!pc.run(gen, particle, generated));
  BOOST_CHECK(generated.empty());
  // Particle shouldn't be modified
  BOOST_CHECK_EQUAL(particle.fourPosition(), particleInit.fourPosition());
  BOOST_CHECK_EQUAL(particle.fourMomentum(), particleInit.fourMomentum());
  BOOST_CHECK_EQUAL(particle.process(), particleInit.process());
  BOOST_CHECK_EQUAL(particle.properTime(), particleInit.properTime());
  BOOST_CHECK_EQUAL(particle.pathInX0(), particleInit.pathInX0());
  BOOST_CHECK_EQUAL(particle.pathInL0(), particleInit.pathInL0());
}

BOOST_DATA_TEST_CASE(LowMomentumPhoton, Dataset::parametersPhotonConversion,
                     phi, theta, seed) {
  using Scalar = ActsFatras::PhotonConversion::Scalar;
  using namespace Acts::UnitLiterals;

  Generator gen(seed);

  /// Produce a low momentum photon
  ActsFatras::Particle particle =
      Dataset::makeParticle(Acts::PdgParticle::eGamma, phi, theta, 1_keV);
  ActsFatras::Particle particleInit = particle;

  ActsFatras::PhotonConversion pc;

  // No limits should be set - momentum too low
  std::pair<Scalar, Scalar> limits = pc.generatePathLimits(gen, particle);
  BOOST_CHECK_EQUAL(limits.first, std::numeric_limits<Scalar>::infinity());
  BOOST_CHECK_EQUAL(limits.second, std::numeric_limits<Scalar>::infinity());

  // No particles should be generated - momentum too low
  std::vector<ActsFatras::Particle> generated;
  BOOST_CHECK(!pc.run(gen, particle, generated));
  BOOST_CHECK(generated.empty());
  // Particle shouldn't be modified
  BOOST_CHECK_EQUAL(particle.fourPosition(), particleInit.fourPosition());
  BOOST_CHECK_EQUAL(particle.fourMomentum(), particleInit.fourMomentum());
  BOOST_CHECK_EQUAL(particle.process(), particleInit.process());
  BOOST_CHECK_EQUAL(particle.properTime(), particleInit.properTime());
  BOOST_CHECK_EQUAL(particle.pathInX0(), particleInit.pathInX0());
  BOOST_CHECK_EQUAL(particle.pathInL0(), particleInit.pathInL0());
}

BOOST_DATA_TEST_CASE(HighMomentumPhoton, Dataset::parametersPhotonConversion,
                     phi, theta, seed) {
  using Scalar = ActsFatras::PhotonConversion::Scalar;
  using namespace Acts::UnitLiterals;

  Generator gen(seed);

  /// Produce a high momentum photon
  ActsFatras::Particle particle =
      Dataset::makeParticle(Acts::PdgParticle::eGamma, phi, theta, 1_GeV);
  ActsFatras::Particle particleInit = particle;

  ActsFatras::PhotonConversion pc;

  // No limits should be set - momentum too low
  std::pair<Scalar, Scalar> limits = pc.generatePathLimits(gen, particle);
  BOOST_CHECK_NE(limits.first, std::numeric_limits<Scalar>::infinity());
  BOOST_CHECK_EQUAL(limits.second, std::numeric_limits<Scalar>::infinity());

  // No particles should be generated - momentum too low
  std::vector<ActsFatras::Particle> generated;
  BOOST_CHECK(pc.run(gen, particle, generated));
  BOOST_CHECK_EQUAL(generated.size(), 2);

  // Test the children
  BOOST_CHECK((generated[0].pdg() == Acts::PdgParticle::eElectron) ||
              (generated[0].pdg() == Acts::PdgParticle::ePositron));
  BOOST_CHECK((generated[1].pdg() == Acts::PdgParticle::eElectron) ||
              (generated[1].pdg() == Acts::PdgParticle::ePositron));
  BOOST_CHECK_NE(generated[0].pdg(), generated[1].pdg());
  BOOST_CHECK_NE(generated[0].fourMomentum(), Acts::Vector4::Zero());
  BOOST_CHECK_NE(generated[1].fourMomentum(), Acts::Vector4::Zero());

  // Test for similar invariant masses
  Acts::Vector4 momSum =
      generated[0].fourMomentum() + generated[1].fourMomentum();
  Acts::Vector3 momVector = momSum.template segment<3>(Acts::eMom0);
  double sSum = momSum[Acts::eEnergy] * momSum[Acts::eEnergy] -
                momVector.norm() * momVector.norm();
  BOOST_CHECK(std::isfinite(sSum));

  double sParticle =
      particleInit.energy() * particleInit.energy() -
      particleInit.absoluteMomentum() * particleInit.absoluteMomentum();
  BOOST_CHECK(std::isfinite(sParticle));
  CHECK_CLOSE_OR_SMALL(sSum, sParticle, 1e-2, 1e-2);
}

BOOST_AUTO_TEST_SUITE_END()
