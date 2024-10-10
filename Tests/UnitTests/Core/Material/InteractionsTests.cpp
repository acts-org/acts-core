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
#include "Acts/Definitions/Units.hpp"
#include "Acts/Material/Interactions.hpp"
#include "Acts/Material/Material.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Tests/CommonHelpers/PredefinedMaterials.hpp"

#include <utility>

namespace data = boost::unit_test::data;
using namespace Acts::UnitLiterals;

// fixed material
static const Acts::Material material = Acts::Test::makeSilicon();
// variable values for other parameters
// thickness
static const double valuesThickness[] = {200_um, 1_mm};
static auto thickness = data::make(valuesThickness);
// particle type, mass, and charge
static const Acts::PdgParticle pdg[] = {Acts::eElectron, Acts::eMuon,
                                        Acts::ePionPlus, Acts::eProton};
static const double mass[] = {511_keV, 105.7_MeV, 139.6_MeV, 938.3_MeV};
static const double charge[] = {-1_e, -1_e, 1_e, 1_e};
static const auto particle =
    data::make(pdg) ^ data::make(mass) ^ data::make(charge);
// momentum range
static const auto momentum_low = data::xrange(100_MeV, 10_GeV, 100_MeV);
static const auto momentum_med = data::xrange(10_GeV, 100_GeV, 10_GeV);
static const auto momentum_high = data::xrange(100_GeV, 10_TeV, 100_GeV);
static const auto momentum = momentum_low + momentum_med + momentum_high;

BOOST_AUTO_TEST_SUITE(interactions)

// consistency checks for the energy loss values
BOOST_DATA_TEST_CASE(energy_loss_consistency, thickness* particle* momentum, x,
                     i, m, q, p) {
  const auto slab = Acts::MaterialSlab(material, x);
  const auto qOverP = q / p;
  const auto absQ = std::abs(q);
  const auto absPdg = Acts::makeAbsolutePdgParticle(i);

  auto dEBethe = computeEnergyLossBethe(slab, m, qOverP, absQ);
  auto dELandau = computeEnergyLossLandau(slab, m, qOverP, absQ);
  auto dELandauSigma = computeEnergyLossLandauSigma(slab, m, qOverP, absQ);
  auto dELandauSigmaQOverP =
      computeEnergyLossLandauSigmaQOverP(slab, m, qOverP, absQ);
  auto dERad = computeEnergyLossRadiative(slab, absPdg, m, qOverP, absQ);
  auto dEMean = computeEnergyLossMean(slab, absPdg, m, qOverP, absQ);
  auto dEMode = computeEnergyLossMode(slab, absPdg, m, qOverP, absQ);

  BOOST_CHECK_LT(0, dEBethe);
  BOOST_CHECK_LT(0, dELandau);
  BOOST_CHECK_LT(0, dELandauSigma);
  BOOST_CHECK_LT(0, dELandauSigmaQOverP);
  BOOST_CHECK_LE(dELandauSigma, dEBethe);
  // radiative terms only kick above some threshold -> can be zero
  BOOST_CHECK_LE(0, dERad);
  BOOST_CHECK_LT(0, dEMean);
  BOOST_CHECK_LT(0, dEMode);
  BOOST_CHECK_LE((dEBethe + dERad), dEMean);
  // TODO verify mode/mean relation for full energy loss
  // BOOST_CHECK_LE(dEMode, dEMean);
}

// consistency checks for multiple scattering
BOOST_DATA_TEST_CASE(multiple_scattering_consistency,
                     thickness* particle* momentum, x, i, m, q, p) {
  const auto slab = Acts::MaterialSlab(material, x);
  const auto slabDoubled = Acts::MaterialSlab(material, 2 * x);
  const auto qOverP = q / p;
  const auto qOver2P = q / (2 * p);
  const auto absQ = std::abs(q);
  const auto absPdg = Acts::makeAbsolutePdgParticle(i);

  auto t0 = computeMultipleScatteringTheta0(slab, absPdg, m, qOverP, absQ);
  BOOST_CHECK_LT(0, t0);
  // use the anti-particle -> same scattering
  auto tanti = computeMultipleScatteringTheta0(slab, absPdg, m, -qOverP, absQ);
  BOOST_CHECK_LT(0, tanti);
  BOOST_CHECK_EQUAL(t0, tanti);
  // double the material -> more scattering
  auto t2x =
      computeMultipleScatteringTheta0(slabDoubled, absPdg, m, qOverP, absQ);
  BOOST_CHECK_LT(0, t2x);
  BOOST_CHECK_LT(t0, t2x);
  // double the momentum -> less scattering
  auto t2p = computeMultipleScatteringTheta0(slab, absPdg, m, qOver2P, absQ);
  BOOST_CHECK_LT(0, t2p);
  BOOST_CHECK_LT(t2p, t0);
}

