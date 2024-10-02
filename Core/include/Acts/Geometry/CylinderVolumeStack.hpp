// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/Volume.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <vector>

namespace Acts {

/// @class CylinderVolumeStack
/// This class implements a z-aligned or r-aligned stack
/// of cylinder volumes with synchronized bounds.
/// Externally, it presents as a single volume.
/// On construction, the input volumes are modified so that
/// they are connected in z and r and have synchronized bounds.
/// The way this is done can be configured using an *attachment*
/// and a *resize* strategy. Depending on the configuration,
/// the input volumes are either extended or gap volumes are created.
///
/// @note The volumes are never shrunk, because this would potentially
///       result in overlaps of the resulting volumes bounds.
class CylinderVolumeStack : public Volume {
 public:
  /// The attachment strategy defines how the volumes are attached
  /// Attachment always happens pair-wise
  enum class AttachmentStrategy {
    /// Given two volumes, the *left* one, i.e. the one with the lower **local**
    /// z or r value is extended
    First,
    /// Given two volumes, the *right* one, i.e. the one with the higher
    /// **local** z or r value is extended
    Second,
    /// Given two volumes, the *midpoint* between the two volumes is found
    Midpoint,
    /// A gap volume is created to fit between the two volumes
    Gap,
  };

  /// The resize strategy defines how the volumes are resized
  enum class ResizeStrategy {
    /// Extend the volume connected to the respective edge to fit the new bounds
    Expand,
    /// Create a gap volume at the respective edge to fit the new bounds
    Gap,
  };

  /// Constructor from a vector of volumes and direction
  /// @param volumes is the vector of volumes
  /// @param direction is the binning direction
  /// @param strategy is the attachment strategy
  /// @param resizeStrategy is the resize strategy
  /// @note @p resizeStrategy only affects resizing along
  ///       @p direction. Resizing in the other direction
  ///       is always delegated to the child volumes,
  ///       which might in turn be @c CylinderVolumeStack
  /// @param logger is the logger
  /// @pre The volumes need to have a common coordinate
  ///      system relative to @p direction. I.e. they need
  ///      to be aligned in @c z and cannot have a rotation
  ///      in @c x or @c y.
  /// @pre The volumes all need to have @c CylinerVolumeBounds
  ///      and cannot have a @f$\phi@f$ sector or bevels.
  /// @note Preconditions are checked on construction
  CylinderVolumeStack(
      std::vector<Volume*>& volumes, BinningValue direction,
      AttachmentStrategy strategy = AttachmentStrategy::Midpoint,
      ResizeStrategy resizeStrategy = ResizeStrategy::Expand,
      const Logger& logger = Acts::getDummyLogger());

  /// Update the volume bounds and transform. This
  /// will update the bounds of all volumes in the stack
  /// to accommodate the new bounds and optionally create
  /// gap volumes according to the resize strategy set during
  /// construction.
  /// @param volbounds is the new bounds
  /// @param transform is the new transform
  /// @pre The volume bounds need to be of type
  ///      @c CylinderVolumeBounds.
  void update(std::shared_ptr<VolumeBounds> volbounds,
              std::optional<Transform3> transform = std::nullopt) override;

  /// Update the volume bounds and transform. This
  /// will update the bounds of all volumes in the stack
  /// to accommodate the new bounds and optionally create
  /// gap volumes according to the resize strategy set during
  /// construction.
  /// @param newBounds is the new bounds
  /// @param transform is the new transform
  /// @param logger is the logger
  /// @pre The volume bounds need to be of type
  ///      @c CylinderVolumeBounds.
  void update(std::shared_ptr<CylinderVolumeBounds> newBounds,
              std::optional<Transform3> transform, const Logger& logger);

  /// Access the gap volume that were created during attachment or resizing.
  /// @return the vector of gap volumes
  const std::vector<std::shared_ptr<Volume>>& gaps() const;

 private:
  /// Helper to get the first volume in the input, and throw an exception if
  /// there is not one.
  /// @param volumes is the vector of volumes
  /// @return the first volume
  static Volume& initialVolume(const std::vector<Volume*>& volumes);

  /// Helper function called during construction that performs the
  /// internal attachment and produces the overall outer volume bounds.
  /// @param direction is the binning direction
  /// @param strategy is the attachment strategy
  /// @param logger is the logger
  void initializeOuterVolume(BinningValue direction,
                             AttachmentStrategy strategy, const Logger& logger);

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
  static void overlapPrint(const VolumeTuple& a, const VolumeTuple& b,
                           const Logger& logger);

  /// Helper function that checks if volumes are properly aligned
  /// for attachment.
  /// @param volumes is the vector of volumes
  /// @param logger is the logger
  static void checkVolumeAlignment(const std::vector<VolumeTuple>& volumes,
                                   const Logger& logger);

  /// Helper function that checks overlaps and attaches in z direction
  /// @param volumes is the vector of volumes
  /// @param strategy is the attachment strategy
  /// @param logger is the logger
  /// @return vector of gap volumes. Can be empty if none were created.
  std::vector<VolumeTuple> checkOverlapAndAttachInZ(
      std::vector<VolumeTuple>& volumes, AttachmentStrategy strategy,
      const Logger& logger);

  /// Helper function to synchronize the r bounds of the volumes
  /// @param volumes is the vector of volumes
  /// @param logger is the logger
  /// @return tuple of the minimum and maximum radii
  std::pair<ActsScalar, ActsScalar> synchronizeRBounds(
      std::vector<VolumeTuple>& volumes, const Logger& logger);

  /// Helper function that checks overlaps and attaches in r direction
  /// @param volumes is the vector of volumes
  /// @param strategy is the attachment strategy
  /// @param logger is the logger
  /// @return vector of gap volumes. Can be empty if none were created.
  std::vector<VolumeTuple> checkOverlapAndAttachInR(
      std::vector<VolumeTuple>& volumes, AttachmentStrategy strategy,
      const Logger& logger);

  /// Helper function to synchronize the z bounds of the volumes
  /// @param volumes is the vector of volumes
  /// @param logger is the logger
  /// @return tuple of the minimum and maximum z extent
  std::pair<ActsScalar, ActsScalar> synchronizeZBounds(
      std::vector<VolumeTuple>& volumes, const Logger& logger);

  /// Helper functions that checks if the cylinder volume bounds
  /// given do not contain any phi sectors or bevels.
  /// @param bounds is the cylinder volume bounds
  /// @param logger is the logger
  static void checkNoPhiOrBevel(const CylinderVolumeBounds& bounds,
                                const Logger& logger);

  /// Helper function to create a gap volume with given bounds and register it.
  /// @param transform is the transform of the gap volume
  /// @param bounds is the bounds of the gap volume
  /// @return the shared pointer to the gap volume
  std::shared_ptr<Volume> addGapVolume(
      const Transform3& transform, const std::shared_ptr<VolumeBounds>& bounds);

  BinningValue m_direction{};
  ResizeStrategy m_resizeStrategy{};
  Transform3 m_groupTransform{};
  std::vector<std::shared_ptr<Volume>> m_gaps{};
  std::vector<Volume*>& m_volumes;
};

/// Output operator for the attachment strategy
/// @param os is the output stream
/// @param strategy is the attachment strategy
/// @return the output stream
std::ostream& operator<<(std::ostream& os,
                         CylinderVolumeStack::AttachmentStrategy strategy);

/// Output operator for the resize strategy
/// @param os is the output stream
/// @param strategy is the resize strategy
/// @return the output stream
std::ostream& operator<<(std::ostream& os,
                         CylinderVolumeStack::ResizeStrategy strategy);

}  // namespace Acts
