// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/TrackFitting/GlobalChiSquareFitterError.hpp"

#include <string>

namespace {

class GlobalChiSquareFitterErrorCategory : public std::error_category {
 public:
  // Return a short descriptive name for the category.
  const char* name() const noexcept final {
    return "GlobalChiSquareFitterError";
  }

  // Return what each enum means in text.
  std::string message(int c) const final {
    using Acts::Experimental::GlobalChiSquareFitterError;

    switch (static_cast<GlobalChiSquareFitterError>(c)) {
      case GlobalChiSquareFitterError::AIsNotInvertible:
        return "Gx2f: aMatrix is not invertible.";
      case GlobalChiSquareFitterError::DidNotConverge:
        return "Gx2f: Did not converge in 'nUpdateMax' updates.";
      case GlobalChiSquareFitterError::NotEnoughMeasurements:
        return "Gx2f: Not enough measurements.";
      case GlobalChiSquareFitterError::UpdatePushedToNewVolume:
        return "Gx2f: Update pushed the parameters to a new volume.";
      case GlobalChiSquareFitterError::UsedUnreachableMeasurements:
        return "Gx2f: Final fit used unreachable measurements.";
      default:
        return "unknown";
    }
  }
};

}  // namespace

std::error_code Acts::Experimental::make_error_code(
    Acts::Experimental::GlobalChiSquareFitterError e) {
  static GlobalChiSquareFitterErrorCategory c;
  return {static_cast<int>(e), c};
}
