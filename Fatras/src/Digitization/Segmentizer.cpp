// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsFatras/Digitization/Segmentizer.hpp"

#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/detail/IntersectionHelper2D.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "Acts/Utilities/Intersection.hpp"

#include <algorithm>
#include <cmath>
#include <memory>

std::vector<ActsFatras::Segmentizer::ChannelSegment>
ActsFatras::Segmentizer::segments(const Acts::GeometryContext& geoCtx,
                                  const Acts::Surface& surface,
                                  const Acts::BinUtility& segmentation,
                                  const Segment2D& segment) const {
  // Return if the segmentation is not two-dimensional
  // (strips need to have one bin along the strip)
  if (segmentation.dimensions() != 2) {
    return {};
  }

  // Start and end point
  const auto& start = segment[0];
  const auto& end = segment[1];

  // Full path length - the full channel
  auto segment2d = (end - start);
  std::vector<ChannelStep> cSteps;
  Bin2D bstart = {0, 0};
  Bin2D bend = {0, 0};

  if (surface.type() == Acts::Surface::SurfaceType::Plane) {
    // Get the segmentation and convert it to lines & arcs
    bstart = {static_cast<unsigned int>(segmentation.bin(start, 0)),
              static_cast<unsigned int>(segmentation.bin(start, 1))};
    bend = {static_cast<unsigned int>(segmentation.bin(end, 0)),
            static_cast<unsigned int>(segmentation.bin(end, 1))};
    // Fast single channel exit
    if (bstart == bend) {
      return {ChannelSegment(bstart, {start, end}, segment2d.norm())};
    }
    // The lines channel segment lines along x
    if (bstart[0] != bend[0]) {
      double k = segment2d.y() / segment2d.x();
      double d = start.y() - k * start.x();

      const auto& xboundaries = segmentation.binningData()[0].boundaries();
      std::vector<double> xbbounds = {
          xboundaries.begin() + std::min(bstart[0], bend[0]) + 1,
          xboundaries.begin() + std::max(bstart[0], bend[0]) + 1};
      for (const auto x : xbbounds) {
        cSteps.push_back(ChannelStep{
            {(bstart[0] < bend[0] ? 1 : -1), 0}, {x, k * x + d}, start});
      }
    }
    // The lines channel segment lines along y
    if (bstart[1] != bend[1]) {
      double k = segment2d.x() / segment2d.y();
      double d = start.x() - k * start.y();
      const auto& yboundaries = segmentation.binningData()[1].boundaries();
      std::vector<double> ybbounds = {
          yboundaries.begin() + std::min(bstart[1], bend[1]) + 1,
          yboundaries.begin() + std::max(bstart[1], bend[1]) + 1};
      for (const auto y : ybbounds) {
        cSteps.push_back(ChannelStep{
            {0, (bstart[1] < bend[1] ? 1 : -1)}, {k * y + d, y}, start});
      }
    }

  } else if (surface.type() == Acts::Surface::SurfaceType::Disc) {
    Acts::Vector2 pstart(Acts::VectorHelpers::perp(start),
                         Acts::VectorHelpers::phi(start));
    Acts::Vector2 pend(Acts::VectorHelpers::perp(end),
                       Acts::VectorHelpers::phi(end));

    // Get the segmentation and convert it to lines & arcs
    bstart = {static_cast<unsigned int>(segmentation.bin(pstart, 0)),
              static_cast<unsigned int>(segmentation.bin(pstart, 1))};
    bend = {static_cast<unsigned int>(segmentation.bin(pend, 0)),
            static_cast<unsigned int>(segmentation.bin(pend, 1))};

    // Fast single channel exit
    if (bstart == bend) {
      return {ChannelSegment(bstart, {start, end}, segment2d.norm())};
    }

    double phistart = pstart[1];
    double phiend = pend[1];

    // The radial boundaries
    if (bstart[0] != bend[0]) {
      const auto& rboundaries = segmentation.binningData()[0].boundaries();
      std::vector<double> rbbounds = {
          rboundaries.begin() + std::min(bstart[0], bend[0]) + 1,
          rboundaries.begin() + std::max(bstart[0], bend[0]) + 1};
      for (const auto& r : rbbounds) {
        auto radIntersection =
            Acts::detail::IntersectionHelper2D::intersectCircleSegment(
                r, std::min(phistart, phiend), std::max(phistart, phiend),
                start, (end - start).normalized());
        cSteps.push_back(ChannelStep{{(bstart[0] < bend[0] ? 1 : -1), 0},
                                     radIntersection.position(),
                                     start});
      }
    }
    // The phi boundaries
    if (bstart[1] != bend[1]) {
      double referenceR =
          surface.binningPositionValue(geoCtx, Acts::BinningValue::binR);
      Acts::Vector2 origin = {0., 0.};
      const auto& phiboundaries = segmentation.binningData()[1].boundaries();
      std::vector<double> phibbounds = {
          phiboundaries.begin() + std::min(bstart[1], bend[1]) + 1,
          phiboundaries.begin() + std::max(bstart[1], bend[1]) + 1};

      for (const auto& phi : phibbounds) {
        Acts::Vector2 philine(referenceR * std::cos(phi),
                              referenceR * std::sin(phi));
        auto phiIntersection =
            Acts::detail::IntersectionHelper2D::intersectSegment(
                origin, philine, start, (end - start).normalized());
        cSteps.push_back(ChannelStep{{0, (bstart[1] < bend[1] ? 1 : -1)},
                                     phiIntersection.position(),
                                     start});
      }
    }
  }

  // Register the last step if successful
  if (!cSteps.empty()) {
    cSteps.push_back(ChannelStep({0, 0}, end, start));
    std::ranges::sort(cSteps, std::less<ChannelStep>{});
  }

  std::vector<ChannelSegment> cSegments;
  cSegments.reserve(cSteps.size());

  Bin2D currentBin = {bstart[0], bstart[1]};
  BinDelta2D lastDelta = {0, 0};
  Acts::Vector2 lastIntersect = start;
  double lastPath = 0.;
  for (auto& cStep : cSteps) {
    currentBin[0] += lastDelta[0];
    currentBin[1] += lastDelta[1];
    double path = cStep.path - lastPath;
    cSegments.push_back(
        ChannelSegment(currentBin, {lastIntersect, cStep.intersect}, path));
    lastPath = cStep.path;
    lastDelta = cStep.delta;
    lastIntersect = cStep.intersect;
  }

  return cSegments;
}
