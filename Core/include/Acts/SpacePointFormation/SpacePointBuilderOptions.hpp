// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/SourceLink.hpp"

namespace Acts {

using ParamCovAccessor =
    std::function<std::pair<const BoundVector, const BoundSquareMatrix>(
        const SourceLink&)>;

struct SpacePointBuilderOptions {
  // ends of strip pairs
  std::pair<const std::pair<Vector3, Vector3>,
            const std::pair<Vector3, Vector3>>
      stripEndsPair;
  // accessor of local position and covariance from source link
  ParamCovAccessor paramCovAccessor;
  /// vertex position
  Vector3 vertex = {0., 0., 0.};
  /// Allowed increase of strip length
  double stripLengthTolerance = 0.01;
  /// Allowed increase of strip length wrt gaps between strips
  double stripLengthGapTolerance = 0.01;
};

struct StripPairOptions {
  // accessor of local position and covariance from source link
  ParamCovAccessor paramCovAccessor;
  /// vertex position
  Vector3 vertex = {0., 0., 0.};
  /// Accepted squared difference in theta for two clusters
  double diffTheta2 = 1.;
  /// Accepted squared difference in phi for two clusters
  double diffPhi2 = 1.;
  /// Accepted distance between two clusters
  double diffDist = 100. * UnitConstants::mm;
};

}  // namespace Acts
