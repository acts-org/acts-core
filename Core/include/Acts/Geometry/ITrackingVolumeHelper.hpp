// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Utilities/BinningType.hpp"

#include <memory>
#include <string>
#include <vector>

namespace Acts {

class Layer;
class TrackingVolume;
class VolumeBounds;
class IVolumeMaterial;

using LayerPtr = std::shared_ptr<const Layer>;
using TrackingVolumePtr = std::shared_ptr<const TrackingVolume>;
using MutableTrackingVolumePtr = std::shared_ptr<TrackingVolume>;

using LayerVector = std::vector<LayerPtr>;
using TrackingVolumeVector = std::vector<TrackingVolumePtr>;
using MutableTrackingVolumeVector = std::vector<MutableTrackingVolumePtr>;

///  @class ITrackingVolumeHelper
///
/// Interface class ITrackingVolumeHelper tools, it inherits from IAlgTool.
/// The ITrackingVolumeHelper is a tool to pack a set of layers into a volume,
/// or - to wrap several volumes into a container volume.
///
/// @todo add documentation how this is done
///
/// TrackingVolumes only exist as std::shared_ptr
///
class ITrackingVolumeHelper {
 public:
  /// Virtual destructor
  virtual ~ITrackingVolumeHelper() = default;

  /// Create a TrackingVolume* from a set of layers and (optional) parameters
  ///
  /// @param gctx is the geometry context for witch the volume is built
  /// @param layers vector of static layers confined by the TrackingVolume
  /// if no bounds or HepTransform is given, they define the size
  /// together with the volume enevlope parameters
  /// @param volumeMaterial material properties for this TrackingVolume
  /// @param volumeBounds: confinement of this TrackingVolume
  /// @param mtvVector (optional) Vector of confined TrackingVolumes
  /// @param transform (optional) placement of this TrackingVolume
  /// @param volumeName  volume name to be given
  /// @param btype (optional) BinningType - arbitrary(default) or equidistant
  ///
  /// @return shared pointer to a new TrackingVolume
  virtual MutableTrackingVolumePtr createTrackingVolume(
      const GeometryContext& gctx, const LayerVector& layers,
      std::shared_ptr<const IVolumeMaterial> volumeMaterial,
      std::shared_ptr<VolumeBounds> volumeBounds,
      MutableTrackingVolumeVector mtvVector = {},
      const Transform3& transform = Transform3::Identity(),
      const std::string& volumeName = "UndefinedVolume",
      BinningType btype = arbitrary) const = 0;

  /// Create a TrackingVolume* from a set of layers and (optional) parameters
  ///
  /// @param gctx is the geometry context for witch the volume is built
  /// @param layers vector of static layers confined by the TrackingVolume
  /// if no bounds or HepTransform is given, they define the size
  /// together with the volume enevlope parameters
  /// @param volumeMaterial material properties for this TrackingVolume
  /// @param mtvVector Vector of confined TrackingVolumes
  /// @param loc0Min, loc0Max, loc1Min, loc1Max : local position in space,
  /// this TrackingVolume is restricted to Translation only
  /// @param volumeName  volume name to be given
  /// @param btype (optional) BinningType - arbitrary(default) or equidistant
  ///
  /// @return shared pointer to a new TrackingVolume
  virtual MutableTrackingVolumePtr createTrackingVolume(
      const GeometryContext& gctx, const LayerVector& layers,
      MutableTrackingVolumeVector mtvVector,
      std::shared_ptr<const IVolumeMaterial> volumeMaterial, double loc0Min,
      double loc0Max, double loc1Min, double loc1Max,
      const std::string& volumeName = "UndefinedVolume",
      BinningType btype = arbitrary) const = 0;

  /// Create a gap volume from dimensions and
  ///
  /// @param gctx is the geometry context for witch the volume is built
  /// @param mtvVector Vector of confined TrackingVolumes
  /// @param volumeMaterial material properties for this TrackingVolume
  /// @param loc0Min, loc0Max, loc1Min, loc1Max : local position in space,
  /// this TrackingVolume is restricted to Translation only
  /// @param materialLayers number of material layers (equidistant binning)
  /// @param cylinder type of layers
  /// @param volumeName  volume name to be given
  ///
  /// @return shared pointer to a new TrackingVolume
  virtual MutableTrackingVolumePtr createGapTrackingVolume(
      const GeometryContext& gctx, MutableTrackingVolumeVector& mtvVector,
      std::shared_ptr<const IVolumeMaterial> volumeMaterial, double loc0Min,
      double loc0Max, double loc1Min, double loc1Max,
      unsigned int materialLayers, bool cylinder = true,
      const std::string& volumeName = "UndefinedVolume") const = 0;

  /// Create a gap volume from dimensions and
  ///
  /// @param gctx is the geometry context for witch the volume is built
  /// @param mtvVector Vector of confined TrackingVolumes
  /// @param volumeMaterial material properties for this TrackingVolume
  /// @param loc0Min, loc0Max, loc1Min, loc1Max local position in space,
  /// @param layerPositions custom layer positions
  /// @param cylinder type of layers
  /// @param volumeName  : volume name to be given
  /// @param btype (optional) BinningType - arbitrary(default) or equidistant
  ///
  /// @return shared pointer to a new TrackingVolume
  virtual MutableTrackingVolumePtr createGapTrackingVolume(
      const GeometryContext& gctx, MutableTrackingVolumeVector& mtvVector,
      std::shared_ptr<const IVolumeMaterial> volumeMaterial, double loc0Min,
      double loc0Max, double loc1Min, double loc1Max,
      const std::vector<double>& layerPositions, bool cylinder = true,
      const std::string& volumeName = "UndefinedVolume",
      BinningType btype = arbitrary) const = 0;

  /// Create a one level higher TrackingVolue
  ///
  /// @param gctx is the geometry context for witch the volume is built
  /// @param volumes the volumes to be contained
  ///
  /// @return shared pointer to a new TrackingVolume
  virtual MutableTrackingVolumePtr createContainerTrackingVolume(
      const GeometryContext& gctx,
      const TrackingVolumeVector& volumes) const = 0;
};

}  // namespace Acts
