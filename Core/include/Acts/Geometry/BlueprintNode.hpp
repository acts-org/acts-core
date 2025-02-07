// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/BlueprintOptions.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/NavigationPolicyFactory.hpp"
#include "Acts/Utilities/AxisDefinitions.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/TransformRange.hpp"

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

namespace Acts {

class Volume;
class TrackingVolume;
class VolumeBounds;
class PortalShellBase;
class CylinderContainerBlueprintNode;
class MaterialDesignatorBlueprintNode;
class StaticBlueprintNode;
class LayerBlueprintNode;

/// Base class for all nodes in the blueprint tree. This class defines the
/// three-phase construction process. The three phases are
///
/// -# **Build**: Construct volume representation + compute final sizing
/// -# **Connect**: Create and connect portals at volume boundaries
/// -# **Finalize**: Register portals with volumes + create acceleration
///    structures
///
/// During the *build* phase, the `build` method of all nodes in the tree are
/// called recursively. Some nodes, like @ref Acts::CylinderContainerBlueprintNode,
/// will take action on the volumes returns from its children, and perform
/// sizing to connect them. See the @ref Acts::CylinderContainerBlueprintNode and @ref
/// Acts::CylinderVolumeStack documentation for details on how the sizing is
/// carried out.
class BlueprintNode {
 public:
  /// Virtual destructor to ensure correct cleanup
  virtual ~BlueprintNode() = default;

  /// Get the name of this node
  virtual const std::string& name() const = 0;

  /// @anchor construction
  /// @name Construction methods
  /// These methods constitute the primary interface of the node that
  /// participates in the geometry construction.
  /// @{

  /// This method is called during the *build* phase of the blueprint tree
  /// construction. It returns a single @ref Acts::Volume which represents transform
  /// and bounds of the entire subtree. This does not have to correspond to the
  /// final @ref Acts::TrackingVolume, some node types will produce temporary volume
  /// representations. Lifetime of the returned volume is managed by the source
  /// node!
  /// Nodes higher in the hierarchy will issue resizes down the tree hierarchy.
  /// This is not done through a direct hierarchy, but coordinated by the
  /// respective node type, by internally consulting its children.
  ///
  /// @note Generally, you should not need to to call this method directly.
  ///       The construction should usually be done through the special
  ///       @ref Acts::Blueprint class.
  ///
  /// @param options The global construction options
  /// @param gctx The geometry context for construction (usually nominal)
  /// @param logger The logger to use for output during construction
  /// @return The volume used for communicating transform and size up the hierarchy
  virtual Volume& build(const BlueprintOptions& options,
                        const GeometryContext& gctx,
                        const Logger& logger = Acts::getDummyLogger()) = 0;

  /// This method is called during the *connect* phase. This phase handles the
  /// creation and connection of *portals* (instances of @ref Acts::PortalLinkBase).
  /// After the build-phase has completed, the volume sizes are **final**. Each
  /// node will consult its fully sized volume to produce *boundary surfaces*.
  /// Each boundary surface is then turned into a @ref Acts::TrivialPortalLink, which
  /// in turn produces a one-sided portal (see @ref Acts::Portal documentation)
  ///
  /// Some nodes (like @ref Acts::CylinderContainerBlueprintNode) will take action on
  /// their children, and unify the connected portals.
  ///
  /// After a node's processing has completed, it returns a reference to a @ref
  /// Acts::PortalShellBase, which represents a set of portals in a specific
  /// geometry arrangement. The returned object lifetime is managed by the
  /// returning node.
  ///
  /// @param options The global construction options
  /// @param gctx The geometry context for construction (usually nominal)
  /// @param logger The logger to use for output during construction
  virtual PortalShellBase& connect(
      const BlueprintOptions& options, const GeometryContext& gctx,
      const Logger& logger = Acts::getDummyLogger()) = 0;

  /// This method is called during the *finalize* phase. This phase handles:
  ///
  /// - Registering portals into their final volumes
  /// - Registering volumes into their parents
  /// - Creating navigation policies
  /// - (In future) Handle geometry identification assignment
  ///
  /// At the end of this phase, each node will have transferred any temporary
  /// resources created during the build, that need to be retained, into the
  /// final @ref Acts::TrackingGeometry, and can be safely destroyed.
  ///
  /// @note The @p parent for volumes, portals, etc to be registered in is passed in **as an
  ///       argument**, rather than being implicitly determined from the
  ///       **parent node**. This is done so that nodes can remove themselves
  ///       from the final volume hierarchy, like container nodes or the
  ///       @ref Acts::MaterialDesignatorBlueprintNode.
  ///
  /// @param options The global construction options
  /// @param gctx The geometry context for construction (usually nominal)
  /// @param parent The parent volume to register in
  /// @param logger The logger to use for output during construction
  virtual void finalize(const BlueprintOptions& options,
                        const GeometryContext& gctx, TrackingVolume& parent,
                        const Logger& logger = Acts::getDummyLogger()) = 0;

  /// @}

