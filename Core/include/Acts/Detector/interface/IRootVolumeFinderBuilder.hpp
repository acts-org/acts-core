// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Navigation/PortalNavigation.hpp"

#include <memory>
#include <vector>

namespace Acts::Experimental {

class DetectorVolume;

/// @brief This is the interface for builders that create root volume finder
/// delegates
class IRootVolumeFinderBuilder {
 public:
  virtual ~IRootVolumeFinderBuilder() = default;

  /// The virtual interface definition for root volume finder builders
  ///
  /// @param gctx the geometry context at the creation of the internal structure
  /// @param rootVolumes the root volumes to be used for the search
  ///
  /// @return a shared detector object
  virtual ExternalNavigationDelegate construct(
      const GeometryContext& gctx,
      const std::vector<std::shared_ptr<DetectorVolume>>& rootVolumes)
      const = 0;
};

}  // namespace Acts::Experimental
