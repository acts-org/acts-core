// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/CylinderContainerBlueprintNode.hpp"

#include "Acts/Geometry/BlueprintNode.hpp"
#include "Acts/Geometry/CylinderVolumeStack.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/PortalShell.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Navigation/INavigationPolicy.hpp"
#include "Acts/Utilities/GraphViz.hpp"
#include "Acts/Visualization/GeometryView3D.hpp"

#include <algorithm>
#include <utility>

namespace Acts {

CylinderContainerBlueprintNode::CylinderContainerBlueprintNode(
    const std::string& name, AxisDirection direction,
    VolumeAttachmentStrategy attachmentStrategy,
    VolumeResizeStrategy resizeStrategy)
    : m_name(name),
      m_direction(direction),
      m_attachmentStrategy(attachmentStrategy),
      m_resizeStrategy(resizeStrategy) {}

const std::string& CylinderContainerBlueprintNode::name() const {
  return m_name;
}

Volume& CylinderContainerBlueprintNode::build(const BlueprintOptions& options,
                                              const GeometryContext& gctx,
                                              const Logger& logger) {
  ACTS_DEBUG(prefix() << "cylinder container build (dir=" << m_direction
                      << ")");

  if (m_stack != nullptr) {
    ACTS_ERROR(prefix() << "Volume is already built");
    throw std::runtime_error("Volume is already built");
  }

  for (auto& child : children()) {
    Volume& volume = child.build(options, gctx, logger);
    m_childVolumes.push_back(&volume);
    // We need to remember which volume we got from which child, so we can
    // assemble a correct portal shell later
    m_volumeToNode[&volume] = &child;
  }
  ACTS_VERBOSE(prefix() << "-> Collected " << m_childVolumes.size()
                        << " child volumes");

  ACTS_VERBOSE(prefix() << "-> Building the stack");
  m_stack = std::make_unique<CylinderVolumeStack>(m_childVolumes, m_direction,
                                                  m_attachmentStrategy,
                                                  m_resizeStrategy, logger);
  ACTS_DEBUG(prefix() << "-> Stack bounds are: " << m_stack->volumeBounds());

  ACTS_DEBUG(prefix() << " *** build complete ***");

  return *m_stack;
}

std::vector<CylinderPortalShell*>
CylinderContainerBlueprintNode::collectChildShells(
    const BlueprintOptions& options, const GeometryContext& gctx,
    const Logger& logger) {
  std::vector<CylinderPortalShell*> shells;
  ACTS_DEBUG(prefix() << "Have " << m_childVolumes.size() << " child volumes");
  for (Volume* volume : m_childVolumes) {
    if (isGapVolume(*volume)) {
      // We need to create a TrackingVolume from the gap and put it in the shell
      auto gap = std::make_unique<TrackingVolume>(*volume);
      gap->setVolumeName(name() + "::Gap" + std::to_string(m_gaps.size() + 1));
      ACTS_DEBUG(prefix() << " ~> Gap volume (" << gap->volumeName()
                          << "): " << gap->volumeBounds());
      auto shell = std::make_unique<SingleCylinderPortalShell>(*gap);
      assert(shell->isValid());
      shells.push_back(shell.get());

      m_gaps.emplace_back(std::move(shell), std::move(gap));

    } else {
      // Figure out which child we got this volume from
      auto it = m_volumeToNode.find(volume);
      if (it == m_volumeToNode.end()) {
        throw std::runtime_error("Volume not found in child volumes");
      }
      BlueprintNode& child = *it->second;

      ACTS_DEBUG(prefix() << " ~> Child (" << child.name()
                          << ") volume: " << volume->volumeBounds());

      auto* shell = dynamic_cast<CylinderPortalShell*>(
          &child.connect(options, gctx, logger));
      if (shell == nullptr) {
        ACTS_ERROR(prefix()
                   << "Child volume of cylinder stack is not a cylinder");
        throw std::runtime_error(
            "Child volume of cylinder stack is not a cylinder");
      }
      assert(shell->isValid());

      shells.push_back(shell);
    }
  }
  return shells;
}

CylinderStackPortalShell& CylinderContainerBlueprintNode::connect(
    const BlueprintOptions& options, const GeometryContext& gctx,
    const Logger& logger) {
  ACTS_DEBUG(prefix() << "Cylinder container connect");
  if (m_stack == nullptr) {
    ACTS_ERROR(prefix() << "Volume is not built");
    throw std::runtime_error("Volume is not built");
  }

  ACTS_DEBUG(prefix() << "Collecting child shells from " << children().size()
                      << " children");

  // We have child volumes and gaps as bare Volumes in `m_childVolumes` after
  // `build()` has completed. For the stack shell, we need TrackingVolumes in
  // the right order.

  std::vector<CylinderPortalShell*> shells =
      collectChildShells(options, gctx, logger);

  // Sanity checks
  throw_assert(shells.size() == m_childVolumes.size(),
               "Number of shells does not match number of child volumes");

  throw_assert(std::ranges::none_of(
                   shells, [](const auto* shell) { return shell == nullptr; }),
               "Invalid shell pointer");

  throw_assert(std::ranges::all_of(
                   shells, [](const auto* shell) { return shell->isValid(); }),
               "Invalid shell");

  ACTS_DEBUG(prefix() << "Producing merged cylinder stack shell in "
                      << m_direction << " direction from " << shells.size()
                      << " shells");
  m_shell = std::make_unique<CylinderStackPortalShell>(gctx, std::move(shells),
                                                       m_direction, logger);

  assert(m_shell != nullptr && "No shell was built at the end of connect");
  assert(m_shell->isValid() && "Shell is not valid at the end of connect");
  return *m_shell;
}

void CylinderContainerBlueprintNode::finalize(const BlueprintOptions& options,
                                              const GeometryContext& gctx,
                                              TrackingVolume& parent,
                                              const Logger& logger) {
  ACTS_DEBUG(prefix() << "Finalizing cylinder container");

  if (m_stack == nullptr) {
    ACTS_ERROR(prefix() << "Volume is not built");
    throw std::runtime_error("Volume is not built");
  }

  if (m_shell == nullptr) {
    ACTS_ERROR(prefix() << "Volume is not connected");
    throw std::runtime_error("Volume is not connected");
  }

  const auto* policyFactory = options.defaultNavigationPolicyFactory.get();

  ACTS_DEBUG(prefix() << "Registering " << m_gaps.size()
                      << " gap volumes with parent");
  for (auto& [shell, gap] : m_gaps) {
    auto* gapPtr = gap.get();
    parent.addVolume(std::move(gap));
    shell->applyToVolume();
    auto policy = policyFactory->build(gctx, *gapPtr, logger);
    gapPtr->setNavigationPolicy(std::move(policy));
  }

  ACTS_DEBUG(prefix() << "Finalizing " << children().size() << " children");

  for (auto& child : children()) {
    child.finalize(options, gctx, parent, logger);
  }
}

bool CylinderContainerBlueprintNode::isGapVolume(const Volume& volume) const {
  assert(m_stack != nullptr);
  return std::ranges::any_of(
      m_stack->gaps(), [&](const auto& gap) { return gap.get() == &volume; });
}

CylinderContainerBlueprintNode& CylinderContainerBlueprintNode::setDirection(
    AxisDirection direction) {
  if (m_stack != nullptr) {
    throw std::runtime_error("Cannot change direction after build");
  }
  m_direction = direction;
  return *this;
}

CylinderContainerBlueprintNode&
CylinderContainerBlueprintNode::setAttachmentStrategy(
    VolumeAttachmentStrategy attachmentStrategy) {
  if (m_stack != nullptr) {
    throw std::runtime_error("Cannot change direction after build");
  }
  m_attachmentStrategy = attachmentStrategy;
  return *this;
}

CylinderContainerBlueprintNode&
CylinderContainerBlueprintNode::setResizeStrategy(
    VolumeResizeStrategy resizeStrategy) {
  if (m_stack != nullptr) {
    throw std::runtime_error("Cannot change direction after build");
  }
  m_resizeStrategy = resizeStrategy;
  return *this;
}

void CylinderContainerBlueprintNode::addToGraphviz(std::ostream& os) const {
  std::stringstream ss;
  ss << "<b>" + name() + "</b>";
  ss << "<br/>CylinderContainer";
  ss << "<br/>dir: " << m_direction;
  GraphViz::Node node{
      .id = name(), .label = ss.str(), .shape = GraphViz::Shape::DoubleOctagon};
  os << node << std::endl;
  for (const auto& child : children()) {
    os << indent() << GraphViz::Edge{{.id = name()}, {.id = child.name()}}
       << std::endl;
    child.addToGraphviz(os);
  }
}

AxisDirection CylinderContainerBlueprintNode::direction() const {
  return m_direction;
}

VolumeAttachmentStrategy CylinderContainerBlueprintNode::attachmentStrategy()
    const {
  return m_attachmentStrategy;
}

VolumeResizeStrategy CylinderContainerBlueprintNode::resizeStrategy() const {
  return m_resizeStrategy;
}

}  // namespace Acts
