// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Json/AmbiguityConfigJsonConverter.hpp"

#include "Acts/AmbiguityResolution/ScoreBasedAmbiguityResolution.hpp"
#include "Acts/Plugins/Json/ActsJson.hpp"

namespace Acts {

void from_json(const nlohmann::json& j, ConfigPair& p) {
  std::vector<ScoreBasedAmbiguityResolution::DetectorConfig> detectorConfigs;
  std::map<std::size_t, std::size_t> volumeMap;

  for (auto& [key, value] : j.items()) {
    ScoreBasedAmbiguityResolution::DetectorConfig detectorConfig;

    std::size_t detectorId = std::stoi(key);

    detectorConfig.hitsScoreWeight = value["hitsScoreWeight"];
    detectorConfig.holesScoreWeight = value["holesScoreWeight"];
    detectorConfig.outliersScoreWeight = value["outliersScoreWeight"];
    detectorConfig.otherScoreWeight = value["otherScoreWeight"];

    detectorConfig.minHits = value["minHits"];
    detectorConfig.maxHits = value["maxHits"];
    detectorConfig.maxHoles = value["maxHoles"];
    detectorConfig.maxOutliers = value["maxOutliers"];
    detectorConfig.maxSharedHits = value["maxSharedHits"];

    detectorConfig.sharedHitsFlag = value["sharedHitsFlag"];

    const std::vector<double>& goodHits = value["goodHits"];
    const std::vector<double>& goodHoles = value["goodHoles"];

    const std::vector<double>& fakeHits = value["fakeHits"];
    const std::vector<double>& fakeHoles = value["fakeHoles"];

    if (goodHits.size() != fakeHits.size()) {
      throw std::invalid_argument("goodHits and FakeHits size mismatch");
    }

    for (std::size_t i = 0; i < goodHits.size(); i++) {
      detectorConfig.factorHits.push_back(goodHits[i] / fakeHits[i]);
    }

    if (goodHoles.size() != fakeHoles.size()) {
      throw std::invalid_argument("goodHoles and FakeHoles size mismatch");
    }

    for (std::size_t i = 0; i < goodHoles.size(); i++) {
      detectorConfig.factorHoles.push_back(goodHoles[i] / fakeHoles[i]);
    }

    detectorConfigs.push_back(detectorConfig);

    std::vector<std::size_t> volumesIds = value["volumesIds"];
    for (auto volumeId : volumesIds) {
      volumeMap[volumeId] = detectorId;
    }
  }
  p = std::make_pair(volumeMap, detectorConfigs);
}

}  // namespace Acts
