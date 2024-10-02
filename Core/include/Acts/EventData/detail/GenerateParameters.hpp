// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Utilities/detail/periodic.hpp"

#include <cmath>
#include <random>
#include <utility>

namespace Acts::detail::Test {

/// Generate a random parameters vector and covariance matrix.
///
/// @return std:::pair<ParametersVector, CovarianceMatrix>
template <typename scalar_t, std::size_t kSize, typename generator_t>
inline auto generateParametersCovariance(generator_t& rng)
    -> std::pair<Eigen::Matrix<scalar_t, kSize, 1>,
                 Eigen::Matrix<scalar_t, kSize, kSize>> {
  using Scalar = scalar_t;
  using ParametersVector = Eigen::Matrix<scalar_t, kSize, 1>;
  using CovarianceMatrix = Eigen::Matrix<scalar_t, kSize, kSize>;

  std::normal_distribution<Scalar> distNormal(0, 1);
  std::uniform_real_distribution<Scalar> distCorr(-1, 1);

  // generate standard deviations
  ParametersVector stddev;
  for (auto i = 0u; i < kSize; ++i) {
    stddev[i] = std::abs(distNormal(rng));
  }
  // generate correlation matrix
  CovarianceMatrix corr;
  for (auto i = 0u; i < kSize; ++i) {
    corr(i, i) = 1;
    // only need generate the sub-diagonal elements
    for (auto j = 0u; j < i; ++j) {
      corr(i, j) = corr(j, i) = distCorr(rng);
    }
  }
  // construct the covariance matrix
  CovarianceMatrix cov = stddev.asDiagonal() * corr * stddev.asDiagonal();

  // generate random parameters
  // this is ignoring the correlations; since this does not need to generate
  // credible data, this should be fine.
  ParametersVector params;
  for (auto i = 0u; i < kSize; ++i) {
    params[i] = stddev[i] * distNormal(rng);
  }

  return {params, cov};
}

/// Generate a random bound parameters vector and covariance matrix.
template <typename generator_t>
inline auto generateBoundParametersCovariance(generator_t& rng) {
  auto parCov = generateParametersCovariance<ActsScalar, eBoundSize>(rng);
  auto [phi, theta] = detail::normalizePhiTheta(parCov.first[eBoundPhi],
                                                parCov.first[eBoundTheta]);
  parCov.first[eBoundPhi] = phi;
  parCov.first[eBoundTheta] = theta;
  return parCov;
}

/// Generate a random free parameters vector and covariance matrix.
template <typename generator_t>
inline auto generateFreeParametersCovariance(generator_t& rng) {
  return generateParametersCovariance<ActsScalar, eFreeSize>(rng);
}

}  // namespace Acts::detail::Test
