// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/TrackParametrization.hpp"

#include <cmath>
#include <random>
#include <utility>

namespace Acts {
namespace Test {

/// Generate a random parameters vector and covariance matrix.
///
/// @return std:::pair<ParametersVector, CovarianceMatrix>
template <typename scalar_t, size_t kSize, typename generator_t>
inline auto generateParametersCovariance(generator_t& rng)
    -> std::pair<Acts::ActsVector<scalar_t, kSize>,
                 Acts::ActsSymMatrix<scalar_t, kSize>> {
  using Scalar = scalar_t;
  using ParametersVector = Acts::ActsVector<Scalar, kSize>;
  using CovarianceMatrix = Acts::ActsSymMatrix<Scalar, kSize>;

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

  return std::make_pair(params, cov);
}

/// Generate a random bound parameters vector and covariance matrix.
template <typename generator_t>
inline auto generateBoundParametersCovariance(generator_t& rng) {
  return generateParametersCovariance<BoundScalar, eBoundSize>(rng);
}

/// Generate a random free parameters vector and covariance matrix.
template <typename generator_t>
inline auto generateFreeParametersCovariance(generator_t& rng) {
  return generateParametersCovariance<FreeScalar, eFreeSize>(rng);
}

}  // namespace Test
}  // namespace Acts
