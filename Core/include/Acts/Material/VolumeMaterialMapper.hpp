// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

// Workaround for building on clang+libstdc++
#include "Acts/Utilities/detail/ReferenceWrapperAnyCompat.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Material/AccumulatedVolumeMaterial.hpp"
#include "Acts/Material/MaterialGridHelper.hpp"
#include "Acts/Material/MaterialInteraction.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Propagator/MaterialInteractor.hpp"
#include "Acts/Propagator/Navigator.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Propagator/StraightLineStepper.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <functional>
#include <map>
#include <memory>
#include <utility>
#include <vector>

namespace Acts {

class ISurfaceMaterial;
class IVolumeMaterial;
class TrackingGeometry;

//
/// @brief VolumeMaterialMapper
///
/// This is the main feature tool to map material information
/// from a 3D geometry onto the TrackingGeometry with its surface
/// material description.
///
/// The process runs as such:
///
///  1) TrackingGeometry is parsed and for each Volume with
///     ProtoVolumeMaterial a local store is initialized
///     the identification is done hereby through the Volume::GeometryIdentifier
///
///  2) A number of N material tracks is read in, each track has :
///       origin, direction, material steps (< position, step length, x0, l0, a,
///       z, rho >, thichness)
///
///       for each track:
///          volume along the origin/direction path are collected.
///          the step are then associated to volume inside which they are.
///          Additional step are created along the track direction.
///
///  3) Each 'hit' bin per event is counted and averaged at the end of the run

class VolumeMaterialMapper {
 public:
  using StraightLinePropagator = Propagator<StraightLineStepper, Navigator>;

  /// @struct Config
  ///
  /// Nested Configuration struct for the material mapper
  struct Config {
    /// Size of the step for the step extrapolation
    float mappingStep = 1.;
  };

  /// @struct State
  ///
  /// Nested State struct which is used for the mapping prococess
  struct State {
    /// Constructor of the State with contexts
    State(const GeometryContext& gctx, const MagneticFieldContext& mctx)
        : geoContext(gctx), magFieldContext(mctx) {}

    /// The recorded material per geometry ID
    std::map<const GeometryIdentifier, Acts::AccumulatedVolumeMaterial>
        homogeneousGrid;

    /// The recorded 2D transform associated the grid for each geometry ID
    std::map<const GeometryIdentifier,
             std::function<Acts::Vector2(Acts::Vector3)>>
        transform2D;

    /// The 2D material grid for each geometry ID
    std::map<const GeometryIdentifier, Grid2D> grid2D;

    /// The recorded 3D transform associated the material grid for each geometry
    /// ID
    std::map<const GeometryIdentifier,
             std::function<Acts::Vector3(Acts::Vector3)>>
        transform3D;

    /// The 3D material grid for each geometry ID
    std::map<const GeometryIdentifier, Grid3D> grid3D;

    /// The binning for each geometry ID
    std::map<const GeometryIdentifier, BinUtility> materialBin;

    /// The surface material of the input tracking geometry
    std::map<GeometryIdentifier, std::shared_ptr<const ISurfaceMaterial>>
        surfaceMaterial;

    /// The created volume material from it
    std::map<GeometryIdentifier, std::unique_ptr<const IVolumeMaterial>>
        volumeMaterial;

    /// Reference to the geometry context for the mapping
    std::reference_wrapper<const GeometryContext> geoContext;

    /// Reference to the magnetic field context
    std::reference_wrapper<const MagneticFieldContext> magFieldContext;
  };

  /// Delete the Default constructor
  VolumeMaterialMapper() = delete;

  /// Constructor with config object
  ///
  /// @param cfg Configuration struct
  /// @param propagator The straight line propagator
  /// @param slogger The logger
  VolumeMaterialMapper(const Config& cfg, StraightLinePropagator propagator,
                       std::unique_ptr<const Logger> slogger = getDefaultLogger(
                           "VolumeMaterialMapper", Logging::INFO));

  /// @brief helper method that creates the cache for the mapping
  ///
  /// @param[in] gctx The geometry context to use
  /// @param[in] mctx The magnetic field context to use
  /// @param[in] tGeometry The geometry which should be mapped
  ///
  /// This method takes a TrackingGeometry,
  /// finds all surfaces with material proxis
  /// and returns you a Cache object tO be used
  State createState(const GeometryContext& gctx,
                    const MagneticFieldContext& mctx,
                    const TrackingGeometry& tGeometry) const;

  /// @brief Method to finalize the maps
  ///
  /// It calls the final run averaging and then transforms
  /// the Homogeneous material into HomogeneousVolumeMaterial and
  /// the 2D and 3D grid into a InterpolatedMaterialMap
  ///
  /// @param mState
  void finalizeMaps(State& mState) const;

  /// Process/map a single track
  ///
  /// @param mState The current state map
  /// @param mTrack The material track to be mapped
  ///
  /// @note the RecordedMaterialSlab of the track are assumed
  /// to be ordered from the starting position along the starting direction
  void mapMaterialTrack(State& mState, RecordedMaterialTrack& mTrack) const;

 private:
  /// selector for finding surface
  struct BoundSurfaceSelector {
    bool operator()(const Surface& sf) const {
      return (sf.geometryId().boundary() != 0);
    }
  };

  /// selector for finding
  struct MaterialVolumeSelector {
    bool operator()(const TrackingVolume& vf) const {
      return (vf.volumeMaterial() != nullptr);
    }
  };

  /// @brief finds all surfaces with ProtoVolumeMaterial of a volume
  ///
  /// @param mState The state to be filled
  /// @param tVolume is current TrackingVolume
  void resolveMaterialVolume(State& mState,
                             const TrackingVolume& tVolume) const;

  /// @brief check and insert
  ///
  /// @param mState is the map to be filled
  /// @param volume is the surface to be checked for a Proxy
  void checkAndInsert(State& mState, const TrackingVolume& volume) const;

  /// @brief check and insert
  ///
  /// @param mState is the map to be filled
  /// @param tVolume is the surface to collect from
  void collectMaterialSurfaces(State& mState,
                               const TrackingVolume& tVolume) const;

  /// Create extra material point for the mapping and add them to the grid
  ///
  /// @param mState The state to be filled
  /// @param currentBinning a pair containing the current geometry ID and the current binning
  /// @param properties material properties of the original hit
  /// @param position position of the original hit
  /// @param direction direction of the track
  void createExtraHits(
      State& mState,
      std::pair<const GeometryIdentifier, BinUtility>& currentBinning,
      Acts::MaterialSlab properties, const Vector3& position,
      Vector3 direction) const;

  /// Standard logger method
  const Logger& logger() const { return *m_logger; }

  /// The configuration object
  Config m_cfg;

  /// The straight line propagator
  StraightLinePropagator m_propagator;

  /// The logging instance
  std::unique_ptr<const Logger> m_logger;
};

}  // namespace Acts
