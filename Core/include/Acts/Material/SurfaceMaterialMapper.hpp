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

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Material/AccumulatedSurfaceMaterial.hpp"
#include "Acts/Material/ISurfaceMaterial.hpp"
#include "Acts/Material/MaterialInteraction.hpp"
#include "Acts/Propagator/MaterialInteractor.hpp"
#include "Acts/Propagator/Navigator.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Propagator/StraightLineStepper.hpp"
#include "Acts/Propagator/SurfaceCollector.hpp"
#include "Acts/Propagator/VolumeCollector.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <array>
#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace Acts {

class IVolumeMaterial;
class ISurfaceMaterial;
class TrackingGeometry;
struct MaterialInteraction;

/// @brief selector for finding surface
struct MaterialSurface {
  bool operator()(const Surface& sf) const {
    return (sf.surfaceMaterial() != nullptr);
  }
};

/// @brief selector for finding volume
struct MaterialVolume {
  bool operator()(const TrackingVolume& vf) const {
    return (vf.volumeMaterial() != nullptr);
  }
};

/// @brief SurfaceMaterialMapper
///
/// This is the main feature tool to map material information
/// from a 3D geometry onto the TrackingGeometry with its surface
/// material description.
///
/// The process runs as such:
///
///  1) TrackingGeometry is parsed and for each Surface with
///     ProtoSurfaceMaterial a local store is initialized
///     the identification is done hereby through the
///     Surface::GeometryIdentifier
///
///  2) A Cache is generated that is used to keep the filling thread local,
///     the filling is protected with std::mutex
///
///  3) A number of N material tracks is read in, each track has :
///       origin, direction, material steps < position, step length, x0, l0, a,
///       z, rho >
///
///       for each track:
///          surfaces along the origin/direction path are collected
///          the closest material steps are assigned
///
///  4) Each 'hit' bin per event is counted and averaged at the end of the run
///
class SurfaceMaterialMapper {
 public:
  using StraightLinePropagator = Propagator<StraightLineStepper, Navigator>;

  /// @struct Config
  ///
  /// Nested Configuration struct for the material mapper
  struct Config {
    /// Mapping range
    std::array<double, 2> etaRange = {{-6., 6.}};
    /// Correct for empty bins (recommended)
    bool emptyBinCorrection = true;
    /// Mapping output to debug stream
    bool mapperDebugOutput = false;
    /// Compute the variance of each material slab (only if using an input map)
    bool computeVariance = false;
  };

  /// @struct State
  ///
  /// Nested State struct which is used for the mapping prococess
  struct State {
    /// @param [in] gctx The geometry context to use
    /// @param [in] mctx The magnetic field context to use
    State(const GeometryContext& gctx, const MagneticFieldContext& mctx)
        : geoContext(gctx), magFieldContext(mctx) {}

    /// The accumulated material per geometry ID
    std::map<GeometryIdentifier, AccumulatedSurfaceMaterial>
        accumulatedMaterial;

    /// The created surface material from it
    std::map<GeometryIdentifier, std::unique_ptr<const ISurfaceMaterial>>
        surfaceMaterial;

    /// The surface material of the input tracking geometry
    std::map<GeometryIdentifier, std::shared_ptr<const ISurfaceMaterial>>
        inputSurfaceMaterial;

    /// The volume material of the input tracking geometry
    std::map<GeometryIdentifier, std::shared_ptr<const IVolumeMaterial>>
        volumeMaterial;

    /// Reference to the geometry context for the mapping
    std::reference_wrapper<const GeometryContext> geoContext;

    /// Reference to the magnetic field context
    std::reference_wrapper<const MagneticFieldContext> magFieldContext;
  };

  /// Delete the Default constructor
  SurfaceMaterialMapper() = delete;

  /// Constructor with config object
  ///
  /// @param cfg Configuration struct
  /// @param propagator The straight line propagator
  /// @param slogger The logger
  SurfaceMaterialMapper(const Config& cfg, StraightLinePropagator propagator,
                        std::unique_ptr<const Logger> slogger =
                            getDefaultLogger("SurfaceMaterialMapper",
                                             Logging::INFO));

  /// @brief helper method that creates the cache for the mapping
  ///
  /// @param [in] gctx The geometry context to use
  /// @param [in] mctx The magnetic field context to use
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
  /// the AccumulatedSurface material class to a surface material
  /// class type
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

  /// Loop through all the material interactions and add them to the
  /// associated surface
  ///
  /// @param mState The current state map
  /// @param mTrack The material track to be mapped
  ///
  void mapInteraction(State& mState, RecordedMaterialTrack& mTrack) const;

  /// Loop through all the material interactions and add them to the
  /// associated surface
  ///
  /// @param mState The current state map
  /// @param rMaterial Vector of all the material interactions that will be mapped
  ///
  /// @note The material interactions are assumed to have an associated surface ID
  void mapSurfaceInteraction(State& mState,
                             std::vector<MaterialInteraction>& rMaterial) const;

 private:
  /// @brief finds all surfaces with ProtoSurfaceMaterial of a volume
  ///
  /// @param mState The state to be filled
  /// @param tVolume is current TrackingVolume
  void resolveMaterialSurfaces(State& mState,
                               const TrackingVolume& tVolume) const;

  /// @brief check and insert
  ///
  /// @param mState is the map to be filled
  /// @param surface is the surface to be checked for a Proxy
  void checkAndInsert(State& mState, const Surface& surface) const;

  /// @brief check and insert
  ///
  /// @param mState is the map to be filled
  /// @param tVolume is the volume collect from
  void collectMaterialVolumes(State& mState,
                              const TrackingVolume& tVolume) const;

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
