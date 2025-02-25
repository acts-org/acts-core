// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace ActsExamples{
 /// @brief Example implementation of a muon hough transform seeder
/// Uses the hough tools from the ACTS Core repo
/// Reads CSV files with muon sim hits (= true trajectories)
/// and drift circles (= measurements), performs
/// a hough transform to the drift circles in each station,
/// and compares to the true parameters of the sim hit in the
/// given station.
class MuonSegmentFinder final : public IAlgorithm {
    public:
     /// config
     struct Config {
       std::string inHoughSeeds{};
       std::string outSegments{};
     };
   
     MuonSegmentFinder(Config cfg, Acts::Logging::Level lvl);
   
     /// Run the seeding algorithm.
     ///
     /// @param ctx is the algorithm context with event information
     /// @return a process code indication success or failure
     ProcessCode execute(const AlgorithmContext& ctx) const final;
     ProcessCode initialize() final;
     ProcessCode finalize() final;
   
     /// Const access to the config
     const Config& config() const { return m_cfg; }
   
    private:
     Config m_cfg;
     std::unique_ptr<const Acts::Logger> m_logger;
     const Acts::Logger& logger() const { return *m_logger; }
   
       /// ReadDataHandle<SimHitContainer> m_inputSimHits{this, "InputSimHits"};
       /// ReadDataHandle<DriftCircleContainer> m_inputDriftCircles{this,
       ///                                                          "InputDriftCircles"};
     };   
}