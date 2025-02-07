// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Geometry/BlueprintNode.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/PortalShell.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"

namespace Acts {

/// The static blueprint node wraps a single already-constructred @c TrackingVolume.
/// The node will present this volume to its hierarchy. The volume is given as
/// mutable, and will be potentially enlarged in order to connect to neighboring
/// volumes.
/// - In case the volume already has child volumes, they will be retained.
/// - In case the volume already has a registered navigation policy, it will be
///   overwritten with the one configured on this node, regardless of content.
class StaticBlueprintNode : public BlueprintNode {
 public:
  /// Construct the static node from an existing volume
  /// @param volume The volume to wrap
  explicit StaticBlueprintNode(std::unique_ptr<TrackingVolume> volume);

  /// Get the name of this node. It is automatically taken from the wrapped
  /// volume
  /// @return The name of the volume
  const std::string& name() const override;

  /// @copydoc BlueprintNode::build
  /// Build-phase of the blueprint construction. Returns the wrapped volume for
  /// sizing.
  Volume& build(const BlueprintOptions& options, const GeometryContext& gctx,
                const Logger& logger = Acts::getDummyLogger()) override;

  /// @copydoc BlueprintNode::connect
  PortalShellBase& connect(
      const BlueprintOptions& options, const GeometryContext& gctx,
      const Logger& logger = Acts::getDummyLogger()) override;

  /// @copydoc BlueprintNode::finalize
  void finalize(const BlueprintOptions& options, const GeometryContext& gctx,
                TrackingVolume& parent,
                const Logger& logger = Acts::getDummyLogger()) override;

  virtual StaticBlueprintNode& setNavigationPolicyFactory(
      std::shared_ptr<NavigationPolicyFactory> navigationPolicyFactory);

  const NavigationPolicyFactory* navigationPolicyFactory() const;

 protected:
  void addToGraphviz(std::ostream& os) const override;

  std::unique_ptr<TrackingVolume> m_volume;

  std::unique_ptr<PortalShellBase> m_shell;

  std::shared_ptr<NavigationPolicyFactory> m_navigationPolicyFactory;
};

}  // namespace Acts
