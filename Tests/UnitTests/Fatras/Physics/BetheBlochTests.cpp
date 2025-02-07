// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Tests/CommonHelpers/PredefinedMaterials.hpp"
#include "ActsFatras/EventData/Particle.hpp"
#include "ActsFatras/Physics/ElectroMagnetic/BetheBloch.hpp"

#include <array>
#include <random>

#include "Dataset.hpp"

using Generator = std::ranlux48;

BOOST_DATA_TEST_CASE(FatrasBetheBloch, Dataset::parameters, pdg, phi, theta, p,
                     seed) {
  Generator gen(seed);
  ActsFatras::Particle before = Dataset::makeParticle(pdg, phi, theta, p);
  ActsFatras::Particle after = before;

  ActsFatras::BetheBloch process;
  const auto outgoing = process(gen, Acts::Test::makeUnitSlab(), after);
  // energy loss changes momentum and energy
  BOOST_CHECK_LT(after.absoluteMomentum(), before.absoluteMomentum());
  BOOST_CHECK_LT(after.energy(), before.energy());
  // energy loss creates no new particles
  BOOST_CHECK(outgoing.empty());
}
