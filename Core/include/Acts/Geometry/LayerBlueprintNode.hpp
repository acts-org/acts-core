// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/StaticBlueprintNode.hpp"

#include <ostream>

namespace Acts::Experimental {

/// The layer node is essentially an auto-sizing wrapper around a set of
/// surfaces.
/// @note This implementation is **preliminary** and will likely change
///       in the future.
/// It defers most of the functionality to @ref Acts::Experimental::StaticBlueprintNode,
/// after the initial volume creation is completed.
///
/// The layer volume is created to wrap around the surfaces registered with
/// this node. The orientation of the resulting volume defaults to the identity
/// matrix. If another orientation is desired, this can be set with the @ref
/// Acts::Experimental::LayerBlueprintNode::setTransform. See @ref Acts::ProtoLayer for
/// details on the auto-sizing from surfaces.
///
class LayerBlueprintNode : public StaticBlueprintNode {
 public:
  /// Enum that lists out the supported layer types.
  enum class LayerType {
    /// A cylinder layer
    Cylinder,

    /// A disc layer
    Disc,

    /// A plane layer
    /// @note This is not yet implemented
    Plane
  };

  /// Constructor for a layer node.
  /// @param name The name of the layer
  explicit LayerBlueprintNode(const std::string& name)
      : StaticBlueprintNode{nullptr}, m_name(name) {}

  /// @copydoc BlueprintNode::name
  const std::string& name() const override;

  /// This function participates in the geometry construction.
  /// It will:
  /// -# Analyze the surfaces provided and produce a wrapping volume
  /// -# Register the surfaces with the volume
  /// -# Return the volume
  /// @note At least one surfaces needs to be registered via
  ///       @ref Acts::Experimental::LayerBlueprintNode::setSurfaces before
  ///       geometry construction.
  Volume& build(const BlueprintOptions& options, const GeometryContext& gctx,
                const Logger& logger = Acts::getDummyLogger()) override;

  /// Register a set of surfaces with the layer node.
  /// @param surfaces The surfaces to register
  /// @return Reference to this node for chaining
  LayerBlueprintNode& setSurfaces(
      std::vector<std::shared_ptr<Surface>> surfaces);

  /// Access the registered surfaces.
  /// @return The registered surfaces
  const std::vector<std::shared_ptr<Surface>>& surfaces() const;

  /// Set the transformation of the layer node.
  /// This can be used to specifically orient the resulting layer volume.
  /// @param transform The transformation to set
  /// @return Reference to this node for chaining
  LayerBlueprintNode& setTransform(const Transform3& transform);

  /// Access the transformation of the layer node.
  /// @return The transformation
  const Transform3& transform() const;

  /// Set the envelope of the layer node. This configures the amount of space to
  /// add around the contained surfaces.
  /// @param envelope The envelope to set
  /// @return Reference to this node for chaining
  LayerBlueprintNode& setEnvelope(const ExtentEnvelope& envelope);

  /// Access the envelope of the layer node.
  /// @return The envelope
  const ExtentEnvelope& envelope() const;

  /// Set the layer type of the layer node.
  /// @param layerType The layer type to set
  /// @return Reference to this node for chaining
  LayerBlueprintNode& setLayerType(LayerType layerType);

  /// Access the layer type of the layer node.
  /// @return The layer type
  const LayerType& layerType() const;

  /// Output operator for the layer type enum.
  /// @param os The output stream
  /// @param type The layer type
  friend std::ostream& operator<<(std::ostream& os,
                                  LayerBlueprintNode::LayerType type) {
    switch (type) {
      using enum LayerBlueprintNode::LayerType;
      case Cylinder:
        os << "Cylinder";
        break;
      case Disc:
        os << "Disc";
        break;
      case Plane:
        os << "Plane";
        break;
    }
    return os;
  }

 private:
  /// @copydoc Acts::Experimental::BlueprintNode::addToGraphviz
  void addToGraphviz(std::ostream& os) const override;

  /// Helper method that performs the volume creation from the configured
  /// surfaces. It converts from an @p extent object to an instance of @ref
  /// Acts::VolumeBounds.
  /// @param extent The extent to use for the volume creation
  /// @param logger The logger to use
  void buildVolume(const Extent& extent, const Logger& logger);

  std::string m_name;
  std::vector<std::shared_ptr<Surface>> m_surfaces{};
  Transform3 m_transform = Transform3::Identity();
  ExtentEnvelope m_envelope = ExtentEnvelope::Zero();
  LayerType m_layerType = LayerType::Cylinder;
};

}  // namespace Acts::Experimental
