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
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Tests/CommonHelpers/PredefinedMaterials.hpp"
#include "ActsFatras/EventData/Particle.hpp"
#include "ActsFatras/Physics/ElectroMagnetic/BetheHeitler.hpp"

#include <array>
#include <random>
#include <utility>

#include "Dataset.hpp"

using Generator = std::ranlux48;

BOOST_DATA_TEST_CASE(
    FatrasBetheHeitler,
    Dataset::momentumPhi* Dataset::momentumTheta* Dataset::momentumAbs ^
        Dataset::rngSeed,
    phi, theta, p, seed) {
  Generator gen(seed);
  ActsFatras::Particle before =
      Dataset::makeParticle(Acts::PdgParticle::eElectron, phi, theta, p);
  ActsFatras::Particle after = before;

  ActsFatras::BetheHeitler process;
  const auto outgoing = process(gen, Acts::Test::makeUnitSlab(), after);
  // energy loss changes momentum and energy
  BOOST_CHECK_LT(after.absoluteMomentum(), before.absoluteMomentum());
  BOOST_CHECK_LT(after.energy(), before.energy());
  // energy loss creates no new particles
  BOOST_CHECK_EQUAL(outgoing.size(), 1);
  BOOST_CHECK_GT(outgoing[0].absoluteMomentum(), 0.);
  BOOST_CHECK_EQUAL(outgoing[0].pathInX0(), 0.);
  BOOST_CHECK_EQUAL(outgoing[0].pathInL0(), 0.);

  // Get the four momenta
  Acts::Vector4 p0 = before.fourMomentum();
  Acts::Vector4 p1 = after.fourMomentum();
  Acts::Vector4 k = outgoing[0].fourMomentum();

  // Test for similar invariant masses
  Acts::Vector4 sum = p1 + k;
  double s = sum(Acts::eEnergy) * sum(Acts::eEnergy) -
             sum.template segment<3>(Acts::eMom0).norm() *
                 sum.template segment<3>(Acts::eMom0).norm();
  double s0 = p0(Acts::eEnergy) * p0(Acts::eEnergy) -
              p0.template segment<3>(Acts::eMom0).norm() *
                  p0.template segment<3>(Acts::eMom0).norm();
  CHECK_CLOSE_OR_SMALL(s, s0, 1e-2, 1e-2);
}
