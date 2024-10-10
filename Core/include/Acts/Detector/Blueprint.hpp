// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Common.hpp"
#include "Acts/Detector/ProtoBinning.hpp"
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Geometry/VolumeBounds.hpp"
#include "Acts/Utilities/BinningData.hpp"
#include "Acts/Utilities/StringHelpers.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Acts::Experimental {

class IGeometryIdGenerator;
class IInternalStructureBuilder;
class IRootVolumeFinderBuilder;

/// A Blueprint is an instruction tree that allows you to defina a tree sequence
/// of volume building using the provided tools.
///
/// It follows tree nomenclature and can define:
///
/// - a root node (the top of the tree)
/// - a branch node (also called inner node)
/// - leaf node (also called terminal node)
///
/// Leaf nodes can have internal builders attached, while all the external
/// builders will be created when the blueprint is interpreted.
namespace Blueprint {

struct Node final {
  /// Branch constructor
  ///
  /// @param n name of the node
  /// @param t the transform
  /// @param bt the boundary type
  /// @param bv the boundary values
  /// @param bss the binning values
  /// @param cs the children of the node
  /// @param e the estimated extent of the node (optional)
  Node(const std::string& n, const Transform3& t, VolumeBounds::BoundsType bt,
       const std::vector<ActsScalar>& bv, const std::vector<BinningValue>& bss,
       std::vector<std::unique_ptr<Node>> cs = {}, const Extent& e = Extent())
      : name(n),
        transform(t),
        boundsType(bt),
        boundaryValues(bv),
        children(std::move(cs)),
        binning(bss),
        extent(e) {
    for_each(children.begin(), children.end(),
             [this](std::unique_ptr<Node>& c) { c->parent = this; });
  }

  /// Leaf constructor
  ///
  /// @param n name of the node
  /// @param t the transform
  /// @param bt the boundary type
  /// @param bv the boundary values
  /// @param isb the internal structure builder (optional)
  /// @param e the estimated extent of the node (optional)
  Node(const std::string& n, const Transform3& t, VolumeBounds::BoundsType bt,
       const std::vector<ActsScalar>& bv,
       std::shared_ptr<const IInternalStructureBuilder> isb = nullptr,
       const Extent& e = Extent())
      : name(n),
        transform(t),
        boundsType(bt),
        boundaryValues(bv),
        internalsBuilder(std::move(isb)),
        extent(e) {}

  /// Name identification of this node
  std::string name = "";
  /// Transform definition of this node
  Transform3 transform = Transform3::Identity();
  /// The boundary type
  VolumeBounds::BoundsType boundsType = VolumeBounds::eOther;
  /// The associated values
  std::vector<ActsScalar> boundaryValues = {};
  /// Parent node - nullptr for root only
  const Node* parent = nullptr;
  /// Branch definitions: children
  std::vector<std::unique_ptr<Node>> children = {};
  /// Branch definition binning
  std::vector<BinningValue> binning = {};

  /// Portal proto material binning
  std::map<unsigned int, BinningDescription> portalMaterialBinning = {};

  /// Auxiliary information
  std::vector<std::string> auxiliary = {};

  /// Builders and helper tools that can be attached
  std::shared_ptr<const IRootVolumeFinderBuilder> rootVolumeFinderBuilder =
      nullptr;
  /// Geometry id generator
  std::shared_ptr<const IGeometryIdGenerator> geoIdGenerator = nullptr;
  /// Internal structure builder - for leaf nodes
  std::shared_ptr<const IInternalStructureBuilder> internalsBuilder = nullptr;

  /// An optional extent object
  Extent extent = Extent();

  /// @brief Check if it is a leaf node
  bool isLeaf() const { return children.empty(); }

  /// @brief Check is it is a root
  bool isRoot() const { return parent == nullptr; }

  /// @brief Method to add a child to this branch
  /// @param c the child to be added
  void add(std::unique_ptr<Node> c) {
    c->parent = this;
    children.push_back(std::move(c));
  }
};

}  // namespace Blueprint
}  // namespace Acts::Experimental
