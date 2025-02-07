// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/TrackFitting/KalmanFitterError.hpp"

#include <string>

namespace {

class KalmanFitterErrorCategory : public std::error_category {
 public:
  // Return a short descriptive name for the category.
  const char* name() const noexcept final { return "KalmanFitterError"; }

  // Return what each enum means in text.
  std::string message(int c) const final {
    using Acts::KalmanFitterError;

    switch (static_cast<KalmanFitterError>(c)) {
      case KalmanFitterError::UpdateFailed:
        return "Kalman update failed";
      case KalmanFitterError::SmoothFailed:
        return "Kalman smooth failed";
      case KalmanFitterError::OutputConversionFailed:
        return "Kalman output conversion failed";
      case KalmanFitterError::NoMeasurementFound:
        return "No measurement detected during the propagation";
      case KalmanFitterError::ReversePropagationFailed:
        return "Reverse propagation failed";
      default:
        return "unknown";
    }
  }
};

}  // namespace

std::error_code Acts::make_error_code(Acts::KalmanFitterError e) {
  static KalmanFitterErrorCategory c;
  return {static_cast<int>(e), c};
}
