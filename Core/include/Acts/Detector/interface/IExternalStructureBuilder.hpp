// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
