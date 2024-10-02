// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/TrackFitting/GlobalChiSquareFitter.hpp"

#include "Acts/Definitions/TrackParametrization.hpp"

void Acts::Experimental::updateGx2fCovarianceParams(
    BoundMatrix& fullCovariancePredicted, Eigen::MatrixXd& aMatrixExtended,
    const std::size_t ndfSystem) {
  // make invertible
  for (int i = 0; i < aMatrixExtended.rows(); ++i) {
    if (aMatrixExtended(i, i) == 0.) {
      aMatrixExtended(i, i) = 1.;
    }
  }

  visit_measurement(ndfSystem, [&](auto N) {
    fullCovariancePredicted.topLeftCorner<N, N>() =
        aMatrixExtended.inverse().topLeftCorner<N, N>();
  });

  return;
}
