// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Detector/CuboidalContainerBuilder.hpp"

#include "Acts/Detector/DetectorComponents.hpp"
#include "Acts/Detector/DetectorVolumeBuilder.hpp"
#include "Acts/Detector/VolumeStructureBuilder.hpp"
#include "Acts/Detector/detail/CuboidalDetectorHelper.hpp"
#include "Acts/Detector/interface/IGeometryIdGenerator.hpp"
#include "Acts/Detector/interface/IRootVolumeFinderBuilder.hpp"
#include "Acts/Navigation/DetectorVolumeFinders.hpp"

#include <algorithm>
#include <ostream>
#include <stdexcept>
#include <utility>

namespace Acts::Experimental {
class DetectorVolume;
}  // namespace Acts::Experimental

Acts::Experimental::CuboidalContainerBuilder::CuboidalContainerBuilder(
    const Acts::Experimental::CuboidalContainerBuilder::Config& cfg,
    std::unique_ptr<const Acts::Logger> logger)
    : IDetectorComponentBuilder(), m_cfg(cfg), m_logger(std::move(logger)) {
  // Check if builders are present
  if (m_cfg.builders.empty()) {
    throw std::invalid_argument(
        "CuboidalContainerBuilder: no sub builders provided.");
  }
  // Check if binning value is correctly chosen
  if (m_cfg.binning != Acts::BinningValue::binX &&
      m_cfg.binning != Acts::BinningValue::binY &&
      m_cfg.binning != Acts::BinningValue::binZ) {
    throw std::invalid_argument(
        "CuboidalContainerBuilder: Invalid binning value. Only "
        "Acts::BinningValue::binX, "
        "Acts::BinningValue::binY, Acts::BinningValue::binZ are supported.");
  }
}

Acts::Experimental::CuboidalContainerBuilder::CuboidalContainerBuilder(
    const Acts::Experimental::Blueprint::Node& bpNode,
    Acts::Logging::Level logLevel)
    : IDetectorComponentBuilder(),
      m_logger(getDefaultLogger(bpNode.name + "_cont", logLevel)) {
  if (bpNode.boundsType != VolumeBounds::BoundsType::eCuboid) {
    throw std::invalid_argument(
        "CuboidalContainerBuilder: boundary type must be cuboid - for "
        "building from a blueprint node.");
  }

  std::vector<std::shared_ptr<const IDetectorComponentBuilder>> builders;
  for (const auto& child : bpNode.children) {
    if (child->isLeaf()) {
      // Volume structure
      VolumeStructureBuilder::Config vsCfg;
      vsCfg.transform = child->transform;
      vsCfg.boundsType = child->boundsType;
      vsCfg.boundValues = child->boundaryValues;
      vsCfg.auxiliary = "*** acts auto-generated shape builder ***";
      auto vsBuilder = std::make_shared<VolumeStructureBuilder>(
          vsCfg, getDefaultLogger(child->name + "_shape", logLevel));
      // Detector volume builder
      DetectorVolumeBuilder::Config dvCfg;
      dvCfg.name = child->name;
      dvCfg.externalsBuilder = vsBuilder;
      dvCfg.internalsBuilder = child->internalsBuilder;
      dvCfg.auxiliary = "*** acts auto-generated volume builder ***";
      // Add the builder
      m_cfg.builders.push_back(std::make_shared<DetectorVolumeBuilder>(
          dvCfg, getDefaultLogger(child->name, logLevel)));
    } else {
      // This evokes the recursive stepping down the tree
      m_cfg.builders.push_back(
          std::make_shared<CuboidalContainerBuilder>(*child, logLevel));
    }
  }
  // Check if builders are present
  if (m_cfg.builders.empty()) {
    throw std::invalid_argument(
        "CuboidalContainerBuilder: no sub builders provided.");
  }
  if (bpNode.binning.size() != 1) {
    throw std::invalid_argument(
        "CuboidalContainerBuilder: >1D binning is not supported for cuboid "
        "containers.");
  }
  m_cfg.binning = bpNode.binning.at(0);
  // Check if binning value is correctly chosen
  if (m_cfg.binning != Acts::BinningValue::binX &&
      m_cfg.binning != Acts::BinningValue::binY &&
      m_cfg.binning != Acts::BinningValue::binZ) {
    throw std::invalid_argument(
        "CuboidalContainerBuilder: Invalid binning value. Only "
        "Acts::BinningValue::binX, "
        "Acts::BinningValue::binY, Acts::BinningValue::binZ are supported.");
  }

  m_cfg.auxiliary = "*** acts auto-generated from proxy ***";
  m_cfg.geoIdGenerator = bpNode.geoIdGenerator;
  m_cfg.rootVolumeFinderBuilder = bpNode.rootVolumeFinderBuilder;
}

