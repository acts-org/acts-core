// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Seeding/EstimateTrackParamsFromSeed.hpp"

Acts::BoundMatrix Acts::estimateTrackParamCovariance(
    const BoundVector& params,
    const EstimateTrackParamCovarianceConfig& config) {
  BoundSquareMatrix result = BoundSquareMatrix::Zero();

  for (std::size_t i = eBoundLoc0; i < eBoundSize; ++i) {
    double sigma = config.initialSigmas[i];
    double variance = sigma * sigma;

    if (i == eBoundQOverP) {
      // note that we rely on the fact that sigma theta is already computed
      double varianceTheta = result(eBoundTheta, eBoundTheta);

      // transverse momentum contribution
      variance += std::pow(config.initialSigmaPtRel * params[eBoundQOverP], 2);

      // theta contribution
      variance +=
          varianceTheta *
          std::pow(params[eBoundQOverP] / std::tan(params[eBoundTheta]), 2);
    }

    // Inflate the initial covariance
    variance *= config.initialVarInflation[i];

    result(i, i) = variance;
  }

  return result;
}
