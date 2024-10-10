// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/ParticleData.hpp"
#include "Acts/Definitions/PdgParticle.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"

#include <cmath>
#include <string_view>

using namespace Acts;
using namespace Acts::UnitLiterals;

namespace {
// NOTE: the used mass comparison values are not as exact as the data values
static constexpr float eps = 0.001f;
}  // namespace

BOOST_AUTO_TEST_SUITE(FatrasParticleData)

BOOST_AUTO_TEST_CASE(InvalidInput) {
  BOOST_CHECK(!findCharge(PdgParticle::eInvalid));
  BOOST_CHECK(!findMass(PdgParticle::eInvalid));
  BOOST_CHECK(!findName(PdgParticle::eInvalid));
}

BOOST_AUTO_TEST_CASE(Electron) {
  BOOST_CHECK_EQUAL(*findCharge(PdgParticle::eAntiElectron), 1_e);
  CHECK_CLOSE_REL(*findMass(PdgParticle::eAntiElectron), 511_keV, eps);
  BOOST_CHECK_EQUAL(*findName(PdgParticle::eAntiElectron), "e+");
  BOOST_CHECK_EQUAL(*findCharge(PdgParticle::eElectron), -1_e);
  CHECK_CLOSE_REL(*findMass(PdgParticle::eElectron), 511_keV, eps);
  BOOST_CHECK_EQUAL(*findName(PdgParticle::eElectron), "e-");
  BOOST_CHECK_EQUAL(*findCharge(PdgParticle::ePositron), 1_e);
  CHECK_CLOSE_REL(*findMass(PdgParticle::ePositron), 511_keV, eps);
  BOOST_CHECK_EQUAL(*findName(PdgParticle::ePositron), "e+");
}

BOOST_AUTO_TEST_CASE(Gamma) {
  BOOST_CHECK_EQUAL(*findCharge(PdgParticle::eGamma), 0);
  BOOST_CHECK_EQUAL(*findMass(PdgParticle::eGamma), 0);
  BOOST_CHECK_EQUAL(*findName(PdgParticle::eGamma), "gamma");
}

BOOST_AUTO_TEST_CASE(Pion) {
  BOOST_CHECK_EQUAL(*findCharge(PdgParticle::ePionMinus), -1_e);
  CHECK_CLOSE_REL(*findMass(PdgParticle::ePionMinus), 139.57_MeV, eps);
  BOOST_CHECK_EQUAL(*findName(PdgParticle::ePionMinus), "pi-");
  BOOST_CHECK_EQUAL(*findCharge(PdgParticle::ePionPlus), 1_e);
  CHECK_CLOSE_REL(*findMass(PdgParticle::ePionPlus), 139.57_MeV, eps);
  BOOST_CHECK_EQUAL(*findName(PdgParticle::ePionPlus), "pi+");
  BOOST_CHECK_EQUAL(*findCharge(PdgParticle::ePionZero), 0);
  CHECK_CLOSE_REL(*findMass(PdgParticle::ePionZero), 134.98_MeV, eps);
  BOOST_CHECK_EQUAL(*findName(PdgParticle::ePionZero), "pi0");
}

BOOST_AUTO_TEST_SUITE_END()
