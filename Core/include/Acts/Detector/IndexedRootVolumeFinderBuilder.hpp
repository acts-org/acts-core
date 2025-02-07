// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Detector/interface/IRootVolumeFinderBuilder.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Navigation/PortalNavigation.hpp"
#include "Acts/Utilities/AxisDefinitions.hpp"

#include <memory>
#include <vector>

namespace Acts::Experimental {

class DetectorVolume;

/// @brief This is the interface for builders that create root volume finder
/// delegates
class IndexedRootVolumeFinderBuilder final : public IRootVolumeFinderBuilder {
 public:
  /// @brief Constructor with binning casts
  /// @param binning the cast values for the grid binning
  IndexedRootVolumeFinderBuilder(std::vector<Acts::AxisDirection> binning);

  /// The virtual interface definition for root volume finder builders
  ///
  /// @param gctx the geometry context at the creation of the internal structure
  /// @param rootVolumes the root volumes to be used for the finder
  ///
  /// @return a shared detector object
  ExternalNavigationDelegate construct(
      const GeometryContext& gctx,
      const std::vector<std::shared_ptr<DetectorVolume>>& rootVolumes)
      const final;

 private:
  std::vector<Acts::AxisDirection> m_casts;
};

}  // namespace Acts::Experimental
