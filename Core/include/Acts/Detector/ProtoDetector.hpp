// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Geometry/Extent.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/BinningData.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

namespace Acts {

struct ProtoVolume;

namespace Experimental {

class DetectorVolume;
class Portal;

/// Current volumes (connected)
using DetectorVolumes = std::vector<std::shared_ptr<DetectorVolume>>;
/// Current shell (i.e. outside portals)
using ProtoContainer = std::map<unsigned int, std::shared_ptr<Portal>>;
/// Current block (volumes and shell)
using DetectorBlock = std::tuple<DetectorVolumes, ProtoContainer>;

/// The detector builder function
using DetectorBlockBuilder = std::function<void(
    DetectorBlock&, const GeometryContext&, Acts::Logging::Level)>;
}  // namespace Experimental

/// A proto volume description being used to define an overall
/// structure of either a TrackingVolume or Experimental::DetectorVolume
struct ProtoVolume {
  // Internal structure information
  struct InternalStructure {
    /// Possibility to provide a layer type information
    Surface::SurfaceType layerType = Surface::SurfaceType::Other;
    /// Possibility to provide a surface binning
    std::vector<BinningData> surfaceBinning = {};
  };

  // Container structure information
  struct ContainerStructure {
    /// Internal structure container
    std::vector<ProtoVolume> constituentVolumes = {};
    /// The constituent binning if this a container
    std::vector<BinningData> constituentBinning = {};
    /// Layer container flag
    bool layerContainer = false;
  };

  /// Name of the proto volume
  std::string name = "";
  /// The extent of this volume
  Extent extent;

  /// Information about internal structure - legacy building
  std::optional<InternalStructure> internal = std::nullopt;

  /// Information about container structure - legacy building
  std::optional<ContainerStructure> container = std::nullopt;

  /// An attached Detector volume Builder - new detector schema
  Experimental::DetectorBlockBuilder blockBuilder;

  /// Define an operator==
  ///
  /// @param ptVolume the proto volume to be checked
  bool operator==(const ProtoVolume& ptVolume) const;

  /// Harmonize the detector information, this can run in two
  /// modes, steered by the @param legacy boolean
  ///
  /// The legacy mode prepares everything for `Acts::TrackingVolume`,
  /// if off it creates a description for `Acts::Detector`.
  void harmonize(bool legacy = true);

  /// Extend the tracking volume with its own constituents,
  /// upwards here means that extents are promoted to the mother
  ///
  /// @param ptVolume the protoVolume
  void extendUp(ProtoVolume& ptVolume);

  /// Extend the tracking volume with its own constituents
  /// @param aDir the axis direction that is propagated
  void propagateMinDown(AxisDirection aDir);

  /// Extend the tracking volume with its own constituents
  /// @param aDir the axis direction that is propagated
  void propagateMaxDown(AxisDirection aDir);

  /// Constrain the daughter volumes with this volume
  ///
  /// @param ptVolume is the proto volume from which the constrain
  /// is taken
  void constrainDown(const ProtoVolume& ptVolume);

  /// Write the tracking volume to screen
  /// @param indent the current indentation
  std::string toString(const std::string& indent = "") const;
};

/// A proto detector description being used to define an overall
/// structure of either a TrackingGeometry or Experimental::Detector
struct ProtoDetector {
  std::string name = "";
  ProtoVolume worldVolume;

  /// Harmonize the detector information, this can run in two
  /// modes, steered by the @param legacy boolean
  ///
  /// The legacy mode prepares everything for `Acts::TrackingVolume`,
  /// if off it creates a description for `Acts::Detector`.
  ///
  void harmonize(bool legacy = true) {
    worldVolume.extendUp(worldVolume);
    worldVolume.constrainDown(worldVolume);
    worldVolume.harmonize(legacy);
  }

  /// Write the tracking volume to screen
  /// @param indent the current indentation
  std::string toString(const std::string& indent = "") const;
};

}  // namespace Acts
