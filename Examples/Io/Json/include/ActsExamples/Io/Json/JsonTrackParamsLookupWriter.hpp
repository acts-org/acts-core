// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Plugins/Json/GridJsonConverter.hpp"
#include "ActsExamples/TrackFinding/ITrackParamsLookupWriter.hpp"

#include <fstream>

#include <nlohmann/json.hpp>

namespace ActsExamples {

/// @brief Json writer for track parameter lookup tables
///
/// This writer is used to write track parameter lookup tables
/// to a json file to be later used in track parameter estimation
/// for seeding
class JsonTrackParamsLookupWriter final : public ITrackParamsLookupWriter {
 public:
  /// @brief Nested configuration struct
  struct Config {
    /// Output file name
    std::string path;
  };

  /// Constructor
  ///
  /// @param config The configuration struct of the writer
  explicit JsonTrackParamsLookupWriter(const Config& config) : m_cfg(config) {};

  /// Virtual destructor
  ~JsonTrackParamsLookupWriter() override = default;

  /// Write out track parameters lookup table
  ///
  /// @param lookup The lookup to write
  void writeLookup(const TrackParamsLookup& lookup) const override {
    nlohmann::json jLookup;

    // Iterate over the lookup and serialize the grids
    for (const auto& [id, grid] : lookup) {
      nlohmann::json jGrid;
      jGrid["geo_id"] = id.value();
      jGrid["grid"] = Acts::GridJsonConverter::toJson(grid);

      jLookup.push_back(jGrid);
    }

    // Write the json file
    std::ofstream ofj(m_cfg.path, std::ios::out);
    ofj << std::setw(4) << jLookup << std::endl;
  };

  /// Readonly access to the config
  const Config& config() const { return m_cfg; }

 private:
  /// The config of the writer
  Config m_cfg;
};

}  // namespace ActsExamples
