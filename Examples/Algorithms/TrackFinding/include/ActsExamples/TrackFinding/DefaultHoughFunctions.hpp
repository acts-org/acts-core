// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Utilities/Result.hpp"

#pragma once

enum class HoughError {
  Failure = 1,
  SomethingElse,
};
std::error_code make_error_code(HoughError e) {
  return {static_cast<int>(e), std::generic_category()};
}

namespace std {
// register with STL
template <>
struct is_error_code_enum<HoughError> : std::true_type {};
}  // namespace std

namespace ActsExamples::DefaultHoughFunctions {
using ResultDouble = Acts::Result<double>;
using ResultBool = Acts::Result<bool>;
using ResultUnsigned = Acts::Result<unsigned>;

ResultDouble fieldCorrectionDefault(unsigned region, double y, double r) {
  if (region == 999) {
    return y + r;  // this should not be found, for now this is a dummy to show
                   // what one *could* do
  }
  return ResultDouble::success(0.0);
}

ResultUnsigned findLayerIDDefault(double r) {
  if (r < 50) {
    return ResultUnsigned::success(0);
  } else if (r < 100) {
    return ResultUnsigned::success(1);
  } else if (r < 150) {
    return ResultUnsigned::success(2);
  } else if (r < 200) {
    return ResultUnsigned::success(3);
  } else if (r < 300) {
    return ResultUnsigned::success(4);
  } else if (r < 400) {
    return ResultUnsigned::success(5);
  } else if (r < 550) {
    return ResultUnsigned::success(6);
  } else if (r < 700) {
    return ResultUnsigned::success(7);
  } else if (r < 900) {
    return ResultUnsigned::success(8);
  } else if (r < 1100) {
    return ResultUnsigned::success(9);
  }
  return ResultUnsigned::failure(
      HoughError::Failure);  /// shouldn't be here, this won't be used
}

// default with two slices, one for negative and one for positive z, counting
// some small overlaps, and -1 means "just take everything"
ResultBool inSliceDefault(double z, unsigned layer, int slice) {
  if (slice == -1) {
    return ResultBool::success(true);
  }

  double absz = abs(z);
  if (slice == 0 && z > 50) {
    return ResultBool::success(false);
  } else if (slice == 1 && z < -50) {
    return ResultBool::success(false);
  } else {
    if (layer <= 3) {
      if (absz < 200) {
        return ResultBool::success(true);
      } else {
        return ResultBool::success(false);
      }
    } else if (layer == 4) {
      if (absz < 300) {
        return ResultBool::success(true);
      } else {
        return ResultBool::success(false);
      }
    } else if (layer == 5) {
      if (absz < 400) {
        return ResultBool::success(true);
      } else {
        return ResultBool::success(false);
      }
    } else if (layer == 6) {
      if (absz < 600) {
        return ResultBool::success(true);
      } else {
        return ResultBool::success(false);
      }
    } else if (layer == 7) {
      if (absz < 700) {
        return ResultBool::success(true);
      } else {
        return ResultBool::success(false);
      }
    } else if (layer == 8) {
      if (absz < 800) {
        return ResultBool::success(true);
      } else {
        return ResultBool::success(false);
      }
    } else if (layer == 9) {
      if (absz < 1100) {
        return ResultBool::success(true);
      } else {
        return ResultBool::success(false);
      }
    } else {
      return ResultBool::success(false);
    }
  }
}
}  // namespace ActsExamples::DefaultHoughFunctions
