// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/BoundarySurfaceFace.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/ITrackingVolumeHelper.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

namespace Acts {

class Layer;
class TrackingVolume;
class VolumeBounds;
class CylinderVolumeBounds;
class IVolumeMaterial;
class ILayerArrayCreator;
class ITrackingVolumeArrayCreator;

/// @class CylinderVolumeHelper
///
/// The concrete implementation for cylindrical TrackingVolume
/// objects of the ITrackingVolumeCreator interface
///
class CylinderVolumeHelper : public ITrackingVolumeHelper {
 public:
  /// @struct Config
  /// Nested configuration struct for this CylinderVolumeHelper
  struct Config {
    /// a tool for coherent LayerArray creation
    std::shared_ptr<const ILayerArrayCreator> layerArrayCreator = nullptr;
    /// Helper Tool to create TrackingVolume
    std::shared_ptr<const ITrackingVolumeArrayCreator>
        trackingVolumeArrayCreator = nullptr;
    /// thickness of passive layers
    double passiveLayerThickness = 1;
    /// bins in phi for the passive layer
    int passiveLayerPhiBins = 1;
    /// bins in r/z for the passive layer
    int passiveLayerRzBins = 100;
  };

  /// Constructor
  /// @param cvhConfig is the configuration struct for this builder
  /// @param logger logging instance
  CylinderVolumeHelper(const Config& cvhConfig,
                       std::unique_ptr<const Logger> logger = getDefaultLogger(
                           "CylinderVolumeHelper", Logging::INFO));

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
  /// @param bType (optional) BinningType - arbitrary(default) or equidistant
  ///
  /// @return shared pointer to a new TrackingVolume
  MutableTrackingVolumePtr createTrackingVolume(
      const GeometryContext& gctx, const LayerVector& layers,
      std::shared_ptr<const IVolumeMaterial> volumeMaterial,
      std::shared_ptr<VolumeBounds> volumeBounds,
      MutableTrackingVolumeVector mtvVector = {},
      const Transform3& transform = Transform3::Identity(),
      const std::string& volumeName = "UndefinedVolume",
      BinningType bType = arbitrary) const override;

  /// Create a TrackingVolume* from a set of layers and (optional) parameters
  ///
  /// @param gctx is the geometry context for witch the volume is built
  /// @param layers vector of static layers confined by the TrackingVolume
  /// if no bounds or HepTransform is given, they define the size
  /// together with the volume enevlope parameters
  /// @param volumeMaterial material properties for this TrackingVolume
  /// @param mtvVector Vector of confined TrackingVolumes
  /// @param rMin minimum radius
  /// @param rMax maximum radius
  /// @param zMin minimum z
  /// @param zMax maximum z
  /// @param volumeName  volume name to be given
  /// @param bType (optional) BinningType - arbitrary(default) or equidistant
  ///
  /// @return shared pointer to a new TrackingVolume
  MutableTrackingVolumePtr createTrackingVolume(
      const GeometryContext& gctx, const LayerVector& layers,
      MutableTrackingVolumeVector mtvVector,
      std::shared_ptr<const IVolumeMaterial> volumeMaterial, double rMin,
      double rMax, double zMin, double zMax,
      const std::string& volumeName = "UndefinedVolume",
      BinningType bType = arbitrary) const override;

  /// Create a gap volume from dimensions and
  /// @note this TrackingVolume is restricted to Translation only
  ///
  /// @param [in] gctx the geometry context for this building
  /// @param mtvVector Vector of confined TrackingVolumes
  /// @param volumeMaterial dense material properties for this TrackingVolume
  /// @param rMin minimum radius
  /// @param rMax maximum radius
  /// @param zMin minimum z
  /// @param zMax maximum z
  /// @param materialLayers number of material layers (equidistant binning)
  /// @param cylinder type of layers
  /// @param volumeName  volume name to be given
  ///
  /// @return shared pointer to a new TrackingVolume
  MutableTrackingVolumePtr createGapTrackingVolume(
      const GeometryContext& gctx, MutableTrackingVolumeVector& mtvVector,
      std::shared_ptr<const IVolumeMaterial> volumeMaterial, double rMin,
      double rMax, double zMin, double zMax, unsigned int materialLayers,
      bool cylinder = true,
      const std::string& volumeName = "UndefinedVolume") const override;

  /// Create a gap volume from dimensions and
  ///
  /// @param [in] gctx the geometry context for this building
  /// @param mtvVector Vector of confined TrackingVolumes
  /// @param volumeMaterial dense material properties for this TrackingVolume
  /// @param rMin minimum radius
  /// @param rMax maximum radius
  /// @param zMin minimum z
  /// @param zMax maximum z
  /// @param layerPositions custom layer positions
  /// @param cylinder type of layers
  /// @param volumeName  : volume name to be given
  /// @param bType (optional) BinningType - arbitrary(default) or equidistant
  ///
  /// @return shared pointer to a new TrackingVolume
  MutableTrackingVolumePtr createGapTrackingVolume(
      const GeometryContext& gctx, MutableTrackingVolumeVector& mtvVector,
      std::shared_ptr<const IVolumeMaterial> volumeMaterial, double rMin,
      double rMax, double zMin, double zMax,
      const std::vector<double>& layerPositions, bool cylinder = true,
      const std::string& volumeName = "UndefinedVolume",
      BinningType bType = arbitrary) const override;

