// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Vertexing/TrackAtVertex.hpp"

#include <map>
#include <set>

namespace Acts {

/// @class GaussianTrackDensity
///
/// @brief Class to model tracks as 2D density functions based on
/// their d0 and z0 perigee parameters (mean value) and covariance
/// matrices (determining the width of the function)
class GaussianTrackDensity {
 public:
  /// @brief Struct to store information for a single track
  struct TrackEntry {
    /// @brief Default constructor
    TrackEntry() = default;
    /// @brief Constructor initializing all members
    /// @param z_ Trial z position
    /// @param c0_ z-independent term in exponent
    /// @param c1_ Linear coefficient in exponent
    /// @param c2_ Quadratic coefficient in exponent
    /// @param lowerBound_ The lower bound
    /// @param upperBound_ The upper bound
    TrackEntry(double z_, double c0_, double c1_, double c2_,
               double lowerBound_, double upperBound_)
        : z(z_),
          c0(c0_),
          c1(c1_),
          c2(c2_),
          lowerBound(lowerBound_),
          upperBound(upperBound_) {}

    double z = 0;
    // Cached information for a single track
    // z-independent term in exponent
    double c0 = 0;
    // linear coefficient in exponent
    double c1 = 0;
    // quadratic coefficient in exponent
    double c2 = 0;
    // The lower bound
    double lowerBound = 0;
    // The upper bound
    double upperBound = 0;
  };

  /// @brief The Config struct
  struct Config {
    Config(double d0Sig = 3.5, double z0Sig = 12.)
        : d0MaxSignificance(d0Sig),
          z0MaxSignificance(z0Sig),
          d0SignificanceCut(d0Sig * d0Sig),
          z0SignificanceCut(z0Sig * z0Sig) {}

    // Assumed shape of density function:
    // Gaussian (true) or parabolic (false)
    bool isGaussianShaped = true;

    // Maximum d0 impact parameter significance to use a track
    double d0MaxSignificance;
    // Maximum z0 impact parameter significance to use a track
    double z0MaxSignificance;
    // Corresponding cut values
    double d0SignificanceCut;
    double z0SignificanceCut;

    // Function to extract parameters from InputTrack
    InputTrack::Extractor extractParameters;
  };

  /// @brief The State struct
  struct State {
    // Constructor with size track map
    State(unsigned int nTracks) { trackEntries.reserve(nTracks); }
    // Vector to cache track information
    std::vector<TrackEntry> trackEntries;
  };

  /// Constructor with config
  GaussianTrackDensity(const Config& cfg) : m_cfg(cfg) {
    if (!m_cfg.extractParameters.connected()) {
      throw std::invalid_argument(
          "GaussianTrackDensity: "
          "No parameter extractor provided.");
    }
  }

  /// @brief Calculates z position of global maximum with Gaussian width
  /// for density function.
  /// Strategy:
  /// The global maximum must be somewhere near a track.
  /// Since we can calculate the first and second derivatives, at each point we
  /// can determine a) whether the function is curved up (minimum) or down
  /// (maximum) b) the distance to nearest maximum, assuming either Newton
  /// (parabolic) or Gaussian local behavior.
  /// For each track where the second derivative is negative, find step to
  /// nearest maximum, take that step and then do one final refinement. The
  /// largest density encountered in this procedure (after checking all tracks)
  /// is considered the maximum.
  ///
  /// @param state The track density state
  /// @param trackList All input tracks
  ///
  /// @return Pair of position of global maximum and Gaussian width
  Result<std::optional<std::pair<double, double>>> globalMaximumWithWidth(
      State& state, const std::vector<InputTrack>& trackList) const;

  /// @brief Calculates the z position of the global maximum
  ///
  /// @param state The track density state
  /// @param trackList All input tracks
  ///
  /// @return z position of the global maximum
  Result<std::optional<double>> globalMaximum(
      State& state, const std::vector<InputTrack>& trackList) const;

 private:
  /// The configuration
  Config m_cfg;

  /// @brief Add a track to the set being considered
  ///
  /// @param state The track density state
  /// @param trackList All input tracks
  Result<void> addTracks(State& state,
                         const std::vector<InputTrack>& trackList) const;

  /// @brief Evaluate the density function and its two first
  /// derivatives at the specified coordinate along the beamline
  ///
  /// @param state The track density state
  /// @param z z-position along the beamline
  ///
  /// @return Track density, first and second derivatives
  std::tuple<double, double, double> trackDensityAndDerivatives(State& state,
                                                                double z) const;

  /// @brief Update the current maximum values
  ///
  /// @param newZ The new z value
  /// @param newValue The new value at z position
  /// @param newSecondDerivative The new second derivative
  /// @param maxZ Maximum z value, will be compared against @p newZ
  /// @param maxValue Maximum value
  /// @param maxSecondDerivative Maximum of the second derivative
  /// @return The max z position, the max value at z position, the max second
  /// derivative
  std::tuple<double, double, double> updateMaximum(
      double newZ, double newValue, double newSecondDerivative, double maxZ,
      double maxValue, double maxSecondDerivative) const;

  /// @brief Calculates the step size
  ///
  /// @param y Position value
  /// @param dy First derivative
  /// @param ddy Second derivative
  ///
  /// @return The step size
  double stepSize(double y, double dy, double ddy) const;

  // Helper class to evaluate and store track density at specific position
  class GaussianTrackDensityStore {
   public:
    // Initialise at the z coordinate at which the density is to be evaluated
    GaussianTrackDensityStore(double z_coordinate) : m_z(z_coordinate) {}

    // Add the contribution of a single track to the density
    void addTrackToDensity(const TrackEntry& entry);

    // Return density, first and second derivatives
    inline std::tuple<double, double, double> densityAndDerivatives() const {
      return {m_density, m_firstDerivative, m_secondDerivative};
    }

   private:
    // Store density and derivatives for z position m_z
    double m_z;
    double m_density{0};
    double m_firstDerivative{0};
    double m_secondDerivative{0};
  };
};

}  // namespace Acts
