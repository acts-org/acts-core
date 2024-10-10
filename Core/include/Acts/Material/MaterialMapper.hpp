// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Material/MaterialInteraction.hpp"
#include "Acts/Material/MaterialInteractionAssignment.hpp"
#include "Acts/Material/interface/IAssignmentFinder.hpp"
#include "Acts/Material/interface/ISurfaceMaterialAccumulater.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <memory>
#include <utility>
#include <vector>

namespace Acts {
/// @brief material mapping procedure
class MaterialMapper {
 public:
  /// @brief The material maps
  using SurfaceMaterialMaps =
      std::map<GeometryIdentifier, std::shared_ptr<const ISurfaceMaterial>>;
  using VolumeMaterialMaps =
      std::map<GeometryIdentifier, std::shared_ptr<const IVolumeMaterial>>;
  using DetectorMaterialMaps =
      std::pair<SurfaceMaterialMaps, VolumeMaterialMaps>;

  /// @brief nested configuration struct
  struct Config {
    // The assignment finder
    std::shared_ptr<const IAssignmentFinder> assignmentFinder = nullptr;
    // The material accumulater for surfaces
    std::shared_ptr<const ISurfaceMaterialAccumulater>
        surfaceMaterialAccumulater = nullptr;
  };

  /// @brief nested state struct
  ///
  /// It holds the states of the sub structs
  struct State {
    std::unique_ptr<ISurfaceMaterialAccumulater::State>
        surfaceMaterialAccumulaterState;
  };

  /// @brief nested options struct
  /// holds some options for the delegated calls
  struct Options {
    // The assignment options (including vetos and re-assignments)
    MaterialInteractionAssignment::Options assignmentOptions;
  };

  /// @brief MaterialMapper constructor
  ///
  /// @param cfg the configuration struct
  /// @param mlogger the logger instance
  MaterialMapper(const Config& cfg,
                 std::unique_ptr<const Logger> mlogger = getDefaultLogger(
                     "BinnedSurfaceMaterialAccumulater", Logging::INFO));

  /// @brief Factory for creating the state
  std::unique_ptr<State> createState() const;

  /// @brief Map the material interactions to the surfaces
  ///
  /// @param state the state object holding the sub states
  /// @param gctx the geometry context
  /// @param mctx the magnetic field context
  /// @param rmTrack the recorded material track
  /// @param options the call options (see above)
  ///
  /// @return the mapped and unmapped material tracks
  std::pair<RecordedMaterialTrack, RecordedMaterialTrack> mapMaterial(
      State& state, const GeometryContext& gctx,
      const MagneticFieldContext& mctx, const RecordedMaterialTrack& rmTrack,
      const Options& options = Options{}) const;

  /// Finalize the maps
  DetectorMaterialMaps finalizeMaps(const State& state) const;

 private:
  /// Access method to the logger
  const Logger& logger() const { return *m_logger; }

  /// The configuration
  Config m_cfg;

  /// The logger
  std::unique_ptr<const Logger> m_logger;
};

}  // namespace Acts
