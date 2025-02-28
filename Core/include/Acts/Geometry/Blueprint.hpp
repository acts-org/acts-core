// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/BlueprintNode.hpp"
#include "Acts/Geometry/PortalShell.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"

namespace Acts {

class GeometryContext;

namespace Experimental {

/// This class is the top-level entry point to build a tracking geometry using
/// the blueprint building mechanism. It forms the root of a tree of nodes where
/// each node performs a portion of the construction. This top-level class has
/// the main construction methods that execute the construction of the geometry.
///
/// ```
///            +---------------+  +-----------+
///            |               |  |           |
///            |     Root      |  |           v
///            |               |  |   +---------------+
///            +---------------+  |   |               |
///                    |          |   |    Child 1    |
///         +----------+          |   |               |
///         v          +----------+   +---------------+
/// +---------------+                         |
/// |               |          +--------------+
/// |    Child 2    |          v         +----------+
/// |               |     .---------.    |          |
/// +---------------+    /           \   |          v
///                     (  Proc node  )  |  +---------------+
///                      `.         ,'   |  |               |
///                        `-------'     |  |    Child 3    |
///                            |         |  |               |
///                            |         |  +---------------+
///                            +---------+
/// ```
///
/// The construction phases are documented in @c BlueprintNode, which is the
/// base class for all nodes in the tree.
/// @note This class inherits from @c BlueprintNode, but hides the main
///       blueprint construction phase overloads. The @c Blueprint class is
///       only ever intended to be the top-level node, and not anywhere else
///       in the tree.
class Blueprint : public BlueprintNode {
 public:
  struct Config {
    /// Determine how much envelope space to produce from the highest volume
    /// in the geometry hierarchy and the world volume.
    ExtentEnvelope envelope = ExtentEnvelope::Zero();
  };

  /// Constructor from a config object
  /// @param config The configuration object
  explicit Blueprint(const Config& config);

  /// Construct the tracking geometry from the blueprint tree
  /// @param options The construction options, see @c BlueprintOptions
  /// @param gctx The geometry context for construction. In almost all cases,
  ///             this should be the *nominal* geometry context
  /// @param logger The logger to use for output during construction
  std::unique_ptr<TrackingGeometry> construct(
      const BlueprintOptions& options, const GeometryContext& gctx,
      const Logger& logger = Acts::getDummyLogger());

 protected:
  /// The name of the blueprint node, always "Root"
  /// @return The name
  const std::string& name() const override;

  /// @copydoc BlueprintNode::build
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

  /// @copydoc BlueprintNode::addToGraphviz
  void addToGraphviz(std::ostream& os) const override;

 private:
  Config m_cfg;
};

}  // namespace Experimental
}  // namespace Acts
