// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Detector/DetectorComponents.hpp"
#include "Acts/Geometry/GeometryContext.hpp"

namespace Acts::Experimental {

/// @brief This is the interface definition of internal structure
/// builders for DetectorVolume construction.
///
/// It is assumed that each builder returns a consistent set of
/// DetectorVolume internals, which in turn can be directly provided
/// to a DetectorVolume constructor.
class IInternalStructureBuilder {
 public:
  virtual ~IInternalStructureBuilder() = default;
  /// The interface definition for internal structure creation
  ///
  /// @param gctx the geometry context at the creation of the internal structure
  ///
  /// @return a consistent set of detector volume internals
  virtual InternalStructure construct(const GeometryContext& gctx) const = 0;
};

}  // namespace Acts::Experimental
