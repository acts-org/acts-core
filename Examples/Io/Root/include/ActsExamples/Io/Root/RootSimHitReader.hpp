// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Propagator/MaterialInteractor.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IReader.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

class TChain;

namespace ActsExamples {

/// @class RootParticleReader
///
/// @brief Reads in Particles information from a root file
class RootSimHitReader : public IReader {
 public:
  /// @brief The nested configuration struct
  struct Config {
    /// name of the whiteboard entry
    std::string outputSimHits = "simhits";
    /// name of the output tree
    std::string treeName = "hits";
    ///< The name of the input file
    std::string filePath;
  };

  RootSimHitReader(const RootSimHitReader &) = delete;
  RootSimHitReader(const RootSimHitReader &&) = delete;

  /// Constructor
  /// @param config The Configuration struct
  RootSimHitReader(const Config &config, Acts::Logging::Level level);

  /// Framework name() method
  std::string name() const override { return "RootSimHitReader"; }

  /// Return the available events range.
  std::pair<std::size_t, std::size_t> availableEvents() const override;

  /// Read out data from the input stream
  ///
  /// @param context The algorithm context
  ProcessCode read(const ActsExamples::AlgorithmContext &context) override;

  /// Readonly access to the config
  const Config &config() const { return m_cfg; }

 private:
  /// Private access to the logging instance
  const Acts::Logger &logger() const { return *m_logger; }

  /// The config class
  Config m_cfg;

  WriteDataHandle<SimHitContainer> m_outputSimHits{this, "OutputSimHits"};
  std::unique_ptr<const Acts::Logger> m_logger;

  /// mutex used to protect multi-threaded reads
  std::mutex m_read_mutex;

  /// Vector of {eventNr, entryMin, entryMax}
  std::vector<std::tuple<std::uint32_t, std::size_t, std::size_t>> m_eventMap;

  /// The input tree name
  TChain *m_inputChain = nullptr;

  /// The keys we have in the ROOT file
  constexpr static std::array<const char *, 12> m_floatKeys = {
      "tx",  "ty", "tz",      "tt",      "tpx",     "tpy",
      "tpz", "te", "deltapx", "deltapy", "deltapz", "deltae"};
  constexpr static std::array<const char *, 2> m_uint64Keys = {"geometry_id",
                                                               "particle_id"};
  constexpr static std::array<const char *, 6> m_uint32Keys = {
      "event_id", "volume_id",   "boundary_id",
      "layer_id", "approach_id", "sensitive_id"};
  constexpr static std::array<const char *, 1> m_int32Keys = {"index"};

  std::unordered_map<std::string_view, float> m_floatColumns;
  std::unordered_map<std::string_view, std::uint32_t> m_uint32Columns;
  std::unordered_map<std::string_view, std::int32_t> m_int32Columns;

  // For some reason I need to use here `unsigned long long` instead of
  // `std::uint64_t` to prevent an internal ROOT type mismatch...
  std::unordered_map<std::string_view, unsigned long long> m_uint64Columns;
};

}  // namespace ActsExamples
