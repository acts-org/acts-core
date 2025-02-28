// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/StaticBlueprintNode.hpp"

#include "Acts/Geometry/CylinderPortalShell.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/VolumeBounds.hpp"
#include "Acts/Navigation/INavigationPolicy.hpp"
#include "Acts/Utilities/GraphViz.hpp"
#include "Acts/Visualization/GeometryView3D.hpp"

namespace Acts::Experimental {

StaticBlueprintNode::StaticBlueprintNode(std::unique_ptr<TrackingVolume> volume)
    : m_volume(std::move(volume)) {}

Volume& StaticBlueprintNode::build(const BlueprintOptions& options,
                                   const GeometryContext& gctx,
                                   const Logger& logger) {
  ACTS_DEBUG(prefix() << "static build");
  if (!m_volume) {
    throw std::runtime_error("Volume is not built");
  }

  ACTS_DEBUG(prefix() << "Building volume (" << name() << ") with "
                      << children().size() << " children");
  for (auto& child : children()) {
    child.build(options, gctx, logger);
  }

  ACTS_DEBUG(prefix() << "-> returning volume " << *m_volume);
  return *m_volume;
}

PortalShellBase& StaticBlueprintNode::connect(const BlueprintOptions& options,
                                              const GeometryContext& gctx,
                                              const Logger& logger) {
  ACTS_DEBUG(prefix() << "Static connect");
  if (m_volume == nullptr) {
    throw std::runtime_error("Volume is not present");
  }

  ACTS_DEBUG(prefix() << "Connecting parent volume (" << name() << ") with "
                      << children().size() << " children");

  for (auto& child : children()) {
    auto& shell = child.connect(options, gctx, logger);
    // Register ourselves on the outside of the shell
    shell.fill(*m_volume);
  }

  VolumeBounds::BoundsType type = m_volume->volumeBounds().type();
  if (type == VolumeBounds::eCylinder) {
    m_shell = std::make_unique<SingleCylinderPortalShell>(*m_volume);

  } else if (type == VolumeBounds::eCuboid) {
    throw std::logic_error("Cuboid is not implemented yet");

  } else {
    throw std::logic_error("Volume type is not supported");
  }

  assert(m_shell != nullptr &&
         "No shell was built at the end of StaticBlueprintNode::connect");
  assert(m_shell->isValid() &&
         "Shell is not valid at the end of StaticBlueprintNode::connect");
  return *m_shell;
}

void StaticBlueprintNode::finalize(const BlueprintOptions& options,
                                   const GeometryContext& gctx,
                                   TrackingVolume& parent,
                                   const Logger& logger) {
  ACTS_DEBUG(prefix() << "Finalizing static volume");

  if (!m_volume) {
    ACTS_ERROR(prefix() << "Volume is not built");
    throw std::runtime_error("Volume is not built");
  }

  if (!m_shell) {
    ACTS_ERROR(prefix() << "Shell is not built");
    throw std::runtime_error("Shell is not built");
  }

  for (auto& child : children()) {
    child.finalize(options, gctx, *m_volume, logger);
  }

  ACTS_DEBUG(prefix() << "Registering " << m_shell->size()
                      << " portals into volume " << m_volume->volumeName());
  m_shell->applyToVolume();

  ACTS_DEBUG(prefix() << " Adding volume (" << m_volume->volumeName()
                      << ") to parent volume (" << parent.volumeName() << ")");

  const auto* policyFactory = options.defaultNavigationPolicyFactory.get();

  if (m_navigationPolicyFactory) {
    policyFactory = m_navigationPolicyFactory.get();
  }
  m_volume->setNavigationPolicy(policyFactory->build(gctx, *m_volume, logger));

  parent.addVolume(std::move(m_volume));
}

const std::string& StaticBlueprintNode::name() const {
  static const std::string uninitialized = "uninitialized";
  if (m_volume == nullptr) {
    return uninitialized;
  }
  return m_volume->volumeName();
}

StaticBlueprintNode& StaticBlueprintNode::setNavigationPolicyFactory(
    std::shared_ptr<NavigationPolicyFactory> navigationPolicyFactory) {
  m_navigationPolicyFactory = std::move(navigationPolicyFactory);
  return *this;
}

const NavigationPolicyFactory* StaticBlueprintNode::navigationPolicyFactory()
    const {
  return m_navigationPolicyFactory.get();
}

void StaticBlueprintNode::addToGraphviz(std::ostream& os) const {
  std::stringstream ss;
  ss << "<b>" << name() << "</b>";
  ss << "<br/>";
  if (m_volume == nullptr) {
    throw std::runtime_error("Volume is not built");
  }
  switch (m_volume->volumeBounds().type()) {
    case VolumeBounds::eCylinder:
      ss << "Cylinder";
      break;
    case VolumeBounds::eCuboid:
      ss << "Cuboid";
      break;
    case VolumeBounds::eCone:
      ss << "Cone";
      break;
    case VolumeBounds::eCutoutCylinder:
      ss << "CutoutCylinder";
      break;
    case VolumeBounds::eGenericCuboid:
      ss << "GenericCuboid";
      break;
    case VolumeBounds::eTrapezoid:
      ss << "Trapezoid";
      break;
    default:
      ss << "Other";
  }

  GraphViz::Node node{
      .id = name(), .label = ss.str(), .shape = GraphViz::Shape::Rectangle};

  os << node;

  BlueprintNode::addToGraphviz(os);
}

}  // namespace Acts::Experimental
