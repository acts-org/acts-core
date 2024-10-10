// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Material/interface/IAssignmentFinder.hpp"

#include <utility>
#include <vector>

namespace Acts {

/// @class IntersectionMaterialAssigner
///
/// A purely intersection based material assigner on a trial and error basis.
/// This is to be used if the navigation of the propagator is not available
/// or not reliable, or simply for cross-checking the results.
///
/// @note that differently to the PropagatorMaterialAssigner, this assigner
/// needs to be preconditioned with the surfaces and volumes that are tested
/// for candidate inclusion.
///
/// In a large-n material interaction scenario, this is not the most efficient
class IntersectionMaterialAssigner final : public IAssignmentFinder {
 public:
  /// @brief Nested configuration struct
  struct Config {
    /// @brief  The surfaces to be tested
    std::vector<const Surface*> surfaces;
    /// @brief  The volumes to be tested: TrackingVolume
    std::vector<const TrackingVolume*> trackingVolumes;
    /// @brief  The volumes to be tested: DetectorVolume
    std::vector<const Experimental::DetectorVolume*> detectorVolumes;
  };

  /// @brief Construct with the configuration
  ///
  /// @param cfg is the configuration struct
  /// @param mlogger is the logger
  IntersectionMaterialAssigner(
      const Config& cfg,
      std::unique_ptr<const Logger> mlogger =
          getDefaultLogger("IntersectionMaterialAssigner", Logging::INFO))
      : m_cfg(cfg), m_logger(std::move(mlogger)) {}

  /// @brief Method for generating assignment candidates for the
  /// material interaction assignment to surfaces or volumes
  ///
  /// @param gctx is the geometry context
  /// @param mctx is the magnetic field context
  /// @param position is the position of the initial ray
  /// @param direction is the direction of initial ray
  ///
  /// @return a vector of Surface Assignments and Volume Assignments
  std::pair<std::vector<IAssignmentFinder::SurfaceAssignment>,
            std::vector<IAssignmentFinder::VolumeAssignment>>
  assignmentCandidates(const GeometryContext& gctx,
                       const MagneticFieldContext& mctx,
                       const Vector3& position,
                       const Vector3& direction) const final;

 private:
  /// Access method to the logger
  const Logger& logger() const { return *m_logger; }

  /// The configuration
  Config m_cfg;

  /// The logger
  std::unique_ptr<const Logger> m_logger;
};

}  // namespace Acts
