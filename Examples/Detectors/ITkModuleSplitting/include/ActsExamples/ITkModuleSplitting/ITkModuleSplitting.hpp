// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// This file is part of the Acts project.
//
// Copyright (C) 2024 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Surfaces/AnnulusBounds.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <sstream>

namespace ActsExamples::ITk {

template <typename detector_element_t, typename element_factory_t>
inline std::vector<std::shared_ptr<const detector_element_t>> splitBarrelModule(
    const Acts::GeometryContext& gctx,
    const std::shared_ptr<const detector_element_t>& detElement,
    unsigned int nSegments, const element_factory_t& factory,
    const std::string& name,
    const Acts::Logger& logger = Acts::getDummyLogger()) {
  // Retrieve the surface
  const Acts::Surface& surface = detElement->surface();
  const Acts::SurfaceBounds& bounds = surface.bounds();
  if (bounds.type() != Acts::SurfaceBounds::eRectangle || nSegments <= 1u) {
    ACTS_WARNING("Invalid splitting config for barrel node: "
                 << name << "! Node will not be slpit.");
    return {detElement};
  }

  // Output container for the submodules
  std::vector<std::shared_ptr<const detector_element_t>> detElements = {};
  detElements.reserve(nSegments);

  // Get the geometric information
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
    detElements.emplace_back(factory(elemTransform, rectBounds));

    localTranslation += step;
  }
  return detElements;
}

template <typename detector_element_t, typename element_factory_t>
inline std::vector<std::shared_ptr<detector_element_t>> splitDiscModule(
    const Acts::GeometryContext& gctx,
    const std::shared_ptr<detector_element_t>& detElement,
    const std::vector<std::pair<double, double>>& splitRanges,
    const element_factory_t& factory, const std::string& name,
    const Acts::Logger& logger = Acts::getDummyLogger()) {
  // Retrieve the surface
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
    ACTS_WARNING("Invalid splitting config for disk node: "
                 << name << "! Node will not be slpit.");
    return {detElement};
  }

  auto nSegments = splitRanges.size();

  // Output container for the submodules
  std::vector<std::shared_ptr<const detector_element_t>> detElements = {};
  detElements.reserve(nSegments);

  // Get the geometric information
  const Acts::Transform3& transform = surface.transform(gctx);
  const std::vector<double> boundsValues = bounds.values();
  std::array<double, Acts::AnnulusBounds::eSize> values{};

  std::copy_n(boundsValues.begin(), Acts::AnnulusBounds::eSize, values.begin());

  for (std::size_t i = 0; i < nSegments; i++) {
    if (boundsValues[Acts::AnnulusBounds::eMinR] > splitRanges[i].first ||
        boundsValues[Acts::AnnulusBounds::eMaxR] < splitRanges[i].second) {
      ACTS_WARNING(
          "Radius pattern not within the original bounds, node will not be "
          "split!");
      return {detElement};
    }

    values[Acts::AnnulusBounds::eMinR] = splitRanges[i].first;
    values[Acts::AnnulusBounds::eMaxR] = splitRanges[i].second;
    auto annulusBounds = std::make_shared<Acts::AnnulusBounds>(values);
    ACTS_DEBUG(
        "New r bounds for node: " + std::to_string(annulusBounds->rMin()) +
        ", " + std::to_string(annulusBounds->rMax()));

    auto element = factory(transform, annulusBounds);
    detElements.push_back(std::move(element));
  }
  return detElements;
}

}  // namespace ActsExamples::ITk