// no material -> no interactions
BOOST_DATA_TEST_CASE(vacuum, thickness* particle* momentum, x, i, m, q, p) {
  const auto vacuum = Acts::MaterialSlab(Acts::Material(), x);
  const auto qOverP = q / p;
  const auto absQ = std::abs(q);
  const auto absPdg = Acts::makeAbsolutePdgParticle(i);

  BOOST_CHECK_EQUAL(computeEnergyLossBethe(vacuum, m, qOverP, absQ), 0);
  BOOST_CHECK_EQUAL(computeEnergyLossLandau(vacuum, m, qOverP, absQ), 0);
  BOOST_CHECK_EQUAL(computeEnergyLossLandauSigma(vacuum, m, qOverP, absQ), 0);
  BOOST_CHECK_EQUAL(computeEnergyLossLandauSigmaQOverP(vacuum, m, qOverP, absQ),
                    0);
  BOOST_CHECK_EQUAL(computeEnergyLossRadiative(vacuum, absPdg, m, qOverP, absQ),
                    0);
  BOOST_CHECK_EQUAL(computeEnergyLossMean(vacuum, absPdg, m, qOverP, absQ), 0);
  BOOST_CHECK_EQUAL(computeEnergyLossMode(vacuum, absPdg, m, qOverP, absQ), 0);
  BOOST_CHECK_EQUAL(
      computeMultipleScatteringTheta0(vacuum, absPdg, m, qOverP, absQ), 0);
}

// Silicon Bethe Energy Loss Validation
// PDG value from https://pdg.lbl.gov/2022/AtomicNuclearProperties
static const double momentum[] = {0.1003_GeV, 1.101_GeV, 10.11_GeV, 100.1_GeV};
static const double energy_loss[] = {2.608, 1.803, 2.177, 2.451};

BOOST_DATA_TEST_CASE(silicon_energy_loss,
                     data::make(momentum) ^ data::make(energy_loss), p, loss) {
  const Acts::Material silicon = Acts::Test::makeSilicon();
  const auto thickness = 1_cm;
  const auto slab = Acts::MaterialSlab(silicon, thickness);
  const auto m = 105.7_MeV;
  const auto q = -1_e;
  const auto qOverP = q / p;
  const auto absQ = std::abs(q);

  // Difference is within 5% from PDG value
  BOOST_CHECK_CLOSE(computeEnergyLossBethe(slab, m, qOverP, absQ) / thickness /
                        slab.material().massDensity() / (1_MeV * 1_cm2 / 1_g),
                    loss, 5.);
}

// Silicon Landau Energy Loss Validation
BOOST_AUTO_TEST_CASE(silicon_landau) {
  const Acts::Material silicon = Acts::Test::makeSilicon();
  const auto thickness = 0.17_cm;
  const auto slab = Acts::MaterialSlab(silicon, thickness);
  const float m = 105.7_MeV;
  const float q = -1_e;
  const float qOverP = q / 10_GeV;
  const float absQ = std::abs(q);

  // Difference is within 5% from PDG value
  const auto dE = computeEnergyLossLandau(slab, m, qOverP, absQ) / 1_MeV;
  BOOST_CHECK_CLOSE(dE, 0.525, 5.);

  // Difference is within 10% from PDG value
  const auto fwhm =
      Acts::computeEnergyLossLandauFwhm(slab, m, qOverP, absQ) / 1_MeV;
  BOOST_CHECK_CLOSE(fwhm, 0.13, 10.);
}

BOOST_AUTO_TEST_SUITE_END()
