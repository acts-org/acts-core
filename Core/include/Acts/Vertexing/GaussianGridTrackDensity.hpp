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

namespace Acts {
/// @class GaussianGridTrackDensity
/// @brief Implements a 1-dim density grid to be filled with
/// track Gaussian distributions. Each single track is modelled
/// as a 2(!)-dim Gaussian distribution grid in the d0-z0 plane,
/// but only the overlap with the z-axis (i.e. a 1-dim density
/// vector) needs to be calculated.
/// The position of the highest track density (of either a single
/// bin or the sum of a certain region) can be determined.
/// Single tracks can be cached and removed from the overall density.
class GaussianGridTrackDensity {
 public:
  using MainGridVector = Eigen::Matrix<float, Eigen::Dynamic, 1>;
  using TrackGridVector = Eigen::Matrix<float, Eigen::Dynamic, 1>;

  /// The configuration struct
  struct Config {
    /// @param zMinMax_ The minimum and maximum z-values (in mm) that
    ///                 should be covered by the main 1-dim density grid along
    ///                 the z-axis
    /// @param mainGridSize_ The size of the z-axis 1-dim main density grid
    /// @param trkGridSize_ The 2(!)-dim grid size of a single track, i.e.
    /// a single track is modelled as a (trkGridSize x trkGridSize) grid
    /// in the d0-z0 plane. Note: trkGridSize has to be an odd value.
    /// @note The value of @p zMinMax_ together with @p mainGridSize_ determines the
    /// overall bin size to be used as seen below
    Config(float zMinMax_ = 100, int mainGridSize_ = 2000,
           int trkGridSize_ = 15)
        : mainGridSize(mainGridSize_),
          trkGridSize(trkGridSize_),
          zMinMax(zMinMax_) {
      binSize = 2. * zMinMax / mainGridSize;

      if (trkGridSize % 2 == 0) {
        throw std::runtime_error(
            "GaussianGridTrackDensity: trkGridSize has to be an odd value!");
      }
      if (mainGridSize < trkGridSize) {
        throw std::runtime_error(
            "GaussianGridTrackDensity: mainGridSize has to be bigger than "
            "trkGridSize!");
      }
    }

    int mainGridSize;
    int trkGridSize;

    // Min and max z value of big grid
    float zMinMax;  // mm

    // Z size of one single bin in grid
    float binSize;  // mm

    // Do NOT use just the z-bin with the highest
    // track density, but instead check the (up to)
    // first three density maxima (only those that have
    // a maximum relative deviation of 'relativeDensityDev'
    // from the main maximum) and take the z-bin of the
    // maximum with the highest surrounding density sum
    bool useHighestSumZPosition = false;

    // The maximum relative density deviation from the main
    // maximum to consider the second and third maximum for
    // the highest-sum approach from above
    float maxRelativeDensityDev = 0.01;
  };

  GaussianGridTrackDensity(const Config& cfg) : m_cfg(cfg) {}

  /// @brief Returns the z position of maximum track density
  ///
  /// @param mainGrid The main 1-dim density grid along the z-axis
  ///
  /// @return The z position of maximum track density
  Result<float> getMaxZPosition(MainGridVector& mainGrid) const;

  /// @brief Returns the z position of maximum track density and
  /// the estimated width
  ///
  /// @param mainGrid The main 1-dim density grid along the z-axis
  ///
  /// @return The z position of maximum track density and width
  Result<std::pair<float, float>> getMaxZPositionAndWidth(
      MainGridVector& mainGrid) const;

  /// @brief Adds a single track to the overall grid density
  ///
  /// @param trk The track to be added
  /// @param mainGrid The main 1-dim density grid along the z-axis
  ///
  /// @return A pair storing information about the z-bin position
  /// the track was added (int) and the 1-dim density contribution
  /// of the track itself
  std::pair<int, TrackGridVector> addTrack(const BoundTrackParameters& trk,
                                           MainGridVector& mainGrid) const;

  /// @brief Removes a track from the overall grid density
  ///
  /// @param zBin The center z-bin position the track needs to be
  /// removed from
  /// @param trkGrid The 1-dim density contribution of the track
  /// @param mainGrid The main 1-dim density grid along the z-axis
  void removeTrackGridFromMainGrid(int zBin, const TrackGridVector& trkGrid,
                                   MainGridVector& mainGrid) const;

  const Config& config() const { return m_cfg; }

 private:
  /// @brief Helper function that actually adds the track to the
  /// main density grid
  ///
  /// @param zBin The center z-bin position the track
  /// @param trkGrid The 1-dim density contribution of the track
  /// @param mainGrid The main 1-dim density grid along the z-axis
  void addTrackGridToMainGrid(int zBin, const TrackGridVector& trkGrid,
                              MainGridVector& mainGrid) const;

  /// @brief Helper function that modifies the main density grid
  /// (either adds or removes a track)
  ///
  /// @param zBin The center z-bin position the track
  /// @param trkGrid The 1-dim density contribution of the track
  /// @param mainGrid The main 1-dim density grid along the z-axis
  /// @param modifyModeSign Sign that determines the mode of modification,
  /// +1 for adding a track, -1 for removing a track
  void modifyMainGridWithTrackGrid(int zBin, const TrackGridVector& trkGrid,
                                   MainGridVector& mainGrid,
                                   int modifyModeSign) const;

  /// @brief Function that creates a 1-dim track grid (i.e. a vector)
  /// with the correct density contribution of a track along the z-axis
  ///
  /// @param d0 Transverse impact parameter
  /// @param distCtrZ The distance in z0 from the track position to its
  /// bin center in the 2-dim grid
  /// @param cov The track covariance matrix
  TrackGridVector createTrackGrid(float d0, float distCtrZ,
                                  const Acts::SquareMatrix2& cov) const;

  /// @brief Function that estimates the seed width based on the FWHM of
  /// the maximum density peak
  /// @note This only works if the maximum is sufficiently isolated since
  /// overlapping neighboring peaks might lead to an overestimation of the
  /// seed width.
  ///
  /// @param mainGrid The main 1-dim density grid along the z-axis
  /// @param maxZ z-position of the maximum density value
  ///
  /// @return The width
  Result<float> estimateSeedWidth(MainGridVector& mainGrid, float maxZ) const;

  /// @brief Helper to retrieve values according to a 2-dim normal distribution
  /// @note This function is defined in coordinate system centered around d0 and z0
  float normal2D(float d, float z, const SquareMatrix2& cov) const;

  /// @brief Checks the (up to) first three density maxima (only those that have
  /// a maximum relative deviation of 'relativeDensityDev' from the main
  /// maximum) and take the z-bin of the maximum with the highest surrounding
  /// density
  ///
  /// @param mainGrid The main 1-dim density grid along the z-axis
  ///
  /// @return The z-bin position
  int getHighestSumZPosition(MainGridVector& mainGrid) const;

  /// @brief Calculates the density sum of a z-bin and its two neighboring bins
  /// as needed for 'getHighestSumZPosition'
  ///
  /// @param mainGrid The main 1-dim density grid along the z-axis
  /// @param pos The center z-bin position
  ///
  /// @return The sum
  double getDensitySum(const MainGridVector& mainGrid, int pos) const;

  Config m_cfg;
};

}  // namespace Acts
