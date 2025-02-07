// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Detector/DetectorComponents.hpp"
#include "Acts/Geometry/GeometryContext.hpp"

namespace Acts::Experimental {

/// @brief This is the interface definition of external structure
/// builders for DetectorVolume construction.
class IExternalStructureBuilder {
 public:
  virtual ~IExternalStructureBuilder() = default;

  /// The virtual interface definition for external structure creation
  ///
  /// @param gctx the geometry context at the creation of the internal structure
  ///
  /// @return a consistent set of detector volume externals
  virtual ExternalStructure construct(const GeometryContext& gctx) const = 0;
};

}  // namespace Acts::Experimental
