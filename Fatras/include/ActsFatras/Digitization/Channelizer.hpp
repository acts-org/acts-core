// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "ActsFatras/Digitization/PlanarSurfaceDrift.hpp"
#include "ActsFatras/Digitization/PlanarSurfaceMask.hpp"
#include "ActsFatras/Digitization/Segmentizer.hpp"
#include "ActsFatras/EventData/Hit.hpp"

#include <numeric>

namespace ActsFatras {

/// @brief Class that ties the digitization modules together and produces the channels
class Channelizer {
  PlanarSurfaceDrift m_surfaceDrift;
  PlanarSurfaceMask m_surfaceMask;
  Segmentizer m_segmentizer;

 public:
  /// Do the geometric channelizing
  ///
  /// @param hit The hit we want to channelize
  /// @param surface the surface on which the hit is
  /// @param gctx the Geometry context
  /// @param driftDir the drift direction
  /// @param segmentation the segmentation of the surface
  /// @param thickness the thickness of the surface
  ///
  /// @return the list of channels
  Acts::Result<std::vector<Segmentizer::ChannelSegment>> channelize(
      const Hit& hit, const Acts::Surface& surface,
      const Acts::GeometryContext& gctx, const Acts::Vector3& driftDir,
      const Acts::BinUtility& segmentation, double thickness) const {
    auto driftedSegment = m_surfaceDrift.toReadout(
        gctx, surface, thickness, hit.position(), hit.direction(), driftDir);

    auto maskedSegmentRes = m_surfaceMask.apply(surface, driftedSegment);

    if (!maskedSegmentRes.ok()) {
      return maskedSegmentRes.error();
    }

    // Now Channelize
    auto segments =
        m_segmentizer.segments(gctx, surface, segmentation, *maskedSegmentRes);

    // Go from 2D-path to 3D-path by applying thickness
    const auto path2D = std::accumulate(
        segments.begin(), segments.end(), 0.0,
        [](double sum, const auto& seg) { return sum + seg.activation; });

    for (auto& seg : segments) {
      auto r = path2D != 0.0 ? (seg.activation / path2D) : 1.0;
      auto segThickness = r * thickness;

      seg.activation = std::hypot(segThickness, seg.activation);
    }

    return segments;
  }
};

}  // namespace ActsFatras
