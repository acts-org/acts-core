// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once
#include "Acts/Definitions/Algebra.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Utilities/Result.hpp"

#include <boost/container/flat_map.hpp>  // TODO use flat unordered map
#include <boost/functional/hash.hpp>

namespace Acts {

/// @class AdaptiveGridTrackDensity
/// @brief Implements a 1D (no time seeding) / 2D (time seeding)
/// grid that is filled with track densities.
/// Each track is modelled by a 2D / 3D Gaussian distribution in the
/// d0-z0 / d0-z0-t0 plane, which is evaluated at d0=0. Therefore,
/// each track effectively lives in 1D / 2D.
/// The position of the highest track density (of either a single
/// bin or the sum of a certain region) can be determined.
/// Single tracks can be cached and removed from the overall density.
/// Unlike in the GaussianGridTrackDensity, the overall density map
/// grows adaptively when tracks densities are added to the grid.
class AdaptiveGridTrackDensity {
 public:
  /// The first (second) integer indicates the bin's z (t) position
  using Bin = std::pair<std::int32_t, std::int32_t>;
  /// Mapping between bins and track densities
  using DensityMap = boost::container::flat_map<Bin, float>;
  /// Coordinates in the z-t plane; the t value will be set to 0 if time
  /// vertex seeding is disabled
  using ZTPosition = std::pair<double, double>;
  /// z-t position of a maximum and its width
  using ZTPositionAndWidth = std::pair<ZTPosition, double>;
  /// Optional grid size range
  using GridSizeRange =
      std::pair<std::optional<std::uint32_t>, std::optional<std::uint32_t>>;

  /// The configuration struct
  struct Config {
    /// Spatial extent of a bin in d0 and z0 direction, should always be set to
    /// a positive value
    double spatialBinExtent = 15 * UnitConstants::um;

    /// Number of standard deviations that the grid covers in z direction
    double nSpatialTrkSigmas = 3.0;

    /// Temporal extent of a bin, not used if useTime == true
    double temporalBinExtent = 19 * UnitConstants::mm;

    /// Number of standard deviations that the grid covers in t direction, not
    /// used if useTime == true
    double nTemporalTrkSigmas = 3.0;

    /// Spatial window for filling the density map
    std::pair<double, double> spatialWindow = {-250 * UnitConstants::mm,
                                               250 * UnitConstants::mm};
    /// Temporal window for filling the density map
    std::pair<double, double> temporalWindow = {-10 * UnitConstants::ns,
                                                10 * UnitConstants::ns};

    /// Optional minimal and maximal number of bins in z direction
    GridSizeRange spatialTrkGridSizeRange = {std::nullopt, std::nullopt};
    /// Optional minimal and maximal number of bins in time direction
    GridSizeRange temporalTrkGridSizeRange = {std::nullopt, std::nullopt};

    bool useTime = false;

    /// Do NOT use just the z-bin with the highest
    /// track density, but instead check (up to)
    /// first three density maxima (only those that have
    /// a maximum relative deviation of 'relativeDensityDev'
    /// from the main maximum) and take the z-bin of the
    /// maximum with the highest surrounding density sum
    bool useHighestSumZPosition = false;

    /// The maximum relative density deviation from the main
    /// maximum to consider the second and third maximum for
    /// the highest-sum approach from above
    double maxRelativeDensityDev = 0.01;
  };

  AdaptiveGridTrackDensity(const Config& cfg);

  /// @brief Returns the z and t coordinate of maximum (surrounding)
  /// track density
  /// @note if time vertex seeding is not enabled, the t coordinate
  /// will be set to 0.
  ///
  /// @param densityMap Map between bins and corresponding density
  /// values
  ///
  /// @return The z and t coordinates of maximum track density
  Result<ZTPosition> getMaxZTPosition(DensityMap& densityMap) const;

  /// @brief Returns the z-t position of maximum track density
  /// and the estimated z-width of the maximum
  ///
  /// @param densityMap Map between bins and corresponding density
  /// values
  ///
  /// @return The z-t position of the maximum track density and
  /// its width
  Result<ZTPositionAndWidth> getMaxZTPositionAndWidth(
      DensityMap& densityMap) const;

  /// @brief Adds a single track to the overall grid density
  ///
  /// @param trk The track to be added
  /// @param mainDensityMap Map between bins and corresponding density
  ///
  /// @return The density map of the track that was added
  DensityMap addTrack(const BoundTrackParameters& trk,
                      DensityMap& mainDensityMap) const;

