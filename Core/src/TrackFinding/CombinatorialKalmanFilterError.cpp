// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/TrackFinding/CombinatorialKalmanFilterError.hpp"

#include <string>

namespace {

class CombinatorialKalmanFilterErrorCategory : public std::error_category {
 public:
  // Return a short descriptive name for the category.
  const char* name() const noexcept final {
    return "CombinatorialKalmanFilterError";
  }

  // Return what each enum means in text.
  std::string message(int c) const final {
    using Acts::CombinatorialKalmanFilterError;

    switch (static_cast<CombinatorialKalmanFilterError>(c)) {
      case CombinatorialKalmanFilterError::UpdateFailed:
        return "Kalman update failed";
      case CombinatorialKalmanFilterError::SmoothFailed:
        return "Kalman smooth failed";
      case CombinatorialKalmanFilterError::OutputConversionFailed:
        return "Kalman output conversion failed";
      case CombinatorialKalmanFilterError::MeasurementSelectionFailed:
        return "Measurement selection failed";
      case CombinatorialKalmanFilterError::PropagationReachesMaxSteps:
        return "Propagation reaches max steps before track finding is "
               "finished";
      default:
        return "unknown";
    }
  }
};

}  // namespace

std::error_code Acts::make_error_code(Acts::CombinatorialKalmanFilterError e) {
  static CombinatorialKalmanFilterErrorCategory c;
  return {static_cast<int>(e), c};
}