  /// Create a container volumes from sub volumes, input volumes are ordered in
  /// R or Z by convention
  ///
  /// @param [in] gctx the geometry context for this building
  /// @param volumes the volumes to be contained
  ///
  ///
  /// @return shared pointer to a new TrackingVolume
  MutableTrackingVolumePtr createContainerTrackingVolume(
      const GeometryContext& gctx,
      const TrackingVolumeVector& volumes) const override;

  /// Set configuration method
  ///
  /// @param cvhConfig is the configuration struct assigned
  void setConfiguration(const Config& cvhConfig);

  /// Get configuration method
  Config getConfiguration() const;

  /// Set logging instance
  ///
  /// @param newLogger is the logger instance to be set
  void setLogger(std::unique_ptr<const Logger> newLogger);

 protected:
  /// Configuration object
  Config m_cfg;

 private:
  /// Private access method to the logging instance
  const Logger& logger() const { return *m_logger; }

  /// the looging instance
  std::unique_ptr<const Logger> m_logger;

  /// Private method - it estimates the CylinderBounds and Translation
  /// of layers, these are checked against the layer positions/dimensions.
  ///
  /// @param gctx [in] the geometry context of this build
  /// @param layers the layers for which the dimensions are checked
  /// @param cylinderVolumeBounds the cylinder volume bounds needed for wrapping
  /// @param transform a transformation of the layers, volume
  /// @param rMinClean the smallest radius given by layers
  /// @param rMaxClean the maximal radius given by layers
  /// @param zMinClean the smallest z extend given by layers
  /// @param zMaxClean the maximal z extend given by layers
  /// @param bValue the binning value in which the binning works
  /// @param bType is the type of binning: equidistant, arbitrary
  bool estimateAndCheckDimension(
      const GeometryContext& gctx, const LayerVector& layers,
      std::shared_ptr<CylinderVolumeBounds>& cylinderVolumeBounds,
      const Transform3& transform, double& rMinClean, double& rMaxClean,
      double& zMinClean, double& zMaxClean, BinningValue& bValue,
      BinningType bType = arbitrary) const;

  /// Private method - interglue all volumes contained by a TrackingVolume
  /// and set the outside glue volumes in the descriptor
  ///
  /// @param gctx [in] the geometry context of this build
  /// @param tVolume the tracking volume that is glued together
  /// @param rBinned a boolean indicating if it is binned in r
  /// @param rMin the minimum radius of the volume
  /// @param rGlueMin the minimum glue radius (@todo check and document)
  /// @param rMax the maximum radius of the volume
  /// @param zMin the minimum z extend of the volume
  /// @param zMax the maximum z extend of the volume
  bool interGlueTrackingVolume(const GeometryContext& gctx,
                               const MutableTrackingVolumePtr& tVolume,
                               bool rBinned, double rMin, double rGlueMin,
                               double rMax, double zMin, double zMax) const;

  /// Private method - glue volume to the other
  ///
  /// @param gctx [in] the geometry context of this build
  /// @param tvolOne is the first volume in the glue process
  /// @param faceOne is the first boundary face of the glue process
  /// @param tvolTwo is the second volume in the glue process
  /// @param faceTwo is the second boundary face of the glue process
  /// @param rMin the minimum radius of the volume
  /// @param rGlueMin the minimum glue radius (@todo check and document)
  /// @param rMax the maximum radius of the volume
  /// @param zMin the minimum z extend of the volume
  /// @param zMax the maximum z extend of the volume
  void glueTrackingVolumes(const GeometryContext& gctx,
                           const MutableTrackingVolumePtr& tvolOne,
                           BoundarySurfaceFace faceOne,
                           const MutableTrackingVolumePtr& tvolTwo,
                           BoundarySurfaceFace faceTwo, double rMin,
                           double rGlueMin, double rMax, double zMin,
                           double zMax) const;

  /// Private method - helper method not to duplicate code
  ///
  /// @param tvol is the volume to which faces are added
  /// @param glueFace the boundary surface to which faces are added
  /// @param vols are the voluems which are added
  void addFaceVolumes(const MutableTrackingVolumePtr& tvol,
                      BoundarySurfaceFace glueFace,
                      TrackingVolumeVector& vols) const;

  /// Private method - helper method to save some code
  ///
  /// @param z is the z position of the layer (@todo use Transform)
  /// @param r is the radius of the layer
  /// @param halflengthZ is the half lengthz in z of the cylinder
  /// @param thickness is the thickness of the cylinder
  /// @param binsPhi are the bins for the material in phi
  /// @param binsZ are the bins for the material in z
  ///
  /// @return shared pointer to newly created cylinder layer
  LayerPtr createCylinderLayer(double z, double r, double halflengthZ,
                               double thickness, int binsPhi, int binsZ) const;

  /// Private method - helper method to save some code
  ///
  /// @param z is the z position of the layer (@todo use Transform)
  /// @param rMin is the minimum radius of the layer
  /// @param rMax is the maximal radius of the layer
  /// @param thickness is the thickness of the cylinder
  /// @param binsPhi are the bins for the material in phi
  /// @param binsR are the bins for the material in R
  ///
  /// @return shared pointer to newly created cylinder layer
  LayerPtr createDiscLayer(double z, double rMin, double rMax, double thickness,
                           int binsPhi, int binsR) const;
};

inline CylinderVolumeHelper::Config CylinderVolumeHelper::getConfiguration()
    const {
  return m_cfg;
}
}  // namespace Acts
