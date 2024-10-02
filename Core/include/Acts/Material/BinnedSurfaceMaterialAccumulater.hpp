// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Material/AccumulatedSurfaceMaterial.hpp"
#include "Acts/Material/interface/ISurfaceMaterialAccumulater.hpp"
#include "Acts/Utilities/Logger.hpp"

namespace Acts {

/// @brief The binned surface material accumulater
///
/// It consumes the assigned material interactions and then accumulates
/// the material on the surfaces in prepared binned containers for averaging

class BinnedSurfaceMaterialAccumulater final
    : public ISurfaceMaterialAccumulater {
 public:
  /// @brief Nested config struct
  struct Config {
    GeometryContext geoContext;

    /// Correct for empty bins (recommended)
    bool emptyBinCorrection = true;

    /// The surfaces to be used for the accummulation
    std::vector<const Surface*> materialSurfaces = {};
  };

  /// @brief Nested state struct
  struct State final : public ISurfaceMaterialAccumulater::State {
    /// The accumulated material per geometry ID
    std::map<GeometryIdentifier, AccumulatedSurfaceMaterial>
        accumulatedMaterial;
  };

  /// Constructor
  ///
  /// @param cfg the configuration struct
  /// @param mlogger the logger
  BinnedSurfaceMaterialAccumulater(
      const Config& cfg,
      std::unique_ptr<const Logger> mlogger =
          getDefaultLogger("BinnedSurfaceMaterialAccumulater", Logging::INFO));

  /// Factory for creating the state
  std::unique_ptr<ISurfaceMaterialAccumulater::State> createState()
      const override;

  /// @brief Accumulate the material interaction on the surface
  ///
  /// @param state is the state of the accumulater
  /// @param interactions is the material interactions, with assigned surfaces
  /// @param surfacesWithoutAssignment are the surfaces without assignment
  ///
  /// @note this the track average over the binned material
  void accumulate(ISurfaceMaterialAccumulater::State& state,
                  const std::vector<MaterialInteraction>& interactions,
                  const std::vector<IAssignmentFinder::SurfaceAssignment>&
                      surfacesWithoutAssignment) const override;

  /// Finalize the surface material maps
  ///
  /// @param state the state of the accumulator
  ///
  /// @note this does the run average over the (binned) material
  std::map<GeometryIdentifier, std::shared_ptr<const ISurfaceMaterial>>
  finalizeMaterial(ISurfaceMaterialAccumulater::State& state) const override;

 private:
  /// Access method to the logger
  const Logger& logger() const { return *m_logger; }

  /// The configuration
  Config m_cfg;

  /// The logger
  std::unique_ptr<const Logger> m_logger;
};

}  // namespace Acts
