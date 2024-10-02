// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Json/ProtoDetectorJsonConverter.hpp"

#include "Acts/Detector/ProtoDetector.hpp"
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Plugins/Json/ExtentJsonConverter.hpp"
#include "Acts/Plugins/Json/SurfaceJsonConverter.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/BinningData.hpp"

#include <optional>
#include <string>
#include <vector>

void Acts::to_json(nlohmann::json& j, const Acts::ProtoVolume& pv) {
  j["name"] = pv.name;
  j["extent"] = pv.extent;

  /// Helper m ethod to write binnings
  ///
  /// @param root the json root into which this is written
  /// @param binning the vector of binning data
  /// @param key the key for the root writing
  auto writeBinning = [&](nlohmann::json& root,
                          const std::vector<BinningData>& binning,
                          const std::string& key) -> void {
    nlohmann::json jbinning;
    for (const auto& bd : binning) {
      jbinning.push_back(bd);
    }
    root[key] = jbinning;
  };

  // The internal structure
  if (pv.internal.has_value()) {
    auto& its = pv.internal.value();
    nlohmann::json jinternal;
    if (its.layerType != Surface::SurfaceType::Other) {
      jinternal["layerType"] = its.layerType;
    }
    if (!its.surfaceBinning.empty()) {
      writeBinning(jinternal, its.surfaceBinning, "surfaceBinning");
    }
    j["internalStructure"] = jinternal;
  }

  // The container structure
  if (pv.container.has_value()) {
    auto& cts = pv.container.value();
    nlohmann::json jcontainer;
    nlohmann::json jconstituents;
    for (const auto& pvc : cts.constituentVolumes) {
      jconstituents.push_back(pvc);
    }
    jcontainer["constituents"] = jconstituents;
    writeBinning(jcontainer, cts.constituentBinning, "constituentBinning");
    jcontainer["layerContainer"] = cts.layerContainer;
    j["containerStructure"] = jcontainer;
  }
}

void Acts::from_json(const nlohmann::json& j, Acts::ProtoVolume& pv) {
  pv.name = j["name"];
  pv.extent = j["extent"];

  /// Helper method to read binnings
  ///
  /// @param root is the json root
  /// @param binning is the vector of binning data to be filled
  /// @param key is the lookup key
  auto readBinning = [&](const nlohmann::json& root,
                         std::vector<BinningData>& binning,
                         const std::string& key) -> void {
    // return if no surface binning in json
    if (root.find(key) == root.end() || root[key].is_null()) {
      return;
    }

    for (const auto& jbinning : root[key]) {
      binning.push_back(jbinning);
    }
  };

  // The internal structure
  if (j.find("internalStructure") != j.end() &&
      !j["internalStructure"].is_null()) {
    auto& jinternal = j["internalStructure"];
    Surface::SurfaceType layerType =
        static_cast<Surface::SurfaceType>(jinternal["layerType"]);
    std::vector<BinningData> surfaceBinning;
    readBinning(jinternal, surfaceBinning, "surfaceBinning");
    pv.internal = ProtoVolume::InternalStructure{layerType, surfaceBinning};
  }

  // The container structure
  if (j.find("containerStructure") != j.end() &&
      !j["containerStructure"].is_null()) {
    std::vector<ProtoVolume> constituentVolumes;
    auto& jcontainer = j["containerStructure"];
    for (const auto& jc : jcontainer["constituents"]) {
      constituentVolumes.push_back(jc);
    }
    std::vector<BinningData> constituentBinning;
    readBinning(jcontainer, constituentBinning, "constituentBinning");
    bool layerContainer = jcontainer["layerContainer"];
    pv.container = ProtoVolume::ContainerStructure{
        constituentVolumes, constituentBinning, layerContainer};
  }
}

void Acts::to_json(nlohmann::json& j, const Acts::ProtoDetector& pd) {
  j["name"] = pd.name;
  j["world"] = pd.worldVolume;
}

void Acts::from_json(const nlohmann::json& j, Acts::ProtoDetector& pd) {
  pd.name = j["name"];
  pd.worldVolume = j["world"];
}
