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
#include "Acts/Propagator/MaterialInteractor.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Vertexing/Vertex.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IReader.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

class TChain;

namespace ActsExamples {
struct AlgorithmContext;

class RootAthenaNTupleReader : public ActsExamples::IReader {
 public:
  /// @brief The nested configuration struct
  struct Config {
    // name of the input tree
    std::string inputTreeName;
    // The name of the input file
    std::string inputFilePath;

    std::string outputTrackParameters = "nTupleTrackParameters";
    std::string outputTruthVtxParameters = "nTupleTruthVtxParameters";
    std::string outputRecoVtxParameters = "nTupleRecoVtxParameters";
    std::string outputBeamspotConstraint = "beamspotConstraint";
  };

  // clang-format off
  // name                 | typename                  | interpretation
  // ---------------------+--------------------------+--------------------------------
  // mcChannelNumber      | std::int32_t              | AsDtype('>i4')
  // EventNumber          | std::int32_t              | AsDtype('>i4')
  // RunNumber            | std::int32_t              | AsDtype('>i4')
  // BCID                 | std::int32_t              | AsDtype('>i4')
  // mu                   | float                     | AsDtype('>f4')
  // muActual             | float                     | AsDtype('>f4')
  // beamspot_x           | float                     | AsDtype('>f4')
  // beamspot_y           | float                     | AsDtype('>f4')
  // beamspot_z           | float                     | AsDtype('>f4')
  // beamspot_sigX        | float                     | AsDtype('>f4')
  // beamspot_sigY        | float                     | AsDtype('>f4')
  // beamspot_sigZ        | float                     | AsDtype('>f4')
  // met_Truth            | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // mpx_Truth            | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // mpy_Truth            | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // sumet_Truth          | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // track_prob           | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // track_d0             | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // track_z0             | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // track_theta          | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // track_phi            | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // track_qOverP         | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // track_t              | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // track_z              | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // track_var_d0         | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // track_var_z0         | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // track_var_phi        | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // track_var_theta      | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // track_var_qOverP     | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // track_cov_d0z0       | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // track_cov_d0phi      | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // track_cov_d0theta    | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // track_cov_d0qOverP   | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // track_cov_z0phi      | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // track_cov_z0theta    | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // track_cov_z0qOverP   | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // track_cov_phitheta   | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // track_cov_phiqOverP  | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // track_cov_tehtaqO... | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // track_t30            | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // track_t60            | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // track_t90            | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // track_t120           | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // track_t180           | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // tracks_numPix        | std::vector<std::int32_t> | AsJagged(AsDtype('>i4'), he...
  // tracks_numSCT        | std::vector<std::int32_t> | AsJagged(AsDtype('>i4'), he...
  // tracks_numPix1L      | std::vector<std::int32_t> | AsJagged(AsDtype('>i4'), he...
  // tracks_numPix2L      | std::vector<std::int32_t> | AsJagged(AsDtype('>i4'), he...
  // jet_pt               | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // jet_eta              | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // jet_phi              | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // jet_m                | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // jet_q                | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // jet_ptmatched_pt     | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // jet_ptmatched_eta    | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // jet_ptmatched_phi    | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // jet_ptmatched_m      | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // jet_drmatched_pt     | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // jet_drmatched_eta    | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // jet_drmatched_phi    | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // jet_drmatched_m      | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // jet_isPU             | std::vector<std::int32_t> | AsJagged(AsDtype('>i4'), he...
  // jet_isHS             | std::vector<std::int32_t> | AsJagged(AsDtype('>i4'), he...
  // jet_label            | std::vector<std::int32_t> | AsJagged(AsDtype('>i4'), he...
  // recovertex_x         | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // recovertex_y         | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // recovertex_z         | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // recovertex_sumPt2    | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // recovertex_isPU      | std::vector<std::int32_t> | AsJagged(AsDtype('>i4'), he...
  // recovertex_isHS      | std::vector<std::int32_t> | AsJagged(AsDtype('>i4'), he...
  // truthvertex_x        | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // truthvertex_y        | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // truthvertex_z        | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // truthvertex_t        | std::vector<float>        | AsJagged(AsDtype('>f4'), he...
  // truthvertex_isPU     | std::vector<std::int32_t> | AsJagged(AsDtype('>i4'), he...
  // truthvertex_isHS     | std::vector<std::int32_t> | AsJagged(AsDtype('>i4'), he...
  // jet_tracks_idx       | std::vector<std::vect...  | AsObjects(AsVector(True, As...
  // recovertex_tracks... | std::vector<std::vect...  | AsObjects(AsVector(True, As...
  // truthvertex_track... | std::vector<std::vect...  | AsObjects(AsVector(True, As...
  // clang-format on

