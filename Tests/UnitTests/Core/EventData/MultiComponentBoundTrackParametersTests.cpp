// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/GenericBoundTrackParameters.hpp"
#include "Acts/EventData/ParticleHypothesis.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Surfaces/CurvilinearSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include <Acts/EventData/Charge.hpp>
#include <Acts/EventData/MultiComponentTrackParameters.hpp>
#include <Acts/Surfaces/PlaneSurface.hpp>

#include <algorithm>
#include <initializer_list>
#include <memory>
#include <optional>
#include <tuple>
#include <vector>

using namespace Acts;

static const auto particleHypothesis = ParticleHypothesis::pion();

BOOST_AUTO_TEST_CASE(test_constructors) {
  std::vector<std::tuple<double, BoundVector, BoundSquareMatrix>> a;
  a.push_back({1.0, BoundVector::Ones(), BoundSquareMatrix::Identity()});

  std::vector<std::tuple<double, BoundVector, std::optional<BoundSquareMatrix>>>
      b;
  b.push_back({1.0, BoundVector::Ones(), BoundSquareMatrix::Identity()});

  auto surface =
      CurvilinearSurface(Vector3::Ones(), Vector3::Ones().normalized())
          .planeSurface();

  const auto ap =
      MultiComponentBoundTrackParameters(surface, a, particleHypothesis);
  const auto bp =
      MultiComponentBoundTrackParameters(surface, b, particleHypothesis);
  const auto aps = MultiComponentBoundTrackParameters(
      surface, std::get<1>(a.front()), std::get<2>(a.front()),
      particleHypothesis);
  const auto bps = MultiComponentBoundTrackParameters(
      surface, std::get<1>(b.front()), std::get<2>(b.front()),
      particleHypothesis);

  BOOST_CHECK(b == ap.components());
  BOOST_CHECK(ap.components() == bp.components());
  BOOST_CHECK(bp.components() == aps.components());
  BOOST_CHECK(aps.components() == bps.components());
}

BOOST_AUTO_TEST_CASE(test_accessors) {
  using cov_t = std::optional<BoundSquareMatrix>;
  for (const auto &cov : {cov_t{}, cov_t{BoundSquareMatrix::Identity()},
                          cov_t{BoundSquareMatrix::Identity()}}) {
    auto surface =
        CurvilinearSurface(Vector3::Ones(), Vector3::Ones().normalized())
            .planeSurface();

    const BoundTrackParameters single_pars(surface, BoundVector::Ones(), cov,
                                           particleHypothesis);

    const auto multi_pars = [&]() {
      std::vector<
          std::tuple<double, BoundVector, std::optional<BoundSquareMatrix>>>
          a;
      for (int i = 0; i < 4; ++i) {
        a.push_back({0.25, single_pars.parameters(), single_pars.covariance()});
      }
      return MultiComponentBoundTrackParameters(surface, a, particleHypothesis);
    }();

    BOOST_CHECK_EQUAL(multi_pars.absoluteMomentum(),
                      single_pars.absoluteMomentum());
    BOOST_CHECK_EQUAL(multi_pars.charge(), single_pars.charge());
    BOOST_CHECK_EQUAL(multi_pars.fourPosition(GeometryContext{}),
                      single_pars.fourPosition(GeometryContext{}));
    BOOST_CHECK_EQUAL(multi_pars.momentum(), single_pars.momentum());
    BOOST_CHECK_EQUAL(multi_pars.parameters(), single_pars.parameters());
    BOOST_CHECK_EQUAL(multi_pars.position(GeometryContext{}),
                      single_pars.position(GeometryContext{}));
    BOOST_CHECK_EQUAL(multi_pars.transverseMomentum(),
                      single_pars.transverseMomentum());
    BOOST_CHECK_EQUAL(multi_pars.direction(), single_pars.direction());

    // Check the behaviour for std::nullopt or zero covariance
    if (cov && *cov != BoundSquareMatrix::Zero()) {
      BOOST_CHECK_EQUAL(*multi_pars.covariance(), *single_pars.covariance());
    } else {
      BOOST_CHECK(!multi_pars.covariance());
    }
  }
}
