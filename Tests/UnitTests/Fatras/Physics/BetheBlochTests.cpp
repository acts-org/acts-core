// This file is part of the Acts project.
//
// Copyright (C) 2018-2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Tests/CommonHelpers/PredefinedMaterials.hpp"
#include "ActsFatras/Physics/ElectroMagnetic/BetheBloch.hpp"

#include <random>
#include <fstream>

#include "Dataset.hpp"

using Generator = std::ranlux48;

BOOST_DATA_TEST_CASE(FatrasBetheBloch, Dataset::parameters, pdg, phi, lambda, p,
                     seed) {
  Generator gen(seed);
  ActsFatras::Particle before = Dataset::makeParticle(pdg, phi, lambda, p);
  ActsFatras::Particle after = before;

  ActsFatras::BetheBloch process;
  const auto outgoing = process(gen, Acts::Test::makeUnitSlab(), after);
  // energy loss changes momentum and energy
  BOOST_CHECK_LT(after.absoluteMomentum(), before.absoluteMomentum());
  BOOST_CHECK_LT(after.energy(), before.energy());
  // energy loss creates no new particles
  BOOST_CHECK(outgoing.empty());
}

BOOST_AUTO_TEST_CASE(FatrasBetheBlochDist){

  Generator gen(42u);
  ActsFatras::Particle before = Dataset::makeParticle(Acts::PdgParticle::eMuon, 0, 0, 10.);

  unsigned int nD = 100000u;

  std::ofstream dEout;
  dEout.open("FatrasUnitTest_BetheBloch.csv");
  dEout << "dE" << '\n';

  ActsFatras::BetheBloch process;

  auto materialSlab = Acts::Test::makeUnitSlab();
  for (unsigned int ip = 0; ip < nD; ++ip){
    ActsFatras::Particle after = before;
    const auto outgoing = process(gen, materialSlab, after);
    dEout << before.energy()-after.energy() <<'\n';
    BOOST_CHECK(outgoing.empty());
  }
  dEout.close();
}

