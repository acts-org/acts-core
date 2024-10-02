// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/ProtoDetector.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Geometry/ITrackingGeometryBuilder.hpp"
#include "Acts/Geometry/TrackingGeometryBuilder.hpp"
#include "Acts/Utilities/KDTree.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace Acts {

class TrackingGeometry;
class Layer;
class LayerCreator;
class Surface;
class ITrackingVolumeHelper;
class TrackingVolume;

/// A Tracking Geometry builder restricted to cylindrical geometries
///
/// It takes some helper tools and a vector of surface objects,
/// together with a ProtoDetector description that is used to query a
/// KDTree for contained surfaces in structures defined by the proto
/// volume.
class KDTreeTrackingGeometryBuilder : public ITrackingGeometryBuilder {
 public:
  /// Nested Configuration for this TrackingGeometryBuilder
  struct Config {
    /// The tracking volume helper for detector construction
    std::shared_ptr<const ITrackingVolumeHelper> trackingVolumeHelper = nullptr;
    /// The layer creator - for sensitives
    std::shared_ptr<const LayerCreator> layerCreator = nullptr;
    /// The created surfaces
    std::vector<std::shared_ptr<Surface>> surfaces = {};
    /// The proto tracking geometry description
    ProtoDetector protoDetector;
    /// Optional geometry identifier hook to be used during closure
    std::shared_ptr<const GeometryIdentifierHook> geometryIdentifierHook =
        std::make_shared<GeometryIdentifierHook>();
    /// For screen output
    std::string hierarchyIndent = "  ";
  };

  using SurfaceKDT =
      KDTree<2u, std::shared_ptr<Surface>, ActsScalar, std::array, 100>;

  /// Constructor
  ///
  /// @param [in] cfg is the configuration struct for this builder
  /// @param [in] logger logging instance
  KDTreeTrackingGeometryBuilder(
      const Config& cfg,
      std::unique_ptr<const Logger> logger =
          getDefaultLogger("KDTreeTrackingGeometryBuilder", Logging::INFO));

  /// TrackingGeometry Interface method
  ///
  /// @param gctx geometry context of that building call
  ///
  /// @return a unique pointer to a TrackingGeometry
  std::unique_ptr<const TrackingGeometry> trackingGeometry(
      const GeometryContext& gctx) const final;

 private:
  /// Configuration member
  Config m_cfg;

  /// Private access method to the logger
  const Logger& logger() const { return *m_logger; }

  /// the logging instance
  std::unique_ptr<const Logger> m_logger;

  /// Private construction cache
  struct Cache {
    std::size_t surfaceCounter = 0;
  };

  /// Translate a proto tracking volume into a Acts::TrackingVolume
  ///
  /// @param cCache is a cache used to extract the built detector elements
  /// @param gctx is the current geometry context at building
  /// @param kdt is the pre-filled kdt tree for the surface query
  /// @param ptVolume the proto volume to be translated
  /// @param indent is a screen output indentation
  ///
  /// @return a new tracking volume
  std::shared_ptr<TrackingVolume> translateVolume(
      Cache& cCache, const GeometryContext& gctx, const SurfaceKDT& kdt,
      const ProtoVolume& ptVolume, const std::string& indent = "") const;

  /// Translate a layer volume
  ///
  /// @param cCache is a cache used to extract the built detector elements
  /// @param gctx is the current geometry context at building
  /// @param kdt is the pre-filled kdt tree for the surface query
  /// @param plVolume the proto volume representation a layer to be translated
  /// @param indent is a screen output indentation
  ///
  /// @return a new tracking volume
  std::shared_ptr<const Layer> translateLayer(
      Cache& cCache, const GeometryContext& gctx, const SurfaceKDT& kdt,
      const ProtoVolume& plVolume, const std::string& indent = "") const;
};

}  // namespace Acts
