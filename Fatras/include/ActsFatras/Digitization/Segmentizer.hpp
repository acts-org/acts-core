// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"

#include <array>
#include <utility>
#include <vector>

namespace Acts {
class BinUtility;
class Surface;
}  // namespace Acts

namespace ActsFatras {

/// The Segmentizer splits a surface segment, i.e. after projection
/// onto the readout surface into channel segments.
///
struct Segmentizer {
  /// Shorthand for a 2D segment
  using Segment2D = std::array<Acts::Vector2, 2>;
  /// Shorthand for a 2D bin
  using Bin2D = std::array<unsigned int, 2>;
  /// shorthand for a 2D bin delta
  using BinDelta2D = std::array<int, 2>;

  /// Nested struct for stepping from one channel to the next.
  struct ChannelStep {
    /// This is the delta to the last step in bins
    BinDelta2D delta = {0, 0};
    /// The intersection with the channel boundary
    Acts::Vector2 intersect;
    /// The patlength from the start
    double path = 0.;

    /// Constructor with arguments for a ChannelStep.
    ///
    /// @param delta_ The bin delta for this step
    /// @param intersect_ The intersect with the channel boundary
    /// @param start The start of the surface segment, for path from origin
    ChannelStep(BinDelta2D delta_, Acts::Vector2 intersect_,
                const Acts::Vector2& start)
        : delta(delta_),
          intersect(std::move(intersect_)),
          path((intersect - start).norm()) {}

    /// Smaller operator for sorting the ChannelStep objects.
    ///
    /// @param cstep The other ChannelStep to be compared
    ///
    /// The ChannelStep objects can be compared with its path distance
    /// from the start (surface segment origin)
    bool operator<(const ChannelStep& cstep) const { return path < cstep.path; }
  };

  /// Nested struct for representing channel steps.
  struct ChannelSegment {
    /// The bin of this segment
    Bin2D bin = {0, 0};
    /// The segment start, end points
    Segment2D path2D;
    /// The (clipped) value (uncorrected: path length)
    double activation = 0.;

    /// Constructor with arguments
    ///
    /// @param bin_ The bin corresponding to this step
    /// @param path2D_ The start/end 2D position of the segment
    /// @param activation_ The segment activation (clean: length) for this bin
    ChannelSegment(Bin2D bin_, Segment2D path2D_, double activation_)
        : bin(bin_), path2D(std::move(path2D_)), activation(activation_) {}
  };

  /// Divide the surface segment into channel segments.
  ///
  /// @note Channelizing is done in cartesian coordinates (start/end)
  /// @note The start and end cartesian vector is supposed to be inside
  /// the surface bounds (pre-run through the SurfaceMasker)
  /// @note The segmentation has to be 2-dimensional, even if the
  /// actual readout is 1-dimensional, in latter case one bin in the
  /// second coordinate direction is required.
  ///
  /// @param geoCtx The geometry context for the localToGlobal, etc.
  /// @param surface The surface for the channelizing
  /// @param segmentation The segmentation for the channelizing
  /// @param segment The surface segment (cartesian coordinates)
  ///
  /// @return a vector of ChannelSegment objects
  std::vector<ChannelSegment> segments(const Acts::GeometryContext& geoCtx,
                                       const Acts::Surface& surface,
                                       const Acts::BinUtility& segmentation,
                                       const Segment2D& segment) const;
};

}  // namespace ActsFatras
