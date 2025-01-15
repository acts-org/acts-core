// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/Blueprint.hpp"

#include "Acts/Geometry/CuboidVolumeBounds.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Geometry/PortalShell.hpp"
#include "Acts/Geometry/VolumeBounds.hpp"
#include "Acts/Navigation/INavigationPolicy.hpp"
#include "Acts/Utilities/GraphViz.hpp"

namespace {
const std::string s_rootName = "Root";
}

namespace Acts {

Blueprint::Blueprint(const Config &config) : m_cfg(config) {}

const std::string &Blueprint::name() const {
  return s_rootName;
}

Volume &Blueprint::build(const BlueprintOptions & /*options*/,
                         const GeometryContext & /*gctx*/,
                         const Logger & /*logger*/) {
  throw std::logic_error("Root node cannot be built");
}

PortalShellBase &Blueprint::connect(const BlueprintOptions & /*options*/,
                                    const GeometryContext & /*gctx*/,
                                    const Logger & /*logger*/) {
  throw std::logic_error("Root node cannot be connected");
}

void Blueprint::finalize(const BlueprintOptions & /*options*/,
                         const GeometryContext & /*gctx*/,
                         TrackingVolume & /*parent*/,
                         const Logger & /*logger*/) {
  throw std::logic_error("Root node cannot be finalized");
}

void Blueprint::addToGraphviz(std::ostream &os) const {
  GraphViz::Node node{
      .id = name(), .label = "World", .shape = GraphViz::Shape::House};

  os << node;
  BlueprintNode::addToGraphviz(os);
}

std::unique_ptr<TrackingGeometry> Blueprint::construct(
    const BlueprintOptions &options, const GeometryContext &gctx,
    const Logger &logger) {
  using enum AxisDirection;

  ACTS_INFO(prefix() << "Building tracking geometry from blueprint tree");

  options.validate();

  if (m_cfg.envelope == ExtentEnvelope::Zero()) {
    ACTS_WARNING(prefix() << "Root node is configured with zero envelope. This "
                             "might lead to navigation issues");
  }

  if (children().size() != 1) {
    ACTS_ERROR(prefix() << "Root node must have exactly one child");
    throw std::logic_error("Root node must have exactly one child");
  }

  auto &child = children().at(0);

  ACTS_DEBUG(prefix() << "Executing building on tree");
  Volume &topVolume = child.build(options, gctx, logger);
  const auto &bounds = topVolume.volumeBounds();

  std::stringstream ss;
  bounds.toStream(ss);
  ACTS_DEBUG(prefix() << "have top volume: " << ss.str() << "\n"
                      << topVolume.transform().matrix());

  std::shared_ptr<VolumeBounds> worldBounds;

  if (const auto *cyl = dynamic_cast<const CylinderVolumeBounds *>(&bounds);
      cyl != nullptr) {
    using enum CylinderVolumeBounds::BoundValues;

    // Make a copy that we'll modify
    auto newBounds = std::make_shared<CylinderVolumeBounds>(*cyl);

    const auto &zEnv = m_cfg.envelope[AxisZ];
    if (zEnv[0] != zEnv[1]) {
      ACTS_ERROR(
          prefix() << "Root node cylinder envelope for z must be symmetric");
      throw std::logic_error(
          "Root node cylinder envelope for z must be "
          "symmetric");
    }

    const auto &rEnv = m_cfg.envelope[AxisR];

    newBounds->set({
        {eHalfLengthZ, newBounds->get(eHalfLengthZ) + zEnv[0]},
        {eMinR, std::max(0.0, newBounds->get(eMinR) - rEnv[0])},
        {eMaxR, newBounds->get(eMaxR) + rEnv[1]},
    });

    worldBounds = std::move(newBounds);

  } else if (const auto *box =
                 dynamic_cast<const CuboidVolumeBounds *>(&bounds);
             box != nullptr) {
    throw std::logic_error{"Not implemented"};
  } else {
    throw std::logic_error{"Unsupported volume bounds type"};
  }

  ACTS_DEBUG(prefix() << "New root volume bounds are: " << *worldBounds);

  auto world = std::make_unique<TrackingVolume>(
      topVolume.transform(), std::move(worldBounds), "World");

  // @TODO: This needs to become configurable
  world->setNavigationPolicy(
      options.defaultNavigationPolicyFactory->build(gctx, *world, logger));

  // Need one-sided portal shell that connects outwards to nullptr
  SingleCylinderPortalShell worldShell{*world};
  worldShell.applyToVolume();

  auto &shell = child.connect(options, gctx, logger);

  shell.fill(*world);

  child.finalize(options, gctx, *world, logger);

  std::set<std::string, std::less<>> names;

  world->visitVolumes([&names, &logger, this](const auto *volume) {
    if (names.contains(volume->volumeName())) {
      ACTS_ERROR(prefix() << "Duplicate volume name: " << volume->volumeName());
      throw std::logic_error("Duplicate volume name");
    }
    names.insert(volume->volumeName());
  });

  return std::make_unique<TrackingGeometry>(
      std::move(world), nullptr, m_cfg.geometryIdentifierHook, logger);
}

}  // namespace Acts
