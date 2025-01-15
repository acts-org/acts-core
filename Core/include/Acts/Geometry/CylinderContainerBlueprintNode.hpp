// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/BlueprintNode.hpp"
#include "Acts/Geometry/CylinderVolumeStack.hpp"
#include "Acts/Geometry/PortalShell.hpp"
#include "Acts/Utilities/AxisDefinitions.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <map>

namespace Acts {

/// This class handles the case of wrapping a set of cylinder-shaped children
/// and stacking them in a configured direction.
/// The stacking is done using @ref CylinderVolumeStack.
/// The container does not result in an extra volume in the hierarchy, as all
/// input volumes and any gap volumes produced are directly registered in the
/// volume of the parent of this node.
/// @note This node assumes all children produce only cylinder volumes! It throws
///       if this is not the case.
class CylinderContainerBlueprintNode final : public BlueprintNode {
 public:
  /// Main constructor for the cylinder container node.
  /// @param name The name of the node (for debug only)
  /// @param direction The stacking direction
  /// @param attachmentStrategy The attachment strategy for the stack
  /// @param resizeStrategy The resize strategy
  /// @note The parameters are passed through to @ref CylinderVolumeStack,
  ///       see documentation of that class for more information
  CylinderContainerBlueprintNode(
      const std::string& name, AxisDirection direction,
      CylinderVolumeStack::AttachmentStrategy attachmentStrategy =
          CylinderVolumeStack::AttachmentStrategy::Midpoint,
      CylinderVolumeStack::ResizeStrategy resizeStrategy =
          CylinderVolumeStack::ResizeStrategy::Expand);

  /// @copydoc BlueprintNode::name
  const std::string& name() const override;

  /// This participates in the construction of the geometry via the blueprint
  /// tree. The steps are approximately as follows:
  /// -# Collect all child volumes
  /// -# Package them into a @ref Acts::CylinderVolumeStack, which performs
  ///    sizing and/or gap creation
  /// -# Return the @ref Acts::CylinderVolumeStack as a volume up the tree
  ///
  /// @param options The global blueprint options
  /// @param gctx The geometry context (nominal usually)
  /// @param logger The logger to use
  /// @return The combined @ref Acts::CylinderVolumeStack
  Volume& build(const BlueprintOptions& options, const GeometryContext& gctx,
                const Logger& logger = Acts::getDummyLogger()) override;

  /// This participates in the construction of the geometry via the blueprint
  /// tree. The steps are approximately as follows:
  /// -# Walk through all volumes that were created by the build phase
  /// -# Check if they are: *real* child volumes or gap volumes
  ///   - If gap volume: produce a @ref Acts::TrackingVolume, and wrap it in a single use shell
  ///   - If child volume: locate the right child node it came from, call
  ///   ` connect` and collect the returned shell
  /// -# Produce a combined @ref Acts::CylinderStackPortalShell from all the shells
  /// -# Return that shell representation
  ///
  /// @param options The global blueprint options
  /// @param gctx The geometry context (nominal usually)
  /// @param logger The logger to use
  /// @return The combined @ref Acts::CylinderStackPortalShell
  CylinderStackPortalShell& connect(
      const BlueprintOptions& options, const GeometryContext& gctx,
      const Logger& logger = Acts::getDummyLogger()) override;

  /// This participates in the construction of the geometry via the blueprint
  /// tree. The steps are approximately as follows:
  /// -# Register portals created for gap volumes, as they're not handled by
  ///    dedicated nodes
  /// -# Register gap volumes in the @p parent volume
  /// -# Create a configured @ref Acts::INavigationPolicy for the gap
  /// -# Call `finalize` on all children while passing through @p parent.
  ///
  /// @param options The global blueprint options
  /// @param gctx The geometry context (nominal usually)
  /// @param parent The parent volume
  /// @param logger The logger to use
  void finalize(const BlueprintOptions& options, const GeometryContext& gctx,
                TrackingVolume& parent, const Logger& logger) override;

  /// Setter for the stacking direction
  /// @param direction The stacking direction
  /// @return This node for chaining
  CylinderContainerBlueprintNode& setDirection(AxisDirection direction);

  /// Setter for the attachment strategy
  /// @param attachmentStrategy The attachment strategy
  /// @return This node for chaining
  CylinderContainerBlueprintNode& setAttachmentStrategy(
      CylinderVolumeStack::AttachmentStrategy attachmentStrategy);

  /// Setter for the resize strategy
  /// @param resizeStrategy The resize strategy
  /// @return This node for chaining
  CylinderContainerBlueprintNode& setResizeStrategy(
      CylinderVolumeStack::ResizeStrategy resizeStrategy);

  /// Accessor to the stacking direction
  /// @return The stacking direction
  AxisDirection direction() const;

  /// Accessor to the attachment strategy
  /// @return The attachment strategy
  CylinderVolumeStack::AttachmentStrategy attachmentStrategy() const;

  /// Accessor to the resize strategy
  /// @return The resize strategy
  CylinderVolumeStack::ResizeStrategy resizeStrategy() const;

 private:
  /// @copydoc BlueprintNode::addToGraphviz
  void addToGraphviz(std::ostream& os) const override;

  /// Helper function to check if a volume was created as a gap volume.
  /// @param volume The volume to check
  /// @return True if the volume is a gap volume, false otherwise
  bool isGapVolume(const Volume& volume) const;

  std::vector<CylinderPortalShell*> collectChildShells(
      const BlueprintOptions& options, const GeometryContext& gctx,
      const Logger& logger);

  std::string m_name;

  AxisDirection m_direction = AxisDirection::AxisZ;

  CylinderVolumeStack::AttachmentStrategy m_attachmentStrategy{
      CylinderVolumeStack::AttachmentStrategy::Midpoint};

  CylinderVolumeStack::ResizeStrategy m_resizeStrategy{
      CylinderVolumeStack::ResizeStrategy::Expand};

  // Is only initialized during `build`
  std::vector<Volume*> m_childVolumes;
  std::unique_ptr<CylinderVolumeStack> m_stack{nullptr};
  std::map<const Volume*, BlueprintNode*> m_volumeToNode;
  std::vector<std::pair<std::unique_ptr<SingleCylinderPortalShell>,
                        std::unique_ptr<TrackingVolume>>>
      m_gaps;
  std::unique_ptr<CylinderStackPortalShell> m_shell{nullptr};
};

}  // namespace Acts
