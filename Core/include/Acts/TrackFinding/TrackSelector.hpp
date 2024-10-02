// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/EventData/TrackProxyConcept.hpp"
#include "Acts/EventData/TrackStateType.hpp"
#include "Acts/Geometry/GeometryHierarchyMap.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"

#include <cmath>
#include <functional>
#include <limits>
#include <numeric>
#include <ostream>
#include <vector>

#include <boost/container/small_vector.hpp>

namespace Acts {

/// Class which performs filtering of tracks. It accepts an input and an output
/// track container and uses the built-in copy facility to copy tracks into the
/// output container.
class TrackSelector {
  static constexpr double inf = std::numeric_limits<double>::infinity();

 public:
  struct MeasurementCounter {
    // Combination of a geometry hierarchy map and a minimum hit count
    using CounterElement =
        std::pair<GeometryHierarchyMap<unsigned int>, unsigned int>;

    boost::container::small_vector<CounterElement, 4> counters;

    template <TrackProxyConcept track_proxy_t>
    bool isValidTrack(const track_proxy_t& track) const;

    void addCounter(const std::vector<GeometryIdentifier>& identifiers,
                    unsigned int threshold) {
      std::vector<GeometryHierarchyMap<unsigned int>::InputElement> elements;
      for (const auto& id : identifiers) {
        elements.emplace_back(id, 0);
      }
      counters.emplace_back(std::move(elements), threshold);
    }
  };

  /// Configuration of a set of cuts for a single eta bin
  /// Default construction yields a set of cuts that accepts everything.
  struct Config {
    // Minimum/maximum local positions.
    double loc0Min = -inf;
    double loc0Max = inf;
    double loc1Min = -inf;
    double loc1Max = inf;
    // Minimum/maximum track time.
    double timeMin = -inf;
    double timeMax = inf;
    // Direction cuts.
    double phiMin = -inf;
    double phiMax = inf;
    double etaMin = -inf;
    double etaMax = inf;
    double absEtaMin = 0.0;
    double absEtaMax = inf;
    // Momentum cuts.
    double ptMin = 0.0;
    double ptMax = inf;

    std::size_t minMeasurements = 0;
    std::size_t maxHoles = std::numeric_limits<std::size_t>::max();
    std::size_t maxOutliers = std::numeric_limits<std::size_t>::max();
    std::size_t maxHolesAndOutliers = std::numeric_limits<std::size_t>::max();
    std::size_t maxSharedHits = std::numeric_limits<std::size_t>::max();
    double maxChi2 = inf;

    // Defaults to: no cut
    MeasurementCounter measurementCounter;

    // Helper factory functions to produce a populated config object more
    // conveniently

    /// Set loc0 acceptance range
    /// @param min Minimum value
    /// @param max Maximum value
    /// @return Reference to this object
    Config& loc0(double min, double max);

    /// Set loc1 acceptance range
    /// @param min Minimum value
    /// @param max Maximum value
    /// @return Reference to this object
    Config& loc1(double min, double max);

    /// Set time acceptance range
    /// @param min Minimum value
    /// @param max Maximum value
    /// @return Reference to this object
    Config& time(double min, double max);

    /// Set phi acceptance range
    /// @param min Minimum value
    /// @param max Maximum value
    /// @return Reference to this object
    Config& phi(double min, double max);

    /// Set the eta acceptance range
    /// @param min Minimum value
    /// @param max Maximum value
    /// @return Reference to this object
    Config& eta(double min, double max);

    /// Set the absolute eta acceptance range
    /// @param min Minimum value
    /// @param max Maximum value
    /// @return Reference to this object
    Config& absEta(double min, double max);

    /// Set the pt acceptance range
    /// @param min Minimum value
    /// @param max Maximum value
    /// @return Reference to this object
    Config& pt(double min, double max);

    /// Print this set of cuts to an output stream
    /// @param os Output stream
    /// @param cuts Cuts to print
    /// @return Reference to the output stream
    friend std::ostream& operator<<(std::ostream& os, const Config& cuts);
  };

  /// Main config object for the track selector. Combines a set of cut
  /// configurations and corresponding eta bins
  struct EtaBinnedConfig {
    /// Cut sets for each eta bin
    std::vector<Config> cutSets = {};

    /// Eta bin edges for varying cuts by eta
    std::vector<double> absEtaEdges = {};

    /// Get the number of eta bins
    /// @return Number of eta bins
    std::size_t nEtaBins() const { return absEtaEdges.size() - 1; }

