// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Units.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "ActsFatras/Selectors/KinematicCasts.hpp"

#include <cmath>
#include <limits>

#include "Dataset.hpp"

using namespace Acts::UnitLiterals;
using namespace ActsFatras::Casts;

namespace {
// TODO why does this have to be so high to avoid failure in eta tests?
constexpr auto eps = 128 * std::numeric_limits<double>::epsilon();
}  // namespace

BOOST_AUTO_TEST_SUITE(FatrasKinematicCasts)

BOOST_AUTO_TEST_CASE(BackwardParticle) {
  const auto& particle = Dataset::backwardPion;

  CHECK_SMALL(Vrho()(particle), eps);
  CHECK_CLOSE_REL(Vz()(particle), -100_mm, eps);
  CHECK_CLOSE_REL(AbsVz()(particle), 100_mm, eps);
  CHECK_CLOSE_REL(Eta()(particle), -4.5, eps);
  CHECK_CLOSE_REL(AbsEta()(particle), 4.5, eps);
  CHECK_CLOSE_REL(Pt()(particle), 1.5_GeV / std::cosh(4.5), eps);
  CHECK_CLOSE_REL(P()(particle), 1.5_GeV, eps);
  // allow higher threshold to allow for potential mass term differences
  CHECK_CLOSE_REL(E()(particle), std::hypot(1.5_GeV, 139.57018_MeV), 1e-7);
}

BOOST_AUTO_TEST_CASE(CentralParticle) {
  const auto& particle = Dataset::centralPion;

  CHECK_SMALL(Vrho()(particle), eps);
  CHECK_SMALL(Vz()(particle), eps);
  CHECK_SMALL(AbsVz()(particle), eps);
  CHECK_SMALL(Eta()(particle), eps);
  CHECK_SMALL(AbsEta()(particle), eps);
  CHECK_CLOSE_REL(Pt()(particle), 1.5_GeV, eps);
  CHECK_CLOSE_REL(P()(particle), 1.5_GeV, eps);
  // allow higher threshold to allow for potential mass term differences
  CHECK_CLOSE_REL(E()(particle), std::hypot(1.5_GeV, 139.57018_MeV), 1e-7);
}

BOOST_AUTO_TEST_CASE(ForwardParticle) {
  const auto& particle = Dataset::forwardPion;

  CHECK_SMALL(Vrho()(particle), eps);
  CHECK_CLOSE_REL(Vz()(particle), 100_mm, eps);
  CHECK_CLOSE_REL(AbsVz()(particle), 100_mm, eps);
  CHECK_CLOSE_REL(Eta()(particle), 4.5, eps);
  CHECK_CLOSE_REL(AbsEta()(particle), 4.5, eps);
  CHECK_CLOSE_REL(Pt()(particle), 1.5_GeV / std::cosh(4.5), eps);
  CHECK_CLOSE_REL(P()(particle), 1.5_GeV, eps);
  // allow higher threshold to allow for potential mass term differences
  CHECK_CLOSE_REL(E()(particle), std::hypot(1.5_GeV, 139.57018_MeV), 1e-7);
}

BOOST_AUTO_TEST_SUITE_END()
