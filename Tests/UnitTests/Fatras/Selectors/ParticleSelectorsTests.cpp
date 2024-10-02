// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/PdgParticle.hpp"
#include "ActsFatras/EventData/Barcode.hpp"
#include "ActsFatras/Selectors/ParticleSelectors.hpp"

#include "Dataset.hpp"

using namespace ActsFatras;

BOOST_AUTO_TEST_SUITE(FatrasParticleSelectors)

BOOST_AUTO_TEST_CASE(NegativeParticle) {
  const auto& particle = Dataset::centralElectron;
  BOOST_CHECK(!NeutralSelector()(particle));
  BOOST_CHECK(ChargedSelector()(particle));
  BOOST_CHECK(!PositiveSelector()(particle));
  BOOST_CHECK(NegativeSelector()(particle));
}

BOOST_AUTO_TEST_CASE(NeutralParticle) {
  const auto& particle = Dataset::centralNeutron;
  BOOST_CHECK(NeutralSelector()(particle));
  BOOST_CHECK(!ChargedSelector()(particle));
  BOOST_CHECK(!PositiveSelector()(particle));
  BOOST_CHECK(!NegativeSelector()(particle));
}

BOOST_AUTO_TEST_CASE(PositiveParticle) {
  const auto& particle = Dataset::centralPositron;
  BOOST_CHECK(!NeutralSelector()(particle));
  BOOST_CHECK(ChargedSelector()(particle));
  BOOST_CHECK(PositiveSelector()(particle));
  BOOST_CHECK(!NegativeSelector()(particle));
}

namespace {
const auto& electron = Dataset::centralElectron;
const auto& positron = Dataset::centralPositron;
const auto& muon = Dataset::centralMuon;
const auto& antimuon = Dataset::centralAntiMuon;
const auto& pion = Dataset::centralPion;
}  // namespace

BOOST_AUTO_TEST_CASE(PdgSelector) {
  ActsFatras::PdgSelector<Acts::PdgParticle::eElectron> selectElectron;
  BOOST_CHECK(selectElectron(electron));
  BOOST_CHECK(!selectElectron(positron));
  BOOST_CHECK(!selectElectron(muon));
  BOOST_CHECK(!selectElectron(antimuon));
  BOOST_CHECK(!selectElectron(pion));
}

BOOST_AUTO_TEST_CASE(AbsPdgSelector) {
  ActsFatras::AbsPdgSelector<Acts::PdgParticle::eElectron> selectElectronLike;
  BOOST_CHECK(selectElectronLike(electron));
  BOOST_CHECK(selectElectronLike(positron));
  BOOST_CHECK(!selectElectronLike(muon));
  BOOST_CHECK(!selectElectronLike(antimuon));
  BOOST_CHECK(!selectElectronLike(pion));
}

BOOST_AUTO_TEST_CASE(PdgExcluder) {
  ActsFatras::PdgExcluder<Acts::PdgParticle::eMuon> excludeMuon;
  BOOST_CHECK(excludeMuon(electron));
  BOOST_CHECK(excludeMuon(positron));
  BOOST_CHECK(!excludeMuon(muon));
  BOOST_CHECK(excludeMuon(antimuon));
  BOOST_CHECK(excludeMuon(pion));
}

BOOST_AUTO_TEST_CASE(AbsPdgExcluder) {
  ActsFatras::AbsPdgExcluder<Acts::PdgParticle::eMuon> excludeMuonLike;
  BOOST_CHECK(excludeMuonLike(electron));
  BOOST_CHECK(excludeMuonLike(positron));
  BOOST_CHECK(!excludeMuonLike(muon));
  BOOST_CHECK(!excludeMuonLike(antimuon));
  BOOST_CHECK(excludeMuonLike(pion));
}

BOOST_AUTO_TEST_SUITE_END()
