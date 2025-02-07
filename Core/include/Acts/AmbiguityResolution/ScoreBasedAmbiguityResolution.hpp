// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/TrackContainer.hpp"
#include "Acts/EventData/TrackContainerFrontendConcept.hpp"
#include "Acts/EventData/TrackProxyConcept.hpp"
#include "Acts/EventData/TrackStateProxy.hpp"
#include "Acts/Utilities/Delegate.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <cstddef>
#include <map>
#include <memory>
#include <numbers>
#include <string>
#include <tuple>
#include <vector>

#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>

namespace Acts {

/// Generic implementation of the score based ambiguity resolution.
/// The alhorithm is based on the following steps:
/// 1) Compute the initial state of the tracks
/// 2) Compute the score of each track
/// 3) Removes hits that are not good enough for each track
/// 4) Remove tracks that have a score below a certain threshold or not have
/// enough hits
/// 5) Remove tracks that are not good enough based on cuts Contains method for
/// data preparations
class ScoreBasedAmbiguityResolution {
 public:
  /// @brief Detector configuration struct : contains the configuration for each detector
  ///
  /// The configuration can be saved in a json file and loaded from there.
  ///
  struct DetectorConfig {
    int hitsScoreWeight = 0;
    int holesScoreWeight = 0;
    int outliersScoreWeight = 0;
    int otherScoreWeight = 0;

    std::size_t minHits = 0;
    std::size_t maxHits = 0;
    std::size_t maxHoles = 0;
    std::size_t maxOutliers = 0;
    std::size_t maxSharedHits = 0;

    /// if true, the shared hits are considered as bad hits for this detector
    bool sharedHitsFlag = false;

    std::size_t detectorId = 0;

    /// a list of values from  0 to 1, the higher number of hits, higher value
    /// in the list is multiplied to ambuiguity score applied only if
    /// useAmbiguityFunction is true
    std::vector<double> factorHits = {1.0};

    /// a list of values from  0 to 1, the higher number of holes, lower value
    /// in the list is multiplied to ambuiguity score applied only if
    /// useAmbiguityFunction is true
    std::vector<double> factorHoles = {1.0};
  };

  /// @brief  TrackFeatures struct : contains the features that are counted for each track.
  ///
  /// The trackFeatures is used to compute the score of each track
  struct TrackFeatures {
    std::size_t nHits = 0;
    std::size_t nHoles = 0;
    std::size_t nOutliers = 0;
    std::size_t nSharedHits = 0;
  };

  enum class TrackStateTypes : std::uint8_t {
    // A measurement not yet used in any other track
    UnsharedHit,
    // A measurement shared with another track
    SharedHit,
    // A hit that needs to be removed from the track
    RejectedHit,
    // An outlier, to be copied in case
    Outlier,
    // Other trackstate types to be copied in case
    OtherTrackStateType
  };

  /// @brief Configuration struct : contains the configuration for the ambiguity resolution.
  struct Config {
    std::map<std::size_t, std::size_t> volumeMap = {{0, 0}};
    std::vector<DetectorConfig> detectorConfigs;
    /// minimum score for any track
    double minScore = 0;
    /// minimum score for shared tracks
    double minScoreSharedTracks = 0;
    /// maximum number of shared tracks per measurement
    std::size_t maxSharedTracksPerMeasurement = 10;
    /// maximum number of shared hit per track
    std::size_t maxShared = 5;

    double pTMin = 0 * UnitConstants::GeV;
    double pTMax = 1e5 * UnitConstants::GeV;

    double phiMin = -std::numbers::pi * UnitConstants::rad;
    double phiMax = std::numbers::pi * UnitConstants::rad;

    double etaMin = -5;
    double etaMax = 5;

    // if true, the ambiguity score is computed based on a different function.
    bool useAmbiguityFunction = false;
  };

  /// @brief OptionalCuts struct : contains the optional cuts to be applied.
  ///
  /// The optional cuts,weights and score are used to remove tracks that are not
  /// good enough, based on some criteria. Users are free to add their own cuts
  /// with the help of this struct.
  template <TrackProxyConcept track_proxy_t>
  struct OptionalCuts {
    using OptionalFilter = std::function<bool(const track_proxy_t&)>;

    using OptionalScoreModifier =
        std::function<void(const track_proxy_t&, double&)>;