  /// @brief Removes a track from the overall grid density.
  ///
  /// @param trackDensityMap Map between bins and corresponding density
  /// @note The track density comes from a single track
  /// @param mainDensityMap Map between bins and corresponding density
  /// @note The track density comes from an arbitrary number of tracks
  void subtractTrack(const DensityMap& trackDensityMap,
                     DensityMap& mainDensityMap) const;

  // TODO this should not be public
  /// @brief Calculates the bin center from the bin number
  /// @param bin Bin number
  /// @param binExtent Bin extent
  /// @return Bin center
  static double getBinCenter(std::int32_t bin, double binExtent);

 private:
  Config m_cfg;

  /// @brief Calculates the bin number corresponding to a d, z, or time value
  /// @param value d, z, or time value
  /// @param binExtent Bin extent
  /// @return Bin number
  static std::int32_t getBin(double value, double binExtent);

  /// @brief Calculates the grid size in z or time direction
  /// @param sigma Standard deviation of the track density
  /// @param trkSigmas Number of standard deviations that the grid
  ///        covers in z or time direction
  /// @param binExtent Bin extent
  /// @param trkGridSizeRange Optional minimal and maximal number of bins
  ///        in z or time direction
  /// @return Grid size
  static std::uint32_t getTrkGridSize(double sigma, double trkSigmas,
                                      double binExtent,
                                      const GridSizeRange& trkGridSizeRange);

  /// @brief Calculates the bin number corresponding to a z value
  /// @param value z value
  /// @return Bin number
  std::int32_t getSpatialBin(double value) const;
  /// @brief Calculates the bin number corresponding to a time value
  /// @param value Time value
  /// @return Bin number
  std::int32_t getTemporalBin(double value) const;

  /// @brief Calculates the spatial bin center corresponding to a bin number
  /// @param bin Bin number
  /// @return Bin center
  double getSpatialBinCenter(std::int32_t bin) const;
  /// @brief Calculates the temporal bin center corresponding to a bin number
  /// @param bin Bin number
  /// @return Bin center
  double getTemporalBinCenter(std::int32_t bin) const;

  /// @brief Calculates the grid size in z direction
  /// @param sigma Standard deviation of the track density
  /// @return Grid size
  std::uint32_t getSpatialTrkGridSize(double sigma) const;
  /// @brief Calculates the grid size in time direction
  /// @param sigma Standard deviation of the track density
  /// @return Grid size
  std::uint32_t getTemporalTrkGridSize(double sigma) const;

  /// @brief Finds the maximum density of a DensityMap
  /// @param densityMap Map between bins and corresponding density
  /// values
  /// @return Iterator of the map entry with the highest density
  DensityMap::const_iterator highestDensityEntry(
      const DensityMap& densityMap) const;

  /// @brief Function that creates a track density map, i.e., a map from bins
  /// to the corresponding density values for a single track.
  ///
  /// @param impactParams vector containing d0, z0, and t0 of the track
  /// @param centralBin Central z and t bin of the track (where its
  /// density is the highest)
  /// @param cov 3x3 impact parameter covariance matrix
  /// @param spatialTrkGridSize Number of bins in z direction
  /// @param temporalTrkGridSize Number of bins in time direction
  ///
  /// @return The track density map
  DensityMap createTrackGrid(const Acts::Vector3& impactParams,
                             const Bin& centralBin,
                             const Acts::SquareMatrix3& cov,
                             std::uint32_t spatialTrkGridSize,
                             std::uint32_t temporalTrkGridSize) const;

  /// @brief Function that estimates the seed width in z direction based
  /// on the full width at half maximum (FWHM) of the maximum density peak
  /// @note This only works if the maximum is sufficiently isolated since
  /// overlapping neighboring peaks might lead to an overestimation of the
  /// seed width.
  ///
  /// @param densityMap Map from bins to corresponding track density
  /// @param maxZT z-t position of the maximum density value
  ///
  /// @return The width
  Result<double> estimateSeedWidth(const DensityMap& densityMap,
                                   const ZTPosition& maxZT) const;

  /// @brief Checks (up to) first three density maxima that have a
  /// maximum relative deviation of 'relativeDensityDev' from the
  /// global maximum. Returns the bin of the maximum that has the
  /// highest surrounding density in z direction.
  ///
  /// @param densityMap Map between bins and corresponding density values
  ///
  /// @return The bin corresponding to the highest surrounding density
  Bin highestDensitySumBin(DensityMap& densityMap) const;

  /// @brief Calculates the density sum of a bin and its two neighboring bins
  /// in z direction
  ///
  /// @param densityMap Map between bins and corresponding density values
  /// @param bin Bin whose neighbors in z we want to sum up
  ///
  /// @return The density sum
  double getDensitySum(const DensityMap& densityMap, const Bin& bin) const;
};

}  // namespace Acts
