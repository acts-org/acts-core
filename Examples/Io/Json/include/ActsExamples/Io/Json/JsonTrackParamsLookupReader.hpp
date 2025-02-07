// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Plugins/Json/GridJsonConverter.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "ActsExamples/TrackFinding/ITrackParamsLookupReader.hpp"

#include <fstream>

#include <nlohmann/json.hpp>

namespace ActsExamples {

/// @brief Json reader for track parameter lookup tables
///
/// This reader is used to read track parameter lookup tables
/// from a json file to be later used in track parameter estimation
/// for seeding
class JsonTrackParamsLookupReader final : public ITrackParamsLookupReader {
 public:
  /// @brief Nested configuration struct
  struct Config {
    /// Reference tracking layers
    std::unordered_map<Acts::GeometryIdentifier, const Acts::Surface*>
        refLayers;
    /// Binning of the grid to be emposed
    /// onto the reference layers
    std::pair<std::size_t, std::size_t> bins;
  };

  explicit JsonTrackParamsLookupReader(const Config& config) : m_cfg(config) {};

  ~JsonTrackParamsLookupReader() override = default;

  /// @brief Read the lookup from a json file
  ///
  /// @param path path to the json file
  ///
  /// @return lookup table for track parameter estimation
  TrackParamsLookup readLookup(const std::string& path) const override {
    // Read the json file
    std::ifstream ifj(path);
    nlohmann::json jLookup;
    ifj >> jLookup;

    TrackParamsLookup lookup;
    // Iterate over the json and deserialize the grids
    for (const auto& jGrid : jLookup) {
      Acts::GeometryIdentifier id(jGrid["geo_id"]);

      if (!m_cfg.refLayers.contains(id)) {
        throw std::invalid_argument("Geometry identifier not found");
      }

      const auto* refSurface = m_cfg.refLayers.at(id);

      // Get bounds to construct the lookup grid
      auto bounds =
          dynamic_cast<const Acts::RectangleBounds*>(&refSurface->bounds());

      if (bounds == nullptr) {
        throw std::invalid_argument("Only rectangle bounds supported");
      }

      // Axis is not deserilizable, so we need to recreate it
      auto halfX = bounds->halfLengthX();
      auto halfY = bounds->halfLengthY();

      TrackParamsLookupAxisGen axisGen{{-halfX, halfX},
                                       m_cfg.bins.first,
                                       {-halfY, halfY},
                                       m_cfg.bins.second};

      // Deserialize the grid
      TrackParamsLookupGrid grid =
          Acts::GridJsonConverter::fromJson<TrackParamsLookupAxisGen,
                                            TrackParamsLookupPair>(
              jGrid["grid"], axisGen);

      lookup.try_emplace(id, std::move(grid));
    }

    return lookup;
  };

  /// Readonly access to the config
  const Config& config() const { return m_cfg; }

 private:
  /// The config of the writer
  Config m_cfg;
};

}  // namespace ActsExamples
