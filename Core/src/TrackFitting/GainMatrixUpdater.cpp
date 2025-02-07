// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/TrackFitting/GainMatrixUpdater.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/MeasurementHelpers.hpp"
#include "Acts/TrackFitting/KalmanFitterError.hpp"

#include <algorithm>
#include <cstddef>
#include <type_traits>
#include <utility>

#include <Eigen/src/Core/MatrixBase.h>

namespace Acts {

std::tuple<double, std::error_code> GainMatrixUpdater::visitMeasurement(
    InternalTrackState trackState, const Logger& logger) const {
  // default-constructed error represents success, i.e. an invalid error code

  return visit_measurement(
      trackState.calibratedSize,
      [&, this]<std::size_t N>(std::integral_constant<std::size_t, N>)
          -> std::tuple<double, std::error_code> {
        return visitMeasurementImpl<N>(trackState, logger);
      });
}

}  // namespace Acts
