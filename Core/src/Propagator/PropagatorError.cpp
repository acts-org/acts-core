// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Propagator/PropagatorError.hpp"

#include <string>

namespace {

class PropagatorErrorCategory : public std::error_category {
 public:
  // Return a short descriptive name for the category.
  const char* name() const noexcept final { return "PropagatorError"; }

  // Return what each enum means in text.
  std::string message(int c) const final {
    using Acts::PropagatorError;

    switch (static_cast<PropagatorError>(c)) {
      case PropagatorError::Failure:
        return "Propagation failed";
      case PropagatorError::WrongDirection:
        return "Propagation occurred in the wrong direction";
      case PropagatorError::StepCountLimitReached:
        return "Propagation reached the configured maximum number of steps";
      default:
        return "unknown";
    }
  }
};

}  // namespace

std::error_code Acts::make_error_code(Acts::PropagatorError e) {
  static PropagatorErrorCategory c;
  return {static_cast<int>(e), c};
}
