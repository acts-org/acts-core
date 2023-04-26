// This file is part of the Acts project.
//
// Copyright (C) 2023 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Detector/CylindricalContainerBuilder.hpp"

#include "Acts/Detector/DetectorComponents.hpp"
#include "Acts/Detector/detail/CylindricalDetectorHelper.hpp"
#include "Acts/Utilities/BinningData.hpp"

#include <stdexcept>

namespace {

/// @brief Helper method to connect/wrap volumes or containers
///
/// @tparam object_collection either a vector of volumes or containers
/// @param gctx The geometry context of the this call
/// @param objects The object vector
/// @param binnning the chosen binning
/// @param logLevel the logging output level
///
/// @return a newly built container
template <typename object_collection>
Acts::Experimental::DetectorComponent::Container connect(
    const Acts::GeometryContext& gctx, object_collection& objects,
    const std::vector<Acts::BinningValue>& binning,
    Acts::Logging::Level logLevel) {
  // Return container object
  Acts::Experimental::DetectorComponent::Container rContainer;
  if (binning.size() == 1u) {
    Acts::BinningValue bv = binning.front();
    // 1-dimensional binning options
    switch (bv) {
      case Acts::binR: {
        rContainer =
            Acts::Experimental::detail::CylindricalDetectorHelper::connectInR(
                gctx, objects, {}, logLevel);
      } break;
      case Acts::binZ: {
        rContainer =
            Acts::Experimental::detail::CylindricalDetectorHelper::connectInZ(
                gctx, objects, {}, logLevel);
      } break;
      case Acts::binPhi: {
        rContainer =
            Acts::Experimental::detail::CylindricalDetectorHelper::connectInPhi(
                gctx, objects, {}, logLevel);
      } break;
      default: {
        throw std::invalid_argument(
            "CylinderContainerBuilder: binning not supported.");
      }
    }
  } else if (binning ==
                 std::vector<Acts::BinningValue>{Acts::binZ, Acts::binR} and
             objects.size() == 2u) {
    rContainer =
        Acts::Experimental::detail::CylindricalDetectorHelper::wrapInZR(
            gctx, objects, logLevel);
  } else {
    throw std::invalid_argument(
        "CylinderContainerBuilder: binning not supported.");
  }
  return rContainer;
}
}  // namespace

Acts::Experimental::CylindricalContainerBuilder::CylindricalContainerBuilder(
    const Acts::Experimental::CylindricalContainerBuilder::Config& cfg,
    std::unique_ptr<const Acts::Logger> logger)
    : IDetectorComponentBuilder(), m_cfg(cfg), m_logger(std::move(logger)) {
  if (m_cfg.builders.empty()) {
    throw std::invalid_argument(
        "CylindricalContainerBuilder: no sub builders provided.");
  }
}

Acts::Experimental::DetectorComponent
Acts::Experimental::CylindricalContainerBuilder::construct(
    RootDetectorVolumes& roots, const GeometryContext& gctx) const {
  // Return container object
  DetectorComponent::Container rContainer;

  // Create the indivudal components, collect for both outcomes
  std::vector<DetectorComponent> components;
  ACTS_DEBUG("Building container from " << m_cfg.builders.size()
                                        << " components.");
  // Check through the component volumes - if every builder only
  // built exactly one volume, you are at pure navigation level
  bool atNavigationLevel = true;
  components.reserve(m_cfg.builders.size());
  std::for_each(
      m_cfg.builders.begin(), m_cfg.builders.end(), [&](const auto& builder) {
        auto cmp = builder->construct(roots, gctx);
        atNavigationLevel = (atNavigationLevel and cmp.volumes.size() == 1u);
        components.push_back(cmp);
      });
  // Navigation level detected, connect volumes (cleaner and faster than
  // connect containers)
  if (atNavigationLevel) {
    ACTS_VERBOSE(
        "Component volumes are at navigation level: connecting volumes.");
    std::vector<std::shared_ptr<DetectorVolume>> volumes;
    volumes.reserve(components.size());
    std::for_each(components.begin(), components.end(), [&](const auto& cmp) {
      volumes.push_back(cmp.volumes.front());
    });
    // Connect volumes
    rContainer = connect(gctx, volumes, m_cfg.binning, logger().level());
  } else {
    ACTS_VERBOSE("Components contain sub containers: connect containers.");
    std::vector<DetectorComponent::Container> containers;
    containers.reserve(components.size());
    std::for_each(components.begin(), components.end(),
                  [&](const auto& cmp) { containers.push_back(cmp.portals); });
    // Connect containers
    rContainer = connect(gctx, containers, m_cfg.binning, logger().level());
  }
  // Return the container
  return Acts::Experimental::DetectorComponent{{}, rContainer};
}
