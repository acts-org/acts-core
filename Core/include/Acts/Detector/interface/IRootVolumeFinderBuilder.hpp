// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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