Acts::Experimental::DetectorComponent
Acts::Experimental::CuboidalContainerBuilder::construct(
    const GeometryContext& gctx) const {
  // Return container object
  DetectorComponent::PortalContainer rContainer;
  bool atNavigationLevel = true;

  // Create the indivudal components, collect for both outcomes
  std::vector<DetectorComponent> components;
  ACTS_DEBUG("Building container from " << m_cfg.builders.size()
                                        << " components.");
  // Check through the component volumes - if every builder only
  // built exactly one volume, you are at pure navigation level
  // Collect the volumes
  std::vector<std::shared_ptr<DetectorVolume>> volumes;
  std::vector<DetectorComponent::PortalContainer> containers;
  std::vector<std::shared_ptr<DetectorVolume>> rootVolumes;
  // Run through the builders
  std::for_each(
      m_cfg.builders.begin(), m_cfg.builders.end(), [&](const auto& builder) {
        auto [cVolumes, cContainer, cRoots] = builder->construct(gctx);
        atNavigationLevel = (atNavigationLevel && cVolumes.size() == 1u);
        ACTS_VERBOSE("Number of volumes: " << cVolumes.size());
        // Collect individual components, volumes, containers, roots
        volumes.insert(volumes.end(), cVolumes.begin(), cVolumes.end());
        containers.push_back(cContainer);
        rootVolumes.insert(rootVolumes.end(), cRoots.volumes.begin(),
                           cRoots.volumes.end());
      });
  // Navigation level detected, connect volumes (cleaner and faster than
  // connect containers)
  if (atNavigationLevel) {
    ACTS_VERBOSE(
        "Component volumes are at navigation level: connecting volumes.");
    // Connect volumes
    rContainer = Acts::Experimental::detail::CuboidalDetectorHelper::connect(
        gctx, volumes, m_cfg.binning, {}, logger().level());

  } else {
    ACTS_VERBOSE("Components contain sub containers: connect containers.");
    // Connect containers
    rContainer = Acts::Experimental::detail::CuboidalDetectorHelper::connect(
        gctx, containers, m_cfg.binning, {}, logger().level());
  }
  ACTS_VERBOSE("Number of root volumes: " << rootVolumes.size());

  // Geometry Id generation
  if (m_cfg.geoIdGenerator != nullptr) {
    ACTS_DEBUG("Assigning geometry ids to the detector");
    auto cache = m_cfg.geoIdGenerator->generateCache();
    if (m_cfg.geoIdReverseGen) {
      std::for_each(rootVolumes.rbegin(), rootVolumes.rend(), [&](auto& v) {
        m_cfg.geoIdGenerator->assignGeometryId(cache, *v);
        ACTS_VERBOSE("-> Assigning geometry id to volume " << v->name());
      });
    } else {
      std::for_each(rootVolumes.begin(), rootVolumes.end(), [&](auto& v) {
        m_cfg.geoIdGenerator->assignGeometryId(cache, *v);
        ACTS_VERBOSE("-> Assigning geometry id to volume " << v->name());
      });
    }
  }

  // Check if a root volume finder is provided
  if (m_cfg.rootVolumeFinderBuilder) {
    // Return the container
    return Acts::Experimental::DetectorComponent{
        volumes, rContainer,
        RootDetectorVolumes{
            rootVolumes,
            m_cfg.rootVolumeFinderBuilder->construct(gctx, rootVolumes)}};
  }

  // Return the container
  return Acts::Experimental::DetectorComponent{
      volumes, rContainer, RootDetectorVolumes{rootVolumes, tryRootVolumes()}};
}