  /// @anchor convenience
  /// @name Convenience methods
  /// These methods are meant to make the construction of a blueprint tree in
  /// code more ergonomic.
  /// They usually take an optional `callback` parameter. The primary use for
  /// this parameter is structural, as it facilitates introducing scopes to
  /// indicate in code that objects are nested.
  ///
  /// ```cpp
  /// Blueprint::Config cfg;
  /// auto root = std::make_unique<Blueprint>(cfg);
  /// root->addStaticVolume(
  ///     base, std::make_shared<CylinderVolumeBounds>(50_mm, 400_mm, 1000_mm),
  ///     "PixelWrapper", [&](auto& wrapper) {
  ///         // This scope can be used to equip `wrapper`
  ///     });
  /// ```
  ///
  /// Alternatively, they can also be used without a callback, as the newly
  /// created node is also returned by reference:
  ///
  /// ```
  /// auto& wrapper = root->addStaticVolume(
  ///     base, std::make_shared<CylinderVolumeBounds>(50_mm, 400_mm, 1000_mm),
  ///     "PixelWrapper");
  /// ```
  ///
  /// In both cases, it's not necessary to register the newly created node
  /// with a parent node.
  ///
  /// @{

  /// This method creates a new @ref Acts::StaticBlueprintNode wrapping @p
  /// volume and adds it to this node as a child.
  /// @param volume The volume to add
  /// @param callback An optional callback that receives the node as an argument
  /// @return A reference to the created node
  StaticBlueprintNode& addStaticVolume(
      std::unique_ptr<TrackingVolume> volume,
      const std::function<void(StaticBlueprintNode& cylinder)>& callback = {});

  /// Alternative overload for creating a @ref Acts::StaticBlueprintNode. This
  /// overload will invoke the constructor of @ref Acts::TrackingVolume and use
  /// that volume to create the node.
  /// @param transform The transform of the volume
  /// @param volumeBounds The bounds of the volume
  /// @param volumeName The name of the volume
  /// @param callback An optional callback that receives the node as an argument
  StaticBlueprintNode& addStaticVolume(
      const Transform3& transform, std::shared_ptr<VolumeBounds> volumeBounds,
      const std::string& volumeName = "undefined",
      const std::function<void(StaticBlueprintNode& cylinder)>& callback = {});

  /// Convenience method for creating a @ref Acts::CylinderContainerBlueprintNode.
  /// @param name The name of the container node. This name is only reflected
  ///             in the node tree for debugging, as no extra volumes is created
  ///             for the container.
  /// @param direction The direction of the stack configuration. See
  ///                  @ref Acts::CylinderVolumeStack for details.
  /// @param callback An optional callback that receives the node as an argument
  CylinderContainerBlueprintNode& addCylinderContainer(
      const std::string& name, AxisDirection direction,
      const std::function<void(CylinderContainerBlueprintNode& cylinder)>&
          callback = {});

  /// Convenience method for creating a @ref Acts::MaterialDesignatorBlueprintNode.
  /// @param name The name of the material designator node. Used for debugging
  ///             the node tree only.
  /// @param callback An optional callback that receives the node as an argument
  MaterialDesignatorBlueprintNode& addMaterial(
      const std::string& name,
      const std::function<void(MaterialDesignatorBlueprintNode& material)>&
          callback = {});

  /// Convenience method for creating a @ref Acts::LayerBlueprintNode.
  /// @param name The name of the layer node.
  /// @param callback An optional callback that receives the node as an argument
  LayerBlueprintNode& addLayer(
      const std::string& name,
      const std::function<void(LayerBlueprintNode& layer)>& callback = {});

  /// @}

  /// Register a @p child to this node.
  /// @warning This method throws if adding the child would create a
  ///          cycle in the blueprint tree!
  /// @param child The child node to add
  /// @return A reference this node (not the child!)
  BlueprintNode& addChild(std::shared_ptr<BlueprintNode> child);

  /// A range-like object that allows range based for loops and index access.
  /// This type's iterators and accessors return mutable references when
  /// dereferenced.
  using MutableChildRange =
      detail::TransformRange<detail::Dereference,
                             std::vector<std::shared_ptr<BlueprintNode>>>;

  /// A range-like object that allows range based for loops and index access.
  /// This type's iterators and accessors return const references when
  /// dereferenced.
  using ChildRange =
      detail::TransformRange<detail::ConstDereference,
                             const std::vector<std::shared_ptr<BlueprintNode>>>;

  /// Return a @ref MutableChildRange to the children of this node.
  /// @return A range-like object to the children
  MutableChildRange children();

  /// Return a @ref ChildRange to the children of this node.
  /// @return A range-like object to the children
  ChildRange children() const;

  /// Remove all children from this node
  void clearChildren();

  /// Return the depth of this node in the blueprint tree. A depth of zero means
  /// this node does not have a parent.
  /// @return The depth of this node
  std::size_t depth() const;

  /// Print the node tree starting from this node to graphviz format
  /// @param os The stream to print to
  void graphviz(std::ostream& os) const;

  /// Method that writes a representatiohn of **this node only** to graphviz.
  /// This should generally not be called on its own, but through the @ref
  /// BlueprintNode::graphviz method.
  /// @param os The stream to print to
  virtual void addToGraphviz(std::ostream& os) const;

  /// Print a representation of this node to the stream
  /// @param os The stream to print to
  /// @param node The node to print
  /// @return The output stream
  friend std::ostream& operator<<(std::ostream& os, const BlueprintNode& node) {
    node.toStream(os);
    return os;
  }

 protected:
  /// Virtual method to determine stream representation.
  /// @note This method is called by the stream operator.
  virtual void toStream(std::ostream& os) const;

  /// Set the depth to @p depth and update children recursively
  void setDepth(std::size_t depth);

  /// Printing helper returning a prefix including an indent depending on the
  /// depth.
  /// @return The prefix string
  std::string prefix() const;

  /// An indentation depending on the depth of this node.
  /// @return The indentation string
  std::string indent() const;

 private:
  std::size_t m_depth{0};
  std::vector<std::shared_ptr<BlueprintNode>> m_children{};
};

}  // namespace Acts
