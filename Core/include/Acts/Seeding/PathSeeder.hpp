// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/EventData/SourceLink.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Delegate.hpp"

namespace Acts::Experimental {

/// @brief Seeding algorigthm that extracts
/// the IP parameters and sorts the source links
/// into possible track candidates
///
/// The algorithm to convert the given source links
/// into seeds -- pairs of IP parameters and the corresponding
/// source links -- as follows: First the source links
/// are sorted into a user-defined grid. Then, iteration over the source links
/// is performed. If a source link is attached to a surface that is
/// in the first tracking layer, as defined by the user, the IP parameters
/// are estimated and the tracking layers are intersected to construct the
/// core of the "Path". The source links in the subsequent layers are then
/// added to the seed if they lie within the path width of the core.
/// Both the list of source links and the IP parameters are stored in the seed
/// struct.
///
/// @tparam axis_t Type of the axis to bin
/// the source links
///
/// @note The algorithm is designed to be used in the
/// context of a telescope-style geometry. The surfaces
/// are assumed to be planar.
///
/// @note Handling of the rotated surfaces has to happen
/// in the user-defined delegate functions.

template <typename grid_t>
class PathSeeder {
 public:
  using GridType = grid_t;

  /// @brief The seed struct
  ///
  /// The seed struct contains the IP parameters
  /// and the source links that are associated with
  /// the seed.
  struct Seed {
    /// The IP momentum magnitude
    ActsScalar ipP;

    /// The IP momentum direction
    Vector3 ipDir;

    /// The IP vertex position
    Vector3 ipVertex;

    /// The source links associated with the seed
    std::vector<SourceLink> sourceLinks;

    Seed() = delete;
    Seed(ActsScalar ipPmag, Vector3 ipPdir, Vector3 ipPos,
         std::vector<SourceLink> sls)
        : ipP(ipPmag),
          ipDir(std::move(ipPdir)),
          ipVertex(std::move(ipPos)),
          sourceLinks(std::move(sls)) {};
  };

  /// @brief Delegate to provide the relevant grid
  /// filled with source links for the given geometry
  /// member
  ///
  /// @arg The geometry identifier to use
  ///
  /// @return The grid filled with source links
  using SourceLinkGridLookup = Delegate<GridType(const GeometryIdentifier&)>;

  /// @brief Delegate to estimate the IP parameters
  /// and the momentum direction at the first tracking layer
  ///
  /// @arg The geometry context to use
  /// @arg The global position of the pivot source link
  ///
  /// @return Particle charge, the IP momentum magnitude, the IP vertex position,
  /// the IP momentum direction, the momentum direction at the
  /// first tracking layer
  using TrackEstimator =
      Delegate<std::tuple<ActsScalar, ActsScalar, Vector3, Vector3, Vector3>(
          const GeometryContext&, const Vector3&)>;

  /// @brief Delegate to transform the source link to the
  /// appropriate global frame.
  ///
  /// @arg The geometry context to use
  /// @arg The source link to calibrate
  ///
  /// @return The global position of the source link measurement
  using SourceLinkCalibrator =
      Delegate<Vector3(const GeometryContext&, const SourceLink&)>;

  /// @brief Delegate to find the intersections for the given pivot
  /// source link
  ///
  /// @arg The geometry context to use
  /// @arg The global position of the pivot source link
  /// @arg The momentum direction of the pivot source link
  /// at the first tracking layer
  /// @arg The IP momentum magnitude
  /// @arg The particle charge
  using IntersectionLookup =
      Delegate<std::vector<std::pair<GeometryIdentifier, Vector3>>(
          const GeometryContext&, const Vector3&, const Vector3&,
          const ActsScalar&, const ActsScalar&)>;

  /// @brief Delegate to provide the path width around
  /// the intersection point to pull the source links
  /// from the grid
  ///
  /// @arg The geometry context to use
  /// @arg The geometry identifier to use if the
  /// path width is varied across different tracking layers
  ///
  /// @return The path width in the bin0 and bin1 direction
  /// defined with respect to the surface normal
  using PathWidthLookup = Delegate<std::pair<ActsScalar, ActsScalar>(
      const GeometryContext&, const GeometryIdentifier&)>;

