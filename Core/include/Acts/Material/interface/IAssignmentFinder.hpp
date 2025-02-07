// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Material/MaterialInteraction.hpp"

#include <tuple>
#include <utility>
#include <vector>

namespace Acts {

/// @brief Interface for the material mapping that seeks the possible
/// assignment candidates for the material interactiosn
class IAssignmentFinder {
 public:
  /// Virtual destructor
  virtual ~IAssignmentFinder() = default;

  /// @brief SurfaceAssignment is a surface, a position and a direction
  struct SurfaceAssignment {
    /// The surface to which the material interaction is assigned to
    const Surface* surface = nullptr;
    /// Position of the assignment
    Vector3 position = Vector3::Zero();
    // Direction of the assignment
    Vector3 direction = Vector3::Zero();
  };

  /// @brief VolumeAssignment is a volume and a entry and exit of the volume
  struct VolumeAssignment {
    /// The volume to which the material interaction is assigned to
    InteractionVolume volume{};
    /// Entry point of the volume
    Vector3 entry = Vector3::Zero();
    /// Exit point of the volume
    Vector3 exit = Vector3::Zero();
  };

  /// @brief Interface method for generating assignment candidates for the
  /// material interaction assignment to surfaces or volumes
  ///
  /// @param gctx is the geometry context
  /// @param mctx is the magnetic field context
  /// @param position is the position of the initial ray
  /// @param direction is the direction of initial ray
  ///
  /// @return a vector of Surface Assignments and Volume Assignments
  virtual std::pair<std::vector<SurfaceAssignment>,
                    std::vector<VolumeAssignment>>
  assignmentCandidates(const GeometryContext& gctx,
                       const MagneticFieldContext& mctx,
                       const Vector3& position,
                       const Vector3& direction) const = 0;
};

}  // namespace Acts
