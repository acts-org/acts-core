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
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Material/MaterialInteraction.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <memory>

namespace Acts {

class IAssignmentFinder;

/// @brief The material validater is a tool that allows to record the material
/// seen by a ray through a set of material surfaces.
///
/// It does uses a material assigner that can be either done using the
/// propagator or a more sinmple trial and error intersection;
class MaterialValidater {
 public:
  /// Nested configuration struct
  struct Config {
    std::shared_ptr<const IAssignmentFinder> materialAssigner = nullptr;
  };

  /// Constructor
  /// @param cfg The configuration struct carrying the used tools
  /// @param mlogger The logging object
  MaterialValidater(const Config& cfg,
                    std::unique_ptr<const Logger> mlogger =
                        getDefaultLogger("MaterialValidater", Logging::INFO));

  /// Method to record the material along a ray
  /// @param gctx the geometry context
  /// @param mctx the magnetic field context
  /// @param position the starting position of the ray
  /// @param direction the direction of the ray (unit vector)
  ///
  /// @return a rerorded material track
  RecordedMaterialTrack recordMaterial(const GeometryContext& gctx,
                                       const MagneticFieldContext& mctx,
                                       const Vector3& position,
                                       const Vector3& direction) const;

 private:
  /// Access method to the logger
  const Logger& logger() const { return *m_logger; }

  /// The configuration
  Config m_cfg;

  // The logger
  std::unique_ptr<const Logger> m_logger;
};

}  // namespace Acts
