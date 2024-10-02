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
  // Retrieve the surface
  auto identifier = detElement->identifier();
  const Acts::Surface& surface = detElement->surface();
  const Acts::SurfaceBounds& bounds = surface.bounds();
  if (bounds.type() != Acts::SurfaceBounds::eRectangle || nSegments <= 1u) {
    ACTS_WARNING("Invalid splitting config for barrel node: " +
                 std::string(detElement->tgeoNode().GetName()) +
                 "! Node will not be slpit.");
    return {detElement};
  }

  // Output container for the submodules
  std::vector<std::shared_ptr<const Acts::TGeoDetectorElement>> detElements =
      {};
  detElements.reserve(nSegments);

  // Get the geometric information
  double thickness = detElement->thickness();
  const Acts::Transform3& transform = surface.transform(gctx);
  // Determine the new bounds
  const std::vector<double> boundsValues = bounds.values();
  double lengthX = (boundsValues[Acts::RectangleBounds::eMaxX] -
                    boundsValues[Acts::RectangleBounds::eMinX]) /
                   nSegments;
  double lengthY = boundsValues[Acts::RectangleBounds::eMaxY] -
                   boundsValues[Acts::RectangleBounds::eMinY];
  auto rectBounds =
      std::make_shared<Acts::RectangleBounds>(0.5 * lengthX, 0.5 * lengthY);
  // Translation for every subelement
  auto localTranslation = Acts::Vector2(-0.5 * lengthX * (nSegments - 1), 0.);
  const auto step = Acts::Vector2(lengthX, 0.);
  ACTS_DEBUG("Rectangle bounds for new node (half length): " +
             std::to_string(rectBounds->halfLengthX()) + ", " +
             std::to_string(rectBounds->halfLengthY()));

  for (std::size_t i = 0; i < nSegments; i++) {
    Acts::Vector3 globalTranslation =
        surface.localToGlobal(gctx, localTranslation, {}) -
        transform.translation();
    auto elemTransform =
        Acts::Transform3(transform).pretranslate(globalTranslation);
    auto element = std::make_shared<const Acts::TGeoDetectorElement>(
        identifier, detElement->tgeoNode(), elemTransform, rectBounds,
        thickness);
    detElements.push_back(std::move(element));

    localTranslation += step;
  }
  return detElements;
}

/// If applicable, returns a split detector element
inline std::vector<std::shared_ptr<const Acts::TGeoDetectorElement>>
ActsExamples::TGeoITkModuleSplitter::splitDiscModule(
    const Acts::GeometryContext& gctx,
    const std::shared_ptr<const Acts::TGeoDetectorElement>& detElement,
    const std::vector<ActsExamples::TGeoITkModuleSplitter::SplitRange>&
        splitRanges) const {
  // Retrieve the surface
  auto identifier = detElement->identifier();
  const Acts::Surface& surface = detElement->surface();
  const Acts::SurfaceBounds& bounds = surface.bounds();

  // Check annulus bounds origin
  auto printOrigin = [&](const Acts::Surface& sf) {
    Acts::Vector3 discOrigin =
        sf.localToGlobal(gctx, Acts::Vector2(0., 0.), Acts::Vector3::Zero());
    std::string out =
        "Disc surface origin at: " + std::to_string(discOrigin[0]) + ", " +
        std::to_string(discOrigin[1]) + ", " + std::to_string(discOrigin[2]);
    return out;
  };
  ACTS_DEBUG(printOrigin(surface));

  if (bounds.type() != Acts::SurfaceBounds::eAnnulus || splitRanges.empty()) {
    ACTS_WARNING("Invalid splitting config for disk node: " +
                 std::string(detElement->tgeoNode().GetName()) +
                 "! Node will not be slpit.");
    return {detElement};
  }

  auto nSegments = splitRanges.size();

  // Output container for the submodules
  std::vector<std::shared_ptr<const Acts::TGeoDetectorElement>> detElements =
      {};
  detElements.reserve(nSegments);

  // Get the geometric information
  double thickness = detElement->thickness();
  const Acts::Transform3& transform = surface.transform(gctx);
  const std::vector<double> boundsValues = bounds.values();
  std::array<double, Acts::AnnulusBounds::eSize> values{};
  std::copy_n(boundsValues.begin(), Acts::AnnulusBounds::eSize, values.begin());

  for (std::size_t i = 0; i < nSegments; i++) {
    values[Acts::AnnulusBounds::eMinR] = splitRanges[i].first;
    values[Acts::AnnulusBounds::eMaxR] = splitRanges[i].second;
    auto annulusBounds = std::make_shared<Acts::AnnulusBounds>(values);
    ACTS_DEBUG(
        "New r bounds for node: " + std::to_string(annulusBounds->rMin()) +
        ", " + std::to_string(annulusBounds->rMax()));

    auto element = std::make_shared<const Acts::TGeoDetectorElement>(
        identifier, detElement->tgeoNode(), transform, annulusBounds,
        thickness);
    detElements.push_back(std::move(element));
  }
  return detElements;
}
