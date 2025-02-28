// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/Volume.hpp"
#include "Acts/Geometry/VolumeAttachmentStrategy.hpp"
#include "Acts/Geometry/VolumeResizeStrategy.hpp"
#include "Acts/Utilities/AxisDefinitions.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <cstddef>
#include <vector>

namespace Acts {

/// @class CuboidVolumeStack
/// This class implements a x-. y-. z-aligned stack
/// of cuboid volumes with synchronized bounds.
/// Externally, it presents as a single volume.
/// On construction, the input volumes are modified so that
/// they are connected in x, y, z and have synchronized bounds.
/// The way this is done can be configured using an *attachment*
/// and a *resize* strategy. Depending on the configuration,
/// the input volumes are either extended or gap volumes are created.
///
/// @note The size adjustment convention is that volumes are never shrunk
class CuboidVolumeStack : public Volume {
 public:
  /// Constructor from a vector of volumes and direction
  /// @param volumes is the vector of volumes
  /// @param direction is the axis direction
  /// @param strategy is the attachment strategy
  /// @param resizeStrategy is the resize strategy
  /// @note @p resizeStrategy only affects resizing along
  ///       @p direction. Resizing in the other direction
  ///       is always delegated to the child volumes,
  ///       which might in turn be @c CuboidVolumeStack
  /// @param logger is the logger
  /// @pre The volumes need to have a common coordinate
  ///      system relative to @p direction. I.e. they need
  ///      to be aligned in @c z and cannot have a rotation
  ///      in @c x or @c y.
  /// @pre The volumes all need to have @c CuboidVolumeBounds
  /// @note Preconditions are checked on construction
  CuboidVolumeStack(
      std::vector<Volume*>& volumes, AxisDirection direction,
      VolumeAttachmentStrategy strategy = VolumeAttachmentStrategy::Midpoint,
      VolumeResizeStrategy resizeStrategy = VolumeResizeStrategy::Expand,
      const Logger& logger = Acts::getDummyLogger());

  /// Update the volume bounds and transform. This
  /// will update the bounds of all volumes in the stack
  /// to accommodate the new bounds and optionally create
  /// gap volumes according to the resize strategy set during
  /// construction.
  /// @param volbounds is the new bounds
  /// @param transform is the new transform
  /// @param logger is the logger
  /// @pre The volume bounds need to be of type
  ///      @c CuboidVolumeBounds.
  void update(std::shared_ptr<VolumeBounds> volbounds,
              std::optional<Transform3> transform = std::nullopt,
              const Logger& logger = getDummyLogger()) override;

  /// Access the gap volume that were created during attachment or resizing.
  /// @return the vector of gap volumes
  const std::vector<std::shared_ptr<Volume>>& gaps() const;

  /// Convert axis direction to an array index according to
  /// stack convention. For example, AxisX --> 0
  /// @param direction is the axis direction to convert
  static std::size_t axisToIndex(AxisDirection direction);

  /// Get axis directions orthogonal to the given one according
  /// to stack convention. For example AxisX --> <AxisY, AxisZ>
  /// @param direction is the axis direction to find the orthogonal for
  static std::pair<AxisDirection, AxisDirection> getOrthogonalAxes(
      AxisDirection direction);

 private:
  /// Helper to get the first volume in the input, and throw an exception if
  /// there is not one.
  /// @param volumes is the vector of volumes
  /// @return the first volume
  static Volume& initialVolume(const std::vector<Volume*>& volumes);

  /// Helper function called during construction that performs the
  /// internal attachment and produces the overall outer volume bounds.
  /// @param strategy is the attachment strategy
  /// @param logger is the logger
  void initializeOuterVolume(VolumeAttachmentStrategy strategy,
                             const Logger& logger);

  struct VolumeTuple;

  /// Helper function to pretty print the internal volume representation
  /// @param volumes is the vector of volumes
  /// @param logger is the logger
  /// @param lvl is the logging level
  static void printVolumeSequence(const std::vector<VolumeTuple>& volumes,
                                  const Logger& logger,
                                  Acts::Logging::Level lvl);

  /// Helper function that prints output helping in debugging overlaps
  /// @param a is the first volume
  /// @param b is the second volume
  /// @param logger is the logger
  void overlapPrint(const VolumeTuple& a, const VolumeTuple& b,
                    const Logger& logger);

  /// Helper function that checks if volumes are properly aligned
  /// for attachment.
  /// @param volumes is the vector of volumes
  /// @param logger is the logger
  void checkVolumeAlignment(const std::vector<VolumeTuple>& volumes,
                            const Logger& logger) const;

  /// Helper function that checks overlaps and attaches along the stacking
  /// direction
  /// @param volumes is the vector of volumes
  /// @param strategy is the attachment strategy
  /// @param logger is the logger
  /// @return vector of gap volumes. Can be empty if none were created.
  std::vector<VolumeTuple> checkOverlapAndAttach(
      std::vector<VolumeTuple>& volumes, VolumeAttachmentStrategy strategy,
      const Logger& logger);

  /// Helper function to synchronize the bounds of the volumes
  /// @param volumes is the vector of volumes
  /// @param logger is the logger
  /// @return tuple of the minimum and maximum radii
  std::pair<double, double> synchronizeBounds(std::vector<VolumeTuple>& volumes,
                                              const Logger& logger);

  /// Helper function to create a gap volume with given bounds and register it.
  /// @param transform is the transform of the gap volume
  /// @param bounds is the bounds of the gap volume
  /// @return the shared pointer to the gap volume
  std::shared_ptr<Volume> addGapVolume(
      const Transform3& transform, const std::shared_ptr<VolumeBounds>& bounds);

  /// Merging direction of the stack
  /// in local group coordinates
  AxisDirection m_direction{};

  /// Directions orthogonal to the
  /// merging direction of the stack
  /// in local group coordinates
  AxisDirection m_dirOrth1{};
  AxisDirection m_dirOrth2{};

  VolumeResizeStrategy m_resizeStrategy{};
  Transform3 m_groupTransform{};
  std::vector<std::shared_ptr<Volume>> m_gaps{};
  std::vector<Volume*>& m_volumes;
};

}  // namespace Acts
