// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/EventData/SourceLink.hpp"
#include "Acts/EventData/TrackContainer.hpp"
#include "Acts/EventData/TrackProxy.hpp"
#include "Acts/EventData/VectorMultiTrajectory.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/TrackFinding/CombinatorialKalmanFilter.hpp"
#include "Acts/TrackFinding/MeasurementSelector.hpp"
#include "Acts/TrackFinding/TrackSelector.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/Result.hpp"
#include "Acts/Utilities/TrackHelpers.hpp"
#include "ActsExamples/EventData/IndexSourceLink.hpp"
#include "ActsExamples/EventData/Measurement.hpp"
#include "ActsExamples/EventData/SimSeed.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"

#include <atomic>
#include <cstddef>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <variant>
#include <vector>

#include <tbb/combinable.h>

namespace Acts {
class MagneticFieldProvider;
class TrackingGeometry;
}  // namespace Acts

namespace ActsExamples {
struct AlgorithmContext;

class TrackFindingAlgorithm final : public IAlgorithm {
 public:
  /// Track finder function that takes input measurements, initial trackstate
  /// and track finder options and returns some track-finder-specific result.
  using TrackFinderOptions =
      Acts::CombinatorialKalmanFilterOptions<IndexSourceLinkAccessor::Iterator,
                                             TrackContainer>;
  using TrackFinderResult =
      Acts::Result<std::vector<TrackContainer::TrackProxy>>;

  /// Find function that takes the above parameters
  /// @note This is separated into a virtual interface to keep compilation units
  /// small
  class TrackFinderFunction {
   public:
    virtual ~TrackFinderFunction() = default;
    virtual TrackFinderResult operator()(const TrackParameters&,
                                         const TrackFinderOptions&,
                                         TrackContainer&, TrackProxy) const = 0;
  };

  /// Create the track finder function implementation.
  ///
  /// The magnetic field is intentionally given by-value since the variant
  /// contains shared_ptr anyway.
  static std::shared_ptr<TrackFinderFunction> makeTrackFinderFunction(
      std::shared_ptr<const Acts::TrackingGeometry> trackingGeometry,
      std::shared_ptr<const Acts::MagneticFieldProvider> magneticField,
      const Acts::Logger& logger);

  struct Config {
    /// Input measurements collection.
    std::string inputMeasurements;
    /// Input source links collection.
    std::string inputSourceLinks;
    /// Input initial track parameter estimates for for each proto track.
    std::string inputInitialTrackParameters;
    /// Input seeds. These are optional and allow for seed deduplication.
    /// The seeds must match the initial track parameters.
    std::string inputSeeds;
    /// Output find trajectories collection.
    std::string outputTracks;

    /// The tracking geometry that should be used.
    std::shared_ptr<const Acts::TrackingGeometry> trackingGeometry;
    /// The magnetic field that should be used.
    std::shared_ptr<const Acts::MagneticFieldProvider> magneticField;

    /// Type erased track finder function.
    std::shared_ptr<TrackFinderFunction> findTracks;
    /// CKF measurement selector config
    Acts::MeasurementSelector::Config measurementSelectorCfg;
    /// Track selector config
    std::optional<std::variant<Acts::TrackSelector::Config,
                               Acts::TrackSelector::EtaBinnedConfig>>
        trackSelectorCfg = std::nullopt;

    /// Maximum number of propagation steps
    unsigned int maxSteps = 100000;
    /// Extrapolation strategy
    Acts::TrackExtrapolationStrategy extrapolationStrategy =
        Acts::TrackExtrapolationStrategy::firstOrLast;
    /// Run finding in two directions
    bool twoWay = true;
    /// Whether to run the finding in seed parameter direction or reverse
    /// direction
    bool reverseSearch = false;
    /// Whether to use seed deduplication
    /// This is only available if `inputSeeds` is set.
    bool seedDeduplication = false;
    /// Whether to stick on the seed measurements during track finding.
    /// This is only available if `inputSeeds` is set.
    bool stayOnSeed = false;
    /// Compute shared hit information
    bool computeSharedHits = false;
    /// Whether to trim the tracks
    bool trimTracks = true;

    // Pixel and strip volume ids to be used for maxPixel/StripHoles cuts
    std::vector<std::uint32_t> pixelVolumeIds;
    std::vector<std::uint32_t> stripVolumeIds;

    // additional track selector settings
    std::size_t maxPixelHoles = std::numeric_limits<std::size_t>::max();
    std::size_t maxStripHoles = std::numeric_limits<std::size_t>::max();