    using OptionalHitSelection = std::function<void(
        const track_proxy_t&,
        const typename track_proxy_t::ConstTrackStateProxy&, TrackStateTypes&)>;

    std::vector<OptionalFilter> cuts = {};
    std::vector<OptionalScoreModifier> weights = {};

    /// applied only if useAmbiguityFunction is true
    std::vector<OptionalScoreModifier> scores = {};
    std::vector<OptionalHitSelection> hitSelections = {};
  };

  ScoreBasedAmbiguityResolution(
      const Config& cfg,
      std::unique_ptr<const Logger> logger =
          getDefaultLogger("ScoreBasedAmbiguityResolution", Logging::INFO))
      : m_cfg{cfg}, m_logger{std::move(logger)} {}

  /// Compute the initial state of the tracks.
  ///
  /// @param tracks is the input track container
  /// @return trackFeaturesVectors is the trackFeatures map from detector ID to trackFeatures
  template <TrackContainerFrontend track_container_t>
  std::vector<std::vector<TrackFeatures>> computeInitialState(
      const track_container_t& tracks) const;

  /// Compute the score of each track.
  ///
  /// @param tracks is the input track container
  /// @param trackFeaturesVectors is the trackFeatures map from detector ID to trackFeatures
  /// @param optionalCuts is the user defined optional cuts to be applied.
  /// @return a vector of scores for each track
  template <TrackContainerFrontend track_container_t>
  std::vector<double> simpleScore(
      const track_container_t& tracks,
      const std::vector<std::vector<TrackFeatures>>& trackFeaturesVectors,
      const OptionalCuts<typename track_container_t::ConstTrackProxy>&
          optionalCuts = {}) const;

  /// Compute the score of each track based on the ambiguity function.
  ///
  /// @param tracks is the input track container
  /// @param trackFeaturesVectors is the trackFeatures map from detector ID to trackFeatures
  /// @param optionalCuts is the user defined optional cuts to be applied.
  /// @return a vector of scores for each track
  template <TrackContainerFrontend track_container_t>
  std::vector<double> ambiguityScore(
      const track_container_t& tracks,
      const std::vector<std::vector<TrackFeatures>>& trackFeaturesVectors,
      const OptionalCuts<typename track_container_t::ConstTrackProxy>&
          optionalCuts = {}) const;

  /// Remove hits that are not good enough for each track and removes tracks
  /// that have a score below a certain threshold or not enough hits.
  ///
  /// @brief Remove tracks that are not good enough based on cuts
  /// @param track is the input track
  /// @param trackScore is the score of each track
  /// @param measurementsPerTrack is the list of measurements for each track
  /// @param nTracksPerMeasurement is the number of tracks per measurement
  /// @param optionalHitSelections is the optional hit selections to be applied
  /// @return a vector of IDs of the tracks we want to keep
  template <TrackProxyConcept track_proxy_t>
  bool getCleanedOutTracks(
      const track_proxy_t& track, const double& trackScore,
      const std::vector<std::size_t>& measurementsPerTrack,
      const std::map<std::size_t, std::size_t>& nTracksPerMeasurement,
      const std::vector<std::function<
          void(const track_proxy_t&,
               const typename track_proxy_t::ConstTrackStateProxy&,
               TrackStateTypes&)>>& optionalHitSelections = {}) const;

  /// Remove tracks that are bad based on cuts and weighted scores.
  ///
  /// @brief Remove tracks that are not good enough
  /// @param tracks is the input track container
  /// @param sourceLinkHash is the  source links
  /// @param sourceLinkEquality is the equality function for the source links
  /// @param optionalCuts is the optional cuts to be applied
  /// @return a vector of IDs of the tracks we want to keep
  template <TrackContainerFrontend track_container_t,
            typename source_link_hash_t, typename source_link_equality_t>
  std::vector<int> solveAmbiguity(
      const track_container_t& tracks, source_link_hash_t sourceLinkHash,
      source_link_equality_t sourceLinkEquality,
      const OptionalCuts<typename track_container_t::ConstTrackProxy>&
          optionalCuts = {}) const;

 private:
  Config m_cfg;

  /// Logging instance
  std::unique_ptr<const Logger> m_logger = nullptr;

  /// Private access to logging instance
  const Logger& logger() const;
};

}  // namespace Acts

#include "Acts/AmbiguityResolution/ScoreBasedAmbiguityResolution.ipp"