    /// Construct an empty (accepts everything) configuration.
    /// Results in a single cut set and one abs eta bin from 0 to infinity.
    EtaBinnedConfig() : cutSets{{}}, absEtaEdges{{0, inf}} {};

    /// Constructor to create a config object that is not upper-bounded.
    /// This is useful to use the "fluent" API to populate the configuration.
    /// @param etaMin Minimum eta bin edge
    EtaBinnedConfig(double etaMin) : cutSets{}, absEtaEdges{etaMin} {}

    /// Constructor from a vector of eta bin edges. This automatically
    /// initializes all the cuts to be the same for all eta and be essentially
    /// no-op.
    /// @param absEtaEdgesIn is the vector of eta bin edges
    EtaBinnedConfig(std::vector<double> absEtaEdgesIn)
        : absEtaEdges{std::move(absEtaEdgesIn)} {
      cutSets.resize(absEtaEdges.size() - 1);
    }

    /// Auto-converting constructor from a single cut configuration.
    /// Results in a single absolute eta bin from 0 to infinity.
    EtaBinnedConfig(Config cutSet)
        : cutSets{std::move(cutSet)}, absEtaEdges{{0, inf}} {}

    /// Add a new eta bin with the given upper bound.
    /// @param etaMax Upper bound of the new eta bin
    /// @param callback Callback to configure the cuts for this eta bin
    /// @return Reference to this object
    EtaBinnedConfig& addCuts(double etaMax,
                             const std::function<void(Config&)>& callback = {});

    /// Add a new eta bin with an upper bound of +infinity.
    /// @param callback Callback to configure the cuts for this eta bin
    /// @return Reference to this object
    EtaBinnedConfig& addCuts(const std::function<void(Config&)>& callback = {});

    /// Print this configuration to an output stream
    /// @param os Output stream
    /// @param cfg Configuration to print
    /// @return Reference to the output stream
    friend std::ostream& operator<<(std::ostream& os,
                                    const EtaBinnedConfig& cfg);

    /// Check if the configuration has a bin for a given eta
    /// @param eta Eta value
    /// @return True if the configuration has a bin for the given eta
    bool hasCuts(double eta) const;

    /// Get the index of the eta bin for a given eta
    /// @param eta Eta value
    /// @return Index of the eta bin
    std::size_t binIndex(double eta) const;

    /// Get the cuts for a given eta
    /// @param eta Eta value
    /// @return Cuts for the given eta
    const Config& getCuts(double eta) const;
  };

  /// Constructor from a single cut config object
  /// @param config is the configuration object
  TrackSelector(const Config& config);

  /// Constructor from a multi-eta
  /// @param config is the configuration object
  TrackSelector(const EtaBinnedConfig& config);

  /// Select tracks from an input container and copy them into an output
  /// container
  /// @tparam input_tracks_t is the type of the input track container
  /// @tparam output_tracks_t is the type of the output track container
  /// @param inputTracks is the input track container
  /// @param outputTracks is the output track container
  template <typename input_tracks_t, typename output_tracks_t>
  void selectTracks(const input_tracks_t& inputTracks,
                    output_tracks_t& outputTracks) const;

  /// Helper function to check if a track is valid
  /// @tparam track_proxy_t is the type of the track proxy
  /// @param track is the track proxy
  /// @return true if the track is valid
  template <TrackProxyConcept track_proxy_t>
  bool isValidTrack(const track_proxy_t& track) const;

  /// Get readonly access to the config parameters
  /// @return the config object
  const EtaBinnedConfig& config() const { return m_cfg; }

