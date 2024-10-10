// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/TGeoDetector/TGeoITkModuleSplitter.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Plugins/TGeo/TGeoDetectorElement.hpp"
#include "Acts/Surfaces/AnnulusBounds.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "ActsExamples/ITkModuleSplitting/ITkModuleSplitting.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <sstream>

ActsExamples::TGeoITkModuleSplitter::TGeoITkModuleSplitter(
    const ActsExamples::TGeoITkModuleSplitter::Config& cfg,
    std::unique_ptr<const Acts::Logger> logger)
    : m_cfg(cfg), m_logger(std::move(logger)) {
  initSplitCategories();
}

void ActsExamples::TGeoITkModuleSplitter::initSplitCategories() {
  m_splitCategories.reserve(m_cfg.splitPatterns.size());
  for (const std::pair<const std::string, std::string>& pattern_split_category :
       m_cfg.splitPatterns) {
    // mark pattern for disc or barrel module splits:
    bool is_disk = false;
    if (m_cfg.discMap.contains(pattern_split_category.second)) {
      is_disk = true;
    } else if (!m_cfg.barrelMap.contains(pattern_split_category.second)) {
      ACTS_ERROR(
          pattern_split_category.second +
          " is neither a category name for barrel or disk module splits.");
      continue;
    }
    m_splitCategories.push_back(
        std::make_tuple(std::regex(pattern_split_category.first),
                        pattern_split_category.second, is_disk));
  }
}

/// If applicable, returns a split detector element
inline std::vector<std::shared_ptr<const Acts::TGeoDetectorElement>>
ActsExamples::TGeoITkModuleSplitter::split(
    const Acts::GeometryContext& gctx,
    std::shared_ptr<const Acts::TGeoDetectorElement> detElement) const {
  // Is the current node covered by this splitter?
  const TGeoNode& node = detElement->tgeoNode();
  auto sensorName = std::string(node.GetName());

  static const char* category_names[2] = {"barrel", "disc"};
  for (const std::tuple<std::regex, std::string, bool>& split_category :
       m_splitCategories) {
    if (std::regex_match(sensorName, std::get<0>(split_category))) {
      ACTS_DEBUG("Splitting " +
                 std::string(category_names[std::get<2>(split_category)]) +
                 " node " + sensorName + " using split ranges of category " +
                 std::get<1>(split_category));
      if (!std::get<2>(split_category)) {
        return ActsExamples::TGeoITkModuleSplitter::splitBarrelModule(
            gctx, detElement, m_cfg.barrelMap.at(std::get<1>(split_category)));
      } else {
        return ActsExamples::TGeoITkModuleSplitter::splitDiscModule(
            gctx, detElement, m_cfg.discMap.at(std::get<1>(split_category)));
      }
    }
  }
  ACTS_DEBUG("No matching configuration found. Node " +
             std::string(detElement->tgeoNode().GetName()) +
             " will not be split.");

  return {detElement};
}

/// If applicable, returns a split detector element
inline std::vector<std::shared_ptr<const Acts::TGeoDetectorElement>>
ActsExamples::TGeoITkModuleSplitter::splitBarrelModule(
    const Acts::GeometryContext& gctx,
    const std::shared_ptr<const Acts::TGeoDetectorElement>& detElement,
    unsigned int nSegments) const {
  auto name = detElement->tgeoNode().GetName();

  auto factory = [&](const auto& trafo, const auto& bounds) {
    return std::make_shared<const Acts::TGeoDetectorElement>(
        detElement->identifier(), detElement->tgeoNode(), trafo, bounds,
        detElement->thickness());
  };

  return ITk::splitBarrelModule(gctx, detElement, nSegments, factory, name,
                                logger());
}

/// If applicable, returns a split detector element
inline std::vector<std::shared_ptr<const Acts::TGeoDetectorElement>>
ActsExamples::TGeoITkModuleSplitter::splitDiscModule(
    const Acts::GeometryContext& gctx,
    const std::shared_ptr<const Acts::TGeoDetectorElement>& detElement,
    const std::vector<ActsExamples::TGeoITkModuleSplitter::SplitRange>&
        splitRanges) const {
  auto name = detElement->tgeoNode().GetName();

  auto factory = [&](const auto& trafo, const auto& bounds) {
    return std::make_shared<const Acts::TGeoDetectorElement>(
        detElement->identifier(), detElement->tgeoNode(), trafo, bounds,
        detElement->thickness());
  };

  return ITk::splitDiscModule(gctx, detElement, splitRanges, factory, name,
                              logger());
}
