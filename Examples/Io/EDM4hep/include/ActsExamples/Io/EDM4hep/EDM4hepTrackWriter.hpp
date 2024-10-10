// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Plugins/Podio/PodioUtil.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/EventData/Trajectories.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/WriterT.hpp"
#include "ActsFatras/EventData/Hit.hpp"
#include "ActsFatras/EventData/Particle.hpp"

#include <string>

namespace ActsExamples {

class EDM4hepTrackWriter : public WriterT<ConstTrackContainer> {
 public:
  struct Config {
    /// Input track collection
    std::string inputTracks;
    /// Output track collection in edm4hep
    std::string outputTracks = "ActsTracks";
    /// Where to place output file
    std::string outputPath;
    /// Magnetic field along the z axis (needed for the conversion of
    /// parameters)
    double Bz;
  };

  /// constructor
  /// @param config is the configuration object
  /// @param level is the output logging level
  EDM4hepTrackWriter(const Config& config,
                     Acts::Logging::Level level = Acts::Logging::INFO);

  ProcessCode finalize() final;

  /// Readonly access to the config
  const Config& config() const { return m_cfg; }

 protected:
  /// @brief Write method called by the base class
  /// @param [in] context is the algorithm context for consistency
  /// @param [in] tracks is the track collection
  ProcessCode writeT(const AlgorithmContext& context,
                     const ConstTrackContainer& tracks) final;

 private:
  Config m_cfg;

  std::mutex m_writeMutex;

  Acts::PodioUtil::ROOTWriter m_writer;
};

}  // namespace ActsExamples