 private:
  EtaBinnedConfig m_cfg;
  bool m_isUnbinned;
  bool m_noEtaCuts;
};

inline TrackSelector::Config& TrackSelector::Config::loc0(double min,
                                                          double max) {
  loc0Min = min;
  loc0Max = max;
  return *this;
}

inline TrackSelector::Config& TrackSelector::Config::loc1(double min,
                                                          double max) {
  loc1Min = min;
  loc1Max = max;
  return *this;
}

inline TrackSelector::Config& TrackSelector::Config::time(double min,
                                                          double max) {
  timeMin = min;
  timeMax = max;
  return *this;
}

inline TrackSelector::Config& TrackSelector::Config::phi(double min,
                                                         double max) {
  phiMin = min;
  phiMax = max;
  return *this;
}

inline TrackSelector::Config& TrackSelector::Config::eta(double min,
                                                         double max) {
  if (absEtaMin != 0.0 || absEtaMax != inf) {
    throw std::invalid_argument(
        "Cannot set both eta and absEta cuts in the same cut set");
  }
  etaMin = min;
  etaMax = max;
  return *this;
}

inline TrackSelector::Config& TrackSelector::Config::absEta(double min,
                                                            double max) {
  if (etaMin != -inf || etaMax != inf) {
    throw std::invalid_argument(
        "Cannot set both eta and absEta cuts in the same cut set");
  }
  absEtaMin = min;
  absEtaMax = max;
  return *this;
}

inline TrackSelector::Config& TrackSelector::Config::pt(double min,
                                                        double max) {
  ptMin = min;
  ptMax = max;
  return *this;
}

inline std::ostream& operator<<(std::ostream& os,
                                const TrackSelector::Config& cuts) {
  auto print = [&](const char* name, const auto& min, const auto& max) {
    os << " - " << min << " <= " << name << " < " << max << "\n";
  };

  print("loc0", cuts.loc0Min, cuts.loc0Max);
  print("loc1", cuts.loc1Min, cuts.loc1Max);
  print("time", cuts.timeMin, cuts.timeMax);
  print("phi", cuts.phiMin, cuts.phiMax);
  print("eta", cuts.etaMin, cuts.etaMax);
  print("absEta", cuts.absEtaMin, cuts.absEtaMax);
  print("pt", cuts.ptMin, cuts.ptMax);
  print("nHoles", 0, cuts.maxHoles);
  print("nOutliers", 0, cuts.maxOutliers);
  print("nHoles + nOutliers", 0, cuts.maxHolesAndOutliers);
  print("nSharedHits", 0, cuts.maxSharedHits);
  print("chi2", 0.0, cuts.maxChi2);
  os << " - " << cuts.minMeasurements << " <= nMeasurements\n";

  return os;
}

inline TrackSelector::EtaBinnedConfig& TrackSelector::EtaBinnedConfig::addCuts(
    double etaMax, const std::function<void(Config&)>& callback) {
  if (etaMax <= absEtaEdges.back()) {
    throw std::invalid_argument{
        "Abs Eta bin edges must be in increasing order"};
  }

  if (etaMax < 0.0) {
    throw std::invalid_argument{"Abs Eta bin edges must be positive"};
  }

  absEtaEdges.push_back(etaMax);
  cutSets.emplace_back();
  if (callback) {
    callback(cutSets.back());
  }
  return *this;
}

inline TrackSelector::EtaBinnedConfig& TrackSelector::EtaBinnedConfig::addCuts(
    const std::function<void(Config&)>& callback) {
  return addCuts(inf, callback);
}

inline bool TrackSelector::EtaBinnedConfig::hasCuts(double eta) const {
  return std::abs(eta) < absEtaEdges.back();
}

inline std::size_t TrackSelector::EtaBinnedConfig::binIndex(double eta) const {
  if (!hasCuts(eta)) {
    throw std::invalid_argument{"Eta is outside the abs eta bin edges"};
  }

  auto binIt =
      std::upper_bound(absEtaEdges.begin(), absEtaEdges.end(), std::abs(eta));
  std::size_t index = std::distance(absEtaEdges.begin(), binIt) - 1;
  return index;
}

inline const TrackSelector::Config& TrackSelector::EtaBinnedConfig::getCuts(
    double eta) const {
  return nEtaBins() == 1 ? cutSets[0] : cutSets[binIndex(eta)];
}

inline std::ostream& operator<<(std::ostream& os,
                                const TrackSelector::EtaBinnedConfig& cfg) {
  os << "TrackSelector::EtaBinnedConfig:\n";

  for (std::size_t i = 1; i < cfg.absEtaEdges.size(); i++) {
    os << cfg.absEtaEdges[i - 1] << " <= eta < " << cfg.absEtaEdges[i] << "\n";
    os << cfg.cutSets[i - 1];
  }

  return os;
}

template <typename input_tracks_t, typename output_tracks_t>
void TrackSelector::selectTracks(const input_tracks_t& inputTracks,
                                 output_tracks_t& outputTracks) const {
  for (auto track : inputTracks) {
    if (!isValidTrack(track)) {
      continue;
    }
    auto destProxy = outputTracks.makeTrack();
    destProxy.copyFrom(track, false);
    destProxy.tipIndex() = track.tipIndex();
  }
}

template <TrackProxyConcept track_proxy_t>
bool TrackSelector::isValidTrack(const track_proxy_t& track) const {
  auto checkMin = [](auto x, auto min) { return min <= x; };
  auto checkMax = [](auto x, auto max) { return x <= max; };
  auto within = [](double x, double min, double max) {
    return (min <= x) && (x < max);
  };

  const auto theta = track.theta();

  constexpr double kUnset = -std::numeric_limits<double>::infinity();

  double _eta = kUnset;
  double _absEta = kUnset;

  auto absEta = [&]() {
    if (_absEta == kUnset) {
      _eta = -std::log(std::tan(theta / 2));
      _absEta = std::abs(_eta);
    }
    return _absEta;
  };

  const Config* cutsPtr{nullptr};
  if (!m_isUnbinned) {
    if (absEta() < m_cfg.absEtaEdges.front() ||
        _absEta >= m_cfg.absEtaEdges.back()) {
      return false;
    }
    cutsPtr = &m_cfg.getCuts(_eta);
  } else {
    cutsPtr = &m_cfg.cutSets.front();
  }

  const Config& cuts = *cutsPtr;

  return track.hasReferenceSurface() &&
         within(track.transverseMomentum(), cuts.ptMin, cuts.ptMax) &&
         (m_noEtaCuts || (within(absEta(), cuts.absEtaMin, cuts.absEtaMax) &&
                          within(_eta, cuts.etaMin, cuts.etaMax))) &&
         within(track.phi(), cuts.phiMin, cuts.phiMax) &&
         within(track.loc0(), cuts.loc0Min, cuts.loc0Max) &&
         within(track.loc1(), cuts.loc1Min, cuts.loc1Max) &&
         within(track.time(), cuts.timeMin, cuts.timeMax) &&
         checkMin(track.nMeasurements(), cuts.minMeasurements) &&
         checkMax(track.nHoles(), cuts.maxHoles) &&
         checkMax(track.nOutliers(), cuts.maxOutliers) &&
         checkMax(track.nHoles() + track.nOutliers(),
                  cuts.maxHolesAndOutliers) &&
         checkMax(track.nSharedHits(), cuts.maxSharedHits) &&
         checkMax(track.chi2(), cuts.maxChi2) &&
         cuts.measurementCounter.isValidTrack(track);
}

inline TrackSelector::TrackSelector(
    const TrackSelector::EtaBinnedConfig& config)
    : m_cfg(config) {
  if (m_cfg.cutSets.size() != m_cfg.nEtaBins()) {
    throw std::invalid_argument{
        "TrackSelector cut / eta bin configuration is inconsistent"};
  }

  m_isUnbinned = false;
  if (m_cfg.nEtaBins() == 1) {
    static const std::vector<double> infVec = {0, inf};
    bool limitEta = m_cfg.absEtaEdges != infVec;
    m_isUnbinned = !limitEta;  // single bin, no eta edges given

    const Config& cuts = m_cfg.cutSets[0];

    if (limitEta && (cuts.etaMin != -inf || cuts.etaMax != inf ||
                     cuts.absEtaMin != 0.0 || cuts.absEtaMax != inf)) {
      throw std::invalid_argument{
          "Explicit eta cuts are only valid for single eta bin"};
    }
  }

  m_noEtaCuts = m_isUnbinned;
  for (const auto& cuts : m_cfg.cutSets) {
    if (cuts.etaMin != -inf || cuts.etaMax != inf || cuts.absEtaMin != 0.0 ||
        cuts.absEtaMax != inf) {
      m_noEtaCuts = false;
    }
  }
}

inline TrackSelector::TrackSelector(const Config& config)
    : TrackSelector{EtaBinnedConfig{config}} {}

template <TrackProxyConcept track_proxy_t>
bool TrackSelector::MeasurementCounter::isValidTrack(
    const track_proxy_t& track) const {
  // No hit cuts, accept everything
  if (counters.empty()) {
    return true;
  }

  boost::container::small_vector<unsigned int, 4> counterValues;
  counterValues.resize(counters.size(), 0);

  for (const auto& ts : track.trackStatesReversed()) {
    if (!ts.typeFlags().test(Acts::TrackStateFlag::MeasurementFlag)) {
      continue;
    }

    const auto geoId = ts.referenceSurface().geometryId();

    for (std::size_t i = 0; i < counters.size(); i++) {
      const auto& [counterMap, threshold] = counters[i];
      const auto it = counterMap.find(geoId);
      if (it == counterMap.end()) {
        continue;
      }

      counterValues[i]++;
    }
  }

  for (std::size_t i = 0; i < counters.size(); i++) {
    const auto& [counterMap, threshold] = counters[i];
    const unsigned int value = counterValues[i];
    if (value < threshold) {
      return false;
    }
  }

  return true;
}
}  // namespace Acts
