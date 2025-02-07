// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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
