// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

// Set the Mangetic Field Context PLUGIN
#ifdef ACTS_CORE_MAGFIELDCONTEXT_PLUGIN
#include ACTS_CORE_MAGFIELDCONTEXT_PLUGIN
#else

#include "Acts/Utilities/detail/ContextType.hpp"

namespace Acts {

/// @brief This is the central definition of the Acts
/// payload object regarding magnetic field status
///
/// It is propagated through the code to allow for event/thread
/// dependent magnetic field changes

class MagneticFieldContext : public ContextType {
 public:
  /// Inherit all constructors
  using ContextType::ContextType;
  using ContextType::operator=;
};

}  // namespace Acts

#endif
