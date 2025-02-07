// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IReader.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include <Acts/Definitions/Algebra.hpp>
#include <Acts/Propagator/MaterialInteractor.hpp>
#include <Acts/Utilities/Logger.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

class TChain;

namespace ActsExamples {

/// @class RootTrackSummaryReader
///
/// @brief Reads in TrackParameter information from a root file
/// and fills it into a Acts::BoundTrackParameter format
class RootTrackSummaryReader : public IReader {
 public:
  /// @brief The nested configuration struct
  struct Config {
    /// track collection to read
    std::string outputTracks = "outputTracks";
    /// particle collection to read
    std::string outputParticles = "outputParticles";
    /// name of the input tree
    std::string treeName = "tracksummary";
    /// The name of the input file
    std::string filePath;
  };

  /// Constructor
  /// @param config The Configuration struct
  /// @param level The log level
  RootTrackSummaryReader(const Config& config, Acts::Logging::Level level);

  /// Destructor
  ~RootTrackSummaryReader() override;

  /// Framework name() method
  std::string name() const override { return "RootTrackSummaryReader"; }

  /// Return the available events range.
  std::pair<std::size_t, std::size_t> availableEvents() const override;

  /// Read out data from the input stream
  ///
  /// @param context The algorithm context
  ProcessCode read(const ActsExamples::AlgorithmContext& context) override;

  /// Readonly access to the config
  const Config& config() const { return m_cfg; }

 private:
  /// The logger
  std::unique_ptr<const Acts::Logger> m_logger;

  /// Private access to the logging instance
  const Acts::Logger& logger() const { return *m_logger; }

  /// The config class
  Config m_cfg;

  WriteDataHandle<TrackParametersContainer> m_outputTrackParameters{
      this, "OutputTrackParameters"};

  WriteDataHandle<SimParticleContainer> m_outputParticles{this,
                                                          "OutputParticles"};

  /// mutex used to protect multi-threaded reads
  std::mutex m_read_mutex;

  /// The number of events
  std::size_t m_events = 0;

  /// The input tree name
  std::unique_ptr<TChain> m_inputChain;

  /// the event number
  std::uint32_t m_eventNr{0};
  /// the multi-trajectory number
  std::vector<std::uint32_t>* m_multiTrajNr = new std::vector<std::uint32_t>;
  /// the multi-trajectory sub-trajectory number
  std::vector<unsigned int>* m_subTrajNr = new std::vector<unsigned int>;

  /// The entry numbers for accessing events in increased order (there could be
  /// multiple entries corresponding to one event number)
  std::vector<long long> m_entryNumbers = {};

  /// The number of states
  std::vector<unsigned int>* m_nStates = new std::vector<unsigned int>;
  /// The number of measurements
  std::vector<unsigned int>* m_nMeasurements = new std::vector<unsigned int>;
  /// The number of outliers
  std::vector<unsigned int>* m_nOutliers = new std::vector<unsigned int>;
  /// The number of holes
  std::vector<unsigned int>* m_nHoles = new std::vector<unsigned int>;
  /// The total chi2
  std::vector<float>* m_chi2Sum = new std::vector<float>;
  /// The number of ndf of the measurements+outliers
  std::vector<unsigned int>* m_NDF = new std::vector<unsigned int>;
  /// The chi2 on all measurement states
  std::vector<std::vector<double>>* m_measurementChi2 =
      new std::vector<std::vector<double>>;
  /// The chi2 on all outlier states
  std::vector<std::vector<double>>* m_outlierChi2 =
      new std::vector<std::vector<double>>;
  /// The volume id of the measurements
  std::vector<std::vector<std::uint32_t>>* m_measurementVolume =
      new std::vector<std::vector<std::uint32_t>>;
  /// The layer id of the measurements
  std::vector<std::vector<std::uint32_t>>* m_measurementLayer =
      new std::vector<std::vector<std::uint32_t>>;
  /// The volume id of the outliers
  std::vector<std::vector<std::uint32_t>>* m_outlierVolume =
      new std::vector<std::vector<std::uint32_t>>;
  /// The layer id of the outliers
  std::vector<std::vector<std::uint32_t>>* m_outlierLayer =
      new std::vector<std::vector<std::uint32_t>>;

  // The majority truth particle info
  /// The number of hits from majority particle
  std::vector<unsigned int>* m_nMajorityHits = new std::vector<unsigned int>;
  /// The particle Id of the majority particle
  std::vector<std::uint64_t>* m_majorityParticleId =
      new std::vector<std::uint64_t>;
  /// Charge of majority particle
  std::vector<int>* m_t_charge = new std::vector<int>;
  /// Time of majority particle
  std::vector<float>* m_t_time = new std::vector<float>;
  /// Vertex x positions of majority particle
  std::vector<float>* m_t_vx = new std::vector<float>;
  /// Vertex y positions of majority particle
  std::vector<float>* m_t_vy = new std::vector<float>;
  /// Vertex z positions of majority particle
  std::vector<float>* m_t_vz = new std::vector<float>;
  /// Initial momenta px of majority particle
  std::vector<float>* m_t_px = new std::vector<float>;
  /// Initial momenta py of majority particle
  std::vector<float>* m_t_py = new std::vector<float>;
  /// Initial momenta pz of majority particle
  std::vector<float>* m_t_pz = new std::vector<float>;
  /// Initial momenta theta of majority particle
  std::vector<float>* m_t_theta = new std::vector<float>;
  /// Initial momenta phi of majority particle
  std::vector<float>* m_t_phi = new std::vector<float>;
  /// Initial momenta pT of majority particle
  std::vector<float>* m_t_pT = new std::vector<float>;
  /// Initial momenta eta of majority particle
  std::vector<float>* m_t_eta = new std::vector<float>;

  /// If the track has fitted parameter
  std::vector<bool>* m_hasFittedParams = new std::vector<bool>;
  /// Fitted parameters eBoundLoc0 of track
  std::vector<float>* m_eLOC0_fit = new std::vector<float>;
  /// Fitted parameters eBoundLoc1 of track
  std::vector<float>* m_eLOC1_fit = new std::vector<float>;
  /// Fitted parameters ePHI of track
  std::vector<float>* m_ePHI_fit = new std::vector<float>;
  /// Fitted parameters eTHETA of track
  std::vector<float>* m_eTHETA_fit = new std::vector<float>;
  /// Fitted parameters eQOP of track
  std::vector<float>* m_eQOP_fit = new std::vector<float>;
  /// Fitted parameters eT of track
  std::vector<float>* m_eT_fit = new std::vector<float>;
  /// Fitted parameters eLOC err of track
  std::vector<float>* m_err_eLOC0_fit = new std::vector<float>;
  /// Fitted parameters eBoundLoc1 err of track
  std::vector<float>* m_err_eLOC1_fit = new std::vector<float>;
  /// Fitted parameters ePHI err of track
  std::vector<float>* m_err_ePHI_fit = new std::vector<float>;
  /// Fitted parameters eTHETA err of track
  std::vector<float>* m_err_eTHETA_fit = new std::vector<float>;
  /// Fitted parameters eQOP err of track
  std::vector<float>* m_err_eQOP_fit = new std::vector<float>;
  /// Fitted parameters eT err of track
  std::vector<float>* m_err_eT_fit = new std::vector<float>;
};

}  // namespace ActsExamples
