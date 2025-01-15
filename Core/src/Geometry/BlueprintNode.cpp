// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/BlueprintNode.hpp"

#include "Acts/Geometry/Blueprint.hpp"
#include "Acts/Geometry/CylinderContainerBlueprintNode.hpp"
#include "Acts/Geometry/LayerBlueprintNode.hpp"
#include "Acts/Geometry/MaterialDesignatorBlueprintNode.hpp"
#include "Acts/Geometry/StaticBlueprintNode.hpp"
#include "Acts/Navigation/INavigationPolicy.hpp"
#include "Acts/Navigation/TryAllNavigationPolicy.hpp"

#include <concepts>
#include <ostream>

namespace Acts {

namespace {
bool hasDescendent(const BlueprintNode& descendent,
                   const BlueprintNode& ancestor) {
  if (&descendent == &ancestor) {
    return true;
  }

  return std::ranges::any_of(ancestor.children(),
                             [&](const auto& child) -> bool {
                               return hasDescendent(descendent, child);
                             });
}
}  // namespace

void BlueprintNode::toStream(std::ostream& os) const {
  os << "BlueprintNode(" << name() << ")";
}

BlueprintNode& BlueprintNode::addChild(std::shared_ptr<BlueprintNode> child) {
  if (!child) {
    throw std::invalid_argument("Child is nullptr");
  }

  if (dynamic_cast<Blueprint*>(child.get()) != nullptr) {
    throw std::invalid_argument("Cannot add a Blueprint as a child");
  }

  if (child->depth() != 0) {
    throw std::invalid_argument("Child has already been added to another node");
  }

  if (hasDescendent(*this, *child)) {
    throw std::invalid_argument("Adding child would create a cycle");
  }

  child->setDepth(m_depth + 1);
  m_children.push_back(std::move(child));
  return *this;
}

BlueprintNode::MutableChildRange BlueprintNode::children() {
  return MutableChildRange{m_children};
}

BlueprintNode::ChildRange BlueprintNode::children() const {
  return ChildRange{m_children};
}

std::size_t BlueprintNode::depth() const {
  return m_depth;
}

void BlueprintNode::setDepth(std::size_t depth) {
  m_depth = depth;
  for (auto& child : children()) {
    child.setDepth(depth + 1);
  }
}

std::string BlueprintNode::indent() const {
  return std::string(m_depth * 2, ' ');
}

std::string BlueprintNode::prefix() const {
  return indent() + "[" + name() + "]: ";
}

StaticBlueprintNode& BlueprintNode::addStaticVolume(
    std::unique_ptr<TrackingVolume> volume,
    const std::function<void(StaticBlueprintNode& cylinder)>& callback) {
  if (!volume) {
    throw std::invalid_argument("Volume is nullptr");
  }

  auto child = std::make_shared<StaticBlueprintNode>(std::move(volume));
  addChild(child);

  if (callback) {
    callback(*child);
  }
  return *child;
}

StaticBlueprintNode& BlueprintNode::addStaticVolume(
    const Transform3& transform, std::shared_ptr<VolumeBounds> volumeBounds,
    const std::string& volumeName,
    const std::function<void(StaticBlueprintNode& cylinder)>& callback) {
  return addStaticVolume(std::make_unique<TrackingVolume>(
                             transform, std::move(volumeBounds), volumeName),
                         callback);
}

CylinderContainerBlueprintNode& BlueprintNode::addCylinderContainer(
    const std::string& name, AxisDirection direction,
    const std::function<void(CylinderContainerBlueprintNode& cylinder)>&
        callback) {
  auto cylinder =
      std::make_shared<CylinderContainerBlueprintNode>(name, direction);
  addChild(cylinder);
  if (callback) {
    callback(*cylinder);
  }
  return *cylinder;
}

MaterialDesignatorBlueprintNode& BlueprintNode::addMaterial(
    const std::string& name,
    const std::function<void(MaterialDesignatorBlueprintNode& material)>&
        callback) {
  auto material = std::make_shared<MaterialDesignatorBlueprintNode>(name);
  addChild(material);
  if (callback) {
    callback(*material);
  }
  return *material;
}

LayerBlueprintNode& BlueprintNode::addLayer(
    const std::string& name,
    const std::function<void(LayerBlueprintNode& layer)>& callback) {
  auto layer = std::make_shared<LayerBlueprintNode>(name);
  addChild(layer);
  if (callback) {
    callback(*layer);
  }
  return *layer;
}

void BlueprintNode::clearChildren() {
  for (auto& child : children()) {
    child.setDepth(0);
  }
  m_children.clear();
}

void BlueprintNode::graphviz(std::ostream& os) const {
  os << "digraph BlueprintNode {" << std::endl;
  addToGraphviz(os);
  os << "}" << std::endl;
}

void BlueprintNode::addToGraphviz(std::ostream& os) const {
  for (const auto& child : children()) {
    os << indent() << "\"" << name() << "\" -> \"" << child.name() << "\";"
       << std::endl;
    child.addToGraphviz(os);
  }
}

}  // namespace Acts
