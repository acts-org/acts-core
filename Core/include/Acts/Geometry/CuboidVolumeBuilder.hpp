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
#include "Acts/Geometry/ITrackingVolumeBuilder.hpp"
#include "Acts/Utilities/BinningType.hpp"

#include <array>
#include <cstddef>
#include <functional>
#include <iosfwd>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace Acts {

class TrackingVolume;
class VolumeBounds;
class RectangleBounds;
class ISurfaceMaterial;
class IVolumeMaterial;
class DetectorElementBase;
class Surface;
class Layer;

/// @brief This class builds a box detector with a configurable amount of
/// surfaces in it. The idea is to allow a quick configuration of a detector for
/// mostly unit test applications. Therefore this class does not demand to be a
/// universal construction factory but a raw first draft of the idea of factory
/// that may be extended in the future.
class CuboidVolumeBuilder : public ITrackingVolumeBuilder {
 public:
  /// @brief This struct stores the data for the construction of a single
  /// PlaneSurface
  struct SurfaceConfig {
    // Center position
    Vector3 position;
    // Rotation
    RotationMatrix3 rotation = RotationMatrix3::Identity();
    // Bounds
    std::shared_ptr<const RectangleBounds> rBounds = nullptr;
    // Attached material
    std::shared_ptr<const ISurfaceMaterial> surMat = nullptr;
    // Thickness
    double thickness = 0.;
    // Constructor function for optional detector elements
    // Arguments are transform, rectangle bounds and thickness.
    std::function<DetectorElementBase*(
        const Transform3&, std::shared_ptr<const RectangleBounds>, double)>
        detElementConstructor;
  };

  /// @brief This struct stores the data for the construction of a PlaneLayer
  struct LayerConfig {
    // Configuration of the surface
    std::vector<SurfaceConfig> surfaceCfg;
    // Encapsulated surface
    std::vector<std::shared_ptr<const Surface>> surfaces;
    // Boolean flag if layer is active
    bool active = false;
    // Bins in Y direction
    std::size_t binsY = 1;
    // Bins in Z direction
    std::size_t binsZ = 1;
    // Envelope in X
    std::array<ActsScalar, 2u> envelopeX{0, 0};
    // Envelope in Y
    std::array<ActsScalar, 2u> envelopeY{0, 0};
    // Envelope in Z
    std::array<ActsScalar, 2u> envelopeZ{0, 0};
    // An optional rotation for this
    std::optional<RotationMatrix3> rotation{std::nullopt};
    // Dimension for the binning
    Acts::BinningValue binningDimension = Acts::BinningValue::binX;
  };

  /// @brief This struct stores the data for the construction of a cuboid
  /// TrackingVolume with a given number of PlaneLayers
  struct VolumeConfig {
    // Center position
    Vector3 position;
    // Lengths in x,y,z
    Vector3 length;
    // Configurations of its layers
    std::vector<LayerConfig> layerCfg;
    // Stored layers
    std::vector<std::shared_ptr<const Layer>> layers;
    // Configurations of confined volumes
    std::vector<VolumeConfig> volumeCfg;
    // Stored confined volumes
    std::vector<std::shared_ptr<TrackingVolume>> trackingVolumes;
    // Name of the volume
    std::string name = "Volume";
    // Material
    std::shared_ptr<const IVolumeMaterial> volumeMaterial = nullptr;
    // Dimension for the binning
    Acts::BinningValue binningDimension = Acts::BinningValue::binX;
  };

  /// @brief This struct stores the configuration of the tracking geometry
  struct Config {
    // Center position
    Vector3 position = Vector3(0., 0., 0.);
    // Length in x,y,z
    Vector3 length = Vector3(0., 0., 0.);
    // Configuration of its volumes
    std::vector<VolumeConfig> volumeCfg = {};
  };

  /// @brief Default constructor without a configuration
  CuboidVolumeBuilder() = default;

  /// @brief Constructor that sets the config
  ///
  /// @param [in] cfg Configuration of the detector
  CuboidVolumeBuilder(Config& cfg) : m_cfg(cfg) {}

  /// @brief Setter of the config
  ///
  /// @param [in] cfg Configuration that is set
  void setConfig(Config& cfg) { m_cfg = cfg; }

  /// @brief This function creates a surface with a given configuration. A
  /// detector element is attached if the template parameter is non-void.
  ///
  /// @param [in] gctx the geometry context for this building
  /// @param [in] cfg Configuration of the surface
  ///
  /// @return Pointer to the created surface
  std::shared_ptr<const Surface> buildSurface(const GeometryContext& gctx,
                                              const SurfaceConfig& cfg) const;

  /// @brief This function creates a layer with a surface encapsulated with a
  /// given configuration. The surface gets a detector element attached if the
  /// template parameter is non-void.
  ///
  /// @param [in] gctx the geometry context for this building
  /// @param [in, out] cfg Configuration of the layer and the surface
  ///
  /// @return Pointer to the created layer
  std::shared_ptr<const Layer> buildLayer(const GeometryContext& gctx,
                                          LayerConfig& cfg) const;

  /// @brief This function creates a TrackingVolume with a configurable number
  /// of layers and surfaces. Each surface gets a detector element attached if
  /// the template parameter is non-void.
  ///
  /// @param [in] gctx the geometry context for this building
  /// @param [in, out] cfg Configuration of the TrackingVolume
  ///
  /// @return Pointer to the created TrackingVolume
  std::shared_ptr<TrackingVolume> buildVolume(const GeometryContext& gctx,
                                              VolumeConfig& cfg) const;

  /// @brief This function evaluates the minimum and maximum of the binning as
  /// given by the configurations of the surfaces and layers. The ordering
  /// depends on the binning value specified in the configuration of the volume.
  ///
  /// @param [in] gctx the geometry context for this building
  /// @param [in] cfg Container with the given surfaces and layers
  ///
  /// @return Pair containing the minimum and maximum along the binning
  /// direction
  std::pair<double, double> binningRange(const GeometryContext& gctx,
                                         const VolumeConfig& cfg) const;

  void sortVolumes(std::vector<std::pair<TrackingVolumePtr, Vector3>>& tapVec,
                   BinningValue bValue) const;

  /// @brief This function builds a world TrackingVolume based on a given
  /// configuration
  ///
  /// @param [in] gctx the geometry context for this building
  ///
  /// @return Pointer to the created TrackingGeometry
  std::shared_ptr<TrackingVolume> trackingVolume(
      const GeometryContext& gctx,
      std::shared_ptr<const TrackingVolume> /*oppositeVolume*/,
      std::shared_ptr<const VolumeBounds> /*outsideBounds*/) const override;

 private:
  /// Configuration of the world volume
  Config m_cfg;
};
}  // namespace Acts