    /// The volume ids to constrain the track finding to
    std::vector<std::uint32_t> constrainToVolumeIds;
    /// The volume ids to stop the track finding at
    std::vector<std::uint32_t> endOfWorldVolumeIds;
  };

  /// Constructor of the track finding algorithm
  ///
  /// @param config is the config struct to configure the algorithm
  /// @param level is the logging level
  TrackFindingAlgorithm(Config config, Acts::Logging::Level level);

  /// Framework execute method of the track finding algorithm
  ///
  /// @param ctx is the algorithm context that holds event-wise information
  /// @return a process code to steer the algorithm flow
  ActsExamples::ProcessCode execute(
      const ActsExamples::AlgorithmContext& ctx) const final;

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 private:
  template <typename source_link_accessor_container_t>
  void computeSharedHits(const source_link_accessor_container_t& sourceLinks,
                         TrackContainer& tracks) const;

  ActsExamples::ProcessCode finalize() override;

 private:
  Config m_cfg;
  std::optional<Acts::TrackSelector> m_trackSelector;

  ReadDataHandle<MeasurementContainer> m_inputMeasurements{this,
                                                           "InputMeasurements"};
  ReadDataHandle<IndexSourceLinkContainer> m_inputSourceLinks{
      this, "InputSourceLinks"};
  ReadDataHandle<TrackParametersContainer> m_inputInitialTrackParameters{
      this, "InputInitialTrackParameters"};
  ReadDataHandle<SimSeedContainer> m_inputSeeds{this, "InputSeeds"};

  WriteDataHandle<ConstTrackContainer> m_outputTracks{this, "OutputTracks"};

  mutable std::atomic<std::size_t> m_nTotalSeeds{0};
  mutable std::atomic<std::size_t> m_nDeduplicatedSeeds{0};
  mutable std::atomic<std::size_t> m_nFailedSeeds{0};
  mutable std::atomic<std::size_t> m_nFailedSmoothing{0};
  mutable std::atomic<std::size_t> m_nFailedExtrapolation{0};
  mutable std::atomic<std::size_t> m_nFoundTracks{0};
  mutable std::atomic<std::size_t> m_nSelectedTracks{0};
  mutable std::atomic<std::size_t> m_nStoppedBranches{0};

  mutable tbb::combinable<Acts::VectorMultiTrajectory::Statistics>
      m_memoryStatistics{[]() {
        auto mtj = std::make_shared<Acts::VectorMultiTrajectory>();
        return mtj->statistics();
      }};
};

// TODO this is somewhat duplicated in AmbiguityResolutionAlgorithm.cpp
// TODO we should make a common implementation in the core at some point
template <typename source_link_accessor_container_t>
void TrackFindingAlgorithm::computeSharedHits(
    const source_link_accessor_container_t& sourceLinks,
    TrackContainer& tracks) const {
  // Compute shared hits from all the reconstructed tracks
  // Compute nSharedhits and Update ckf results
  // hit index -> list of multi traj indexes [traj, meas]

  std::vector<std::size_t> firstTrackOnTheHit(
      sourceLinks.size(), std::numeric_limits<std::size_t>::max());
  std::vector<std::size_t> firstStateOnTheHit(
      sourceLinks.size(), std::numeric_limits<std::size_t>::max());

  for (auto track : tracks) {
    for (auto state : track.trackStatesReversed()) {
      if (!state.typeFlags().test(Acts::TrackStateFlag::MeasurementFlag)) {
        continue;
      }

      std::size_t hitIndex = state.getUncalibratedSourceLink()
                                 .template get<IndexSourceLink>()
                                 .index();

      // Check if hit not already used
      if (firstTrackOnTheHit.at(hitIndex) ==
          std::numeric_limits<std::size_t>::max()) {
        firstTrackOnTheHit.at(hitIndex) = track.index();
        firstStateOnTheHit.at(hitIndex) = state.index();
        continue;
      }

      // if already used, control if first track state has been marked
      // as shared
      int indexFirstTrack = firstTrackOnTheHit.at(hitIndex);
      int indexFirstState = firstStateOnTheHit.at(hitIndex);

      auto firstState = tracks.getTrack(indexFirstTrack)
                            .container()
                            .trackStateContainer()
                            .getTrackState(indexFirstState);
      if (!firstState.typeFlags().test(Acts::TrackStateFlag::SharedHitFlag)) {
        firstState.typeFlags().set(Acts::TrackStateFlag::SharedHitFlag);
      }

      // Decorate this track state
      state.typeFlags().set(Acts::TrackStateFlag::SharedHitFlag);
    }
  }
}

}  // namespace ActsExamples