  struct BranchPointerWrapper {
    std::vector<float> track_d0;
    std::vector<float> track_z0;
    std::vector<float> track_theta;
    std::vector<float> track_phi;
    std::vector<float> track_qOverP;
    std::vector<float> track_t;
    std::vector<float> track_z;

    std::vector<float> track_var_d0;
    std::vector<float> track_var_z0;
    std::vector<float> track_var_phi;
    std::vector<float> track_var_theta;
    std::vector<float> track_var_qOverP;
    std::vector<float> track_cov_d0z0;
    std::vector<float> track_cov_d0phi;
    std::vector<float> track_cov_d0theta;
    std::vector<float> track_cov_d0qOverP;
    std::vector<float> track_cov_z0phi;
    std::vector<float> track_cov_z0theta;
    std::vector<float> track_cov_z0qOverP;
    std::vector<float> track_cov_phitheta;
    std::vector<float> track_cov_phiqOverP;
    std::vector<float> track_cov_tehtaqOverP;

    std::vector<float> truthvertex_x;
    std::vector<float> truthvertex_y;
    std::vector<float> truthvertex_z;
    std::vector<float> truthvertex_t;

    std::vector<float> recovertex_x;
    std::vector<float> recovertex_y;
    std::vector<float> recovertex_z;

    std::vector<std::vector<int>> truthvertex_tracks_idx;

    float beamspot_x = 0;
    float beamspot_y = 0;
    float beamspot_z = 0;
    float beamspot_sigX = 0;
    float beamspot_sigY = 0;
    float beamspot_sigZ = 0;
  };

  /// Constructor
  /// @param config The Configuration struct
  RootAthenaNTupleReader(const Config &config, Acts::Logging::Level level);

  /// Framework name() method
  std::string name() const final { return "RootAthenaNTupleReader"; }

  /// Return the available events range.
  std::pair<std::size_t, std::size_t> availableEvents() const final {
    return {0u, m_events};
  }

  /// Read out data from the input stream
  ///
  /// @param context The algorithm context
  ActsExamples::ProcessCode read(
      const ActsExamples::AlgorithmContext &context) final;

  /// Readonly access to the config
  const Config &config() const { return m_cfg; }

  /// Readonly access to the branches
  const BranchPointerWrapper &branches() const { return m_branches; }

 private:
  /// Private access to the logging instance
  const Acts::Logger &logger() const { return *m_logger; }

  /// The config class
  Config m_cfg;

  std::unique_ptr<const Acts::Logger> m_logger;

  /// mutex used to protect multi-threaded reads
  std::mutex m_read_mutex;

  /// The number of events
  std::size_t m_events = 0;

  /// The input tree name
  TChain *m_inputChain = nullptr;

  /// The handle to branches in current event
  BranchPointerWrapper m_branches;

  WriteDataHandle<TrackParametersContainer> m_outputTrackParameters{
      this, "OutputTrackParameters"};

  WriteDataHandle<std::vector<Acts::Vector4>> m_outputTruthVtxParameters{
      this, "OutputTruthVertices"};

  WriteDataHandle<std::vector<Acts::Vector4>> m_outputRecoVtxParameters{
      this, "OutputRecoVertices"};

  WriteDataHandle<Acts::Vertex> m_outputBeamspotConstraint{
      this, "OutputBeamsspotConstraint"};
};

}  // namespace ActsExamples
