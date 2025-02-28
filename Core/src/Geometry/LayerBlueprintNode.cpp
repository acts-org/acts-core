// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/LayerBlueprintNode.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/CuboidVolumeBounds.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/ProtoLayer.hpp"
#include "Acts/Geometry/VolumeBounds.hpp"
#include "Acts/Utilities/GraphViz.hpp"

namespace Acts::Experimental {

Volume& LayerBlueprintNode::build(const BlueprintOptions& options,
                                  const GeometryContext& gctx,
                                  const Logger& logger) {
  if (m_surfaces.empty()) {
    ACTS_ERROR("LayerBlueprintNode: no surfaces provided");
    throw std::invalid_argument("LayerBlueprintNode: no surfaces provided");
  }

  ACTS_DEBUG(prefix() << "Building Layer " << name() << " from "
                      << m_surfaces.size() << " surfaces");
  ACTS_VERBOSE(prefix() << " -> layer type: " << m_layerType);
  ACTS_VERBOSE(prefix() << " -> transform:\n" << m_transform.matrix());

  Extent extent;

  ProtoLayer protoLayer{gctx, m_surfaces, m_transform.inverse()};
  ACTS_VERBOSE(prefix() << "Built proto layer: " << protoLayer);

  extent.addConstrain(protoLayer.extent, m_envelope);

  ACTS_VERBOSE(prefix() << " -> layer extent: " << extent);

  buildVolume(extent, logger);
  assert(m_volume != nullptr && "Volume not built from proto layer");

  for (auto& surface : m_surfaces) {
    m_volume->addSurface(surface);
  }

  return StaticBlueprintNode::build(options, gctx, logger);
}

void LayerBlueprintNode::buildVolume(const Extent& extent,
                                     const Logger& logger) {
  ACTS_VERBOSE(prefix() << "Building volume for layer " << name());
  using enum AxisDirection;
  using enum LayerType;

  std::shared_ptr<VolumeBounds> bounds;
  switch (m_layerType) {
    case Cylinder:
    case Disc: {
      double minR = extent.min(AxisR);
      double maxR = extent.max(AxisR);
      double hlZ = extent.interval(AxisZ) / 2.0;
      bounds = std::make_shared<CylinderVolumeBounds>(minR, maxR, hlZ);
      break;
    }
    case Plane: {
      double hlX = extent.interval(AxisX) / 2.0;
      double hlY = extent.interval(AxisY) / 2.0;
      double hlZ = extent.interval(AxisZ) / 2.0;
      bounds = std::make_shared<CuboidVolumeBounds>(hlX, hlY, hlZ);
      break;
    }
  }

  assert(bounds != nullptr);

  ACTS_VERBOSE(prefix() << " -> bounds: " << *bounds);

  Transform3 transform = m_transform;
  transform.translation() =
      Vector3{extent.medium(AxisX), extent.medium(AxisY), extent.medium(AxisZ)};

  ACTS_VERBOSE(prefix() << " -> adjusted transform:\n" << transform.matrix());

  m_volume =
      std::make_unique<TrackingVolume>(transform, std::move(bounds), m_name);
}

const std::string& LayerBlueprintNode::name() const {
  return m_name;
}

LayerBlueprintNode& LayerBlueprintNode::setSurfaces(
    std::vector<std::shared_ptr<Surface>> surfaces) {
  m_surfaces = std::move(surfaces);
  return *this;
}

const std::vector<std::shared_ptr<Surface>>& LayerBlueprintNode::surfaces()
    const {
  return m_surfaces;
}

LayerBlueprintNode& LayerBlueprintNode::setTransform(
    const Transform3& transform) {
  m_transform = transform;
  return *this;
}

const Transform3& LayerBlueprintNode::transform() const {
  return m_transform;
}

LayerBlueprintNode& LayerBlueprintNode::setEnvelope(
    const ExtentEnvelope& envelope) {
  m_envelope = envelope;
  return *this;
}

const ExtentEnvelope& LayerBlueprintNode::envelope() const {
  return m_envelope;
}

LayerBlueprintNode& LayerBlueprintNode::setLayerType(LayerType layerType) {
  m_layerType = layerType;
  return *this;
}

const LayerBlueprintNode::LayerType& LayerBlueprintNode::layerType() const {
  return m_layerType;
}

void LayerBlueprintNode::addToGraphviz(std::ostream& os) const {
  std::stringstream ss;
  ss << "<b>" << name() << "</b>";
  ss << "<br/>";
  ss << m_layerType;

  GraphViz::Node node{
      .id = name(), .label = ss.str(), .shape = GraphViz::Shape::Diamond};

  os << node;

  BlueprintNode::addToGraphviz(os);
}

}  // namespace Acts::Experimental
