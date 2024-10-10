// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

// Set the Calibration Context PLUGIN
#ifdef ACTS_CORE_CALIBRATIONCONTEXT_PLUGIN
#include ACTS_CORE_CALIBRATIONCONTEXT_PLUGIN
#else

#include "Acts/Utilities/detail/ContextType.hpp"

namespace Acts {

/// @brief This is the central definition of the Acts
/// payload object regarding detector calibration
///
/// It is propagated through the code to allow for event/thread
/// dependent calibration

class CalibrationContext : public ContextType {
 public:
  /// Inherit all constructors
  using ContextType::ContextType;
  using ContextType::operator=;
};

}  // namespace Acts

#endif