  /// @brief The nested configuration struct
  struct Config {
    /// Binned SourceLink provider
    SourceLinkGridLookup sourceLinkGridLookup;
    /// Parameters estimator
    TrackEstimator trackEstimator;
    /// SourceLink calibrator
    SourceLinkCalibrator sourceLinkCalibrator;
    /// Intersection finder
    IntersectionLookup intersectionFinder;
    /// Path width provider
    PathWidthLookup pathWidthProvider;
    /// First layer extent
    Extent firstLayerExtent;
    /// Direction of the telescope extent
    BinningValue orientation = BinningValue::binX;
  };

  /// @brief Constructor
  PathSeeder(const Config& config) : m_cfg(std::move(config)) {};

  /// @brief Destructor
  ~PathSeeder() = default;

  /// @brief Extract the IP parameters and
  /// sort the source links into the seeds
  ///
  /// @param gctx The geometry context
  /// @param sourceLinks The source links to seed
  ///
  /// @return The vector of seeds
  std::vector<Seed> getSeeds(const GeometryContext& gctx,
                             const std::vector<SourceLink>& sourceLinks) const {
    // Get plane of the telescope
    // sensitive surfaces
    int bin0 = static_cast<int>(BinningValue::binX);
    int bin1 = static_cast<int>(BinningValue::binY);
    if (m_cfg.orientation == BinningValue::binX) {
      bin0 = static_cast<int>(BinningValue::binY);
      bin1 = static_cast<int>(BinningValue::binZ);
    } else if (m_cfg.orientation == BinningValue::binY) {
      bin0 = static_cast<int>(BinningValue::binX);
      bin1 = static_cast<int>(BinningValue::binZ);
    }

    // Create the seeds
    std::vector<Seed> seeds;
    for (const auto& sl : sourceLinks) {
      Vector3 globalPos = m_cfg.sourceLinkCalibrator(gctx, sl);

      // Check if the hit is in the
      // first tracking layer
      if (!m_cfg.firstLayerExtent.contains(globalPos)) {
        continue;
      }

      // Get the IP parameters
      auto [q, ipP, ipVertex, ipDir, flDir] =
          m_cfg.trackEstimator(gctx, globalPos);

      // Intersect with the surfaces
      std::vector<std::pair<GeometryIdentifier, Vector3>> intersections =
          m_cfg.intersectionFinder(gctx, globalPos, flDir, ipP, q);

      // Continue if no intersections
      if (intersections.empty()) {
        continue;
      }
      // Vector to store the source links
      std::vector<SourceLink> seedSourceLinks;

      // Store the pivot source link
      seedSourceLinks.push_back(sl);

      // Iterate over the intersections
      // and get the source links
      // in the subsequent layers
      for (auto& [geoId, refPoint] : intersections) {
        // Get the path width
        auto [pathWidth0, pathWidth1] = m_cfg.pathWidthProvider(gctx, geoId);

        // Get the bounds of the path
        ActsScalar top0 = refPoint[bin0] + pathWidth0;
        ActsScalar bot0 = refPoint[bin0] - pathWidth0;
        ActsScalar top1 = refPoint[bin1] + pathWidth1;
        ActsScalar bot1 = refPoint[bin1] - pathWidth1;

        // Get the lookup table for the source links
        auto grid = m_cfg.sourceLinkGridLookup(geoId);

        // Get the range of bins to search for source links
        auto botLeftBin = grid.localBinsFromPosition(Vector2(bot0, bot1));
        auto topRightBin = grid.localBinsFromPosition(Vector2(top0, top1));

        // Get the source links from the lookup table
        // by iterating over the bin ranges
        auto currentBin = botLeftBin;
        while (currentBin.at(1) <= topRightBin.at(1)) {
          while (currentBin.at(0) <= topRightBin.at(0)) {
            auto sourceLinksToAdd = grid.atLocalBins(currentBin);

            seedSourceLinks.insert(seedSourceLinks.end(),
                                   sourceLinksToAdd.begin(),
                                   sourceLinksToAdd.end());
            currentBin.at(0)++;
          }
          currentBin.at(1)++;
          currentBin.at(0) = botLeftBin.at(0);
        }
      }

      // Store the IP parameters and
      // add the source links to the seed
      Seed seed{ipP, ipDir, ipVertex, seedSourceLinks};

      // Add the seed to the list
      seeds.push_back(seed);
    }
    return seeds;
  };

 private:
  Config m_cfg;
};

}  // namespace Acts::Experimental
