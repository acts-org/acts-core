// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/PdgParticle.hpp"
#include "Acts/Material/Interactions.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Tests/CommonHelpers/PredefinedMaterials.hpp"
#include "ActsFatras/EventData/Particle.hpp"
#include "ActsFatras/Physics/ElectroMagnetic/Scattering.hpp"

#include <cmath>
#include <cstdint>
#include <limits>
#include <random>

#include "Dataset.hpp"

namespace {

constexpr auto eps = std::numeric_limits<double>::epsilon();

double rms(const std::vector<double>& values, double mean) {
  return std::sqrt(std::accumulate(values.begin(), values.end(), 0.0,
                                   [mean](double sum, double value) {
                                     return sum +
                                            (value - mean) * (value - mean);
                                   }) /
                   values.size());
}

// Common test method that will be instantiated for each scattering model.
template <typename Scattering>
void test(const Scattering& scattering, std::uint32_t seed,
          const ActsFatras::Particle& before) {
  std::ranlux48 gen(seed);
  ActsFatras::Particle after = before;

  const auto outgoing = scattering(gen, Acts::Test::makePercentSlab(), after);
  // scattering leaves absolute energy/momentum unchanged
  CHECK_CLOSE_REL(after.absoluteMomentum(), before.absoluteMomentum(), eps);
  CHECK_CLOSE_REL(after.energy(), before.energy(), eps);
  // scattering has changed the direction
  BOOST_CHECK_LT(before.direction().dot(after.direction()), 1);
  // scattering creates no new particles
  BOOST_CHECK(outgoing.empty());
}
}  // namespace

BOOST_AUTO_TEST_SUITE(FatrasScattering)

BOOST_DATA_TEST_CASE(GeneralMixture, Dataset::parameters, pdg, phi, theta, p,
                     seed) {
  test(ActsFatras::GeneralMixtureScattering(), seed,
       Dataset::makeParticle(pdg, phi, theta, p));
}

BOOST_DATA_TEST_CASE(GaussianMixture, Dataset::parameters, pdg, phi, theta, p,
                     seed) {
  test(ActsFatras::GaussianMixtureScattering(), seed,
       Dataset::makeParticle(pdg, phi, theta, p));
}

BOOST_DATA_TEST_CASE(Highland, Dataset::parameters, pdg, phi, theta, p, seed) {
  test(ActsFatras::HighlandScattering(), seed,
       Dataset::makeParticle(pdg, phi, theta, p));
}

BOOST_AUTO_TEST_CASE(HighlandRms) {
  auto scattering = ActsFatras::HighlandScattering();
  auto particle = Dataset::makeParticle(Acts::PdgParticle::eMuon, 0, 0, 1);
  auto materialSlab = Acts::Test::makePercentSlab();

  auto theta0 = Acts::computeMultipleScatteringTheta0(
      materialSlab, particle.absolutePdg(), particle.mass(), particle.qOverP(),
      particle.absoluteCharge());

  std::ranlux48 gen(0);

  std::vector<double> thetaYZs;
  std::vector<double> theta3Ds;

  for (std::size_t i = 0; i < 10000; i++) {
    auto newParticle = particle;
    scattering(gen, materialSlab, newParticle);

    double thetaYZ =
        std::atan2(newParticle.direction().y(), newParticle.direction().z());
    double theta3d =
        std::acos(newParticle.direction().dot(particle.direction()));

    thetaYZs.push_back(thetaYZ);
    theta3Ds.push_back(theta3d);
  }

  double rmsThetaYZ = rms(thetaYZs, 0);
  double rmsTheta3D = rms(theta3Ds, 0);

  CHECK_CLOSE_REL(rmsThetaYZ, theta0, 0.02);
  CHECK_CLOSE_REL(rmsTheta3D, M_SQRT2 * theta0, 0.02);
}

BOOST_AUTO_TEST_SUITE_END()
