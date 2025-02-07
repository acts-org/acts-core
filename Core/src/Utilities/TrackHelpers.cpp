// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Utilities/TrackHelpers.hpp"

#include <string>

namespace {

class TrackExtrapolationErrorCategory : public std::error_category {
 public:
  // Return a short descriptive name for the category.
  const char* name() const noexcept final { return "TrackExtrapolationError"; }

  // Return what each enum means in text.
  std::string message(int c) const final {
    using Acts::TrackExtrapolationError;

    switch (static_cast<TrackExtrapolationError>(c)) {
      case TrackExtrapolationError::CompatibleTrackStateNotFound:
        return "Did not find a compatible track state";
      case TrackExtrapolationError::ReferenceSurfaceUnreachable:
        return "Provided reference surface is unreachable";
      default:
        return "unknown";
    }
  }
};

}  // namespace

std::error_code Acts::make_error_code(Acts::TrackExtrapolationError e) {
  static const TrackExtrapolationErrorCategory c;
  return {static_cast<int>(e), c};
}
