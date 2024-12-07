// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Utilities/BinningData.hpp"

void Acts::to_json(nlohmann::json& j, const Acts::BinningData& bd) {
  // Common to all bin utilities
  j["min"] = bd.min;
  j["max"] = bd.max;
  j["option"] = (bd.option == Acts::open ? "open" : "closed");
  j["value"] = bd.binvalue;
  int bins = bd.bins();
  // Write sub bin data if present
  if (bd.subBinningData != nullptr) {
    nlohmann::json subjson;
    to_json(subjson, *bd.subBinningData);
    j["subdata"] = subjson;
    j["subadditive"] = bd.subBinningAdditive;
    // this modifies the bins as bins() returns total number in general
    if (bd.subBinningAdditive) {
      bins -= static_cast<int>(subjson["bins"]) + 1;
    } else {
      bins /= static_cast<int>(subjson["bins"]);
    }
  }
  // Now distinguish between equidistant / arbitrary
  if (bd.type == Acts::equidistant) {
    j["type"] = "equidistant";
  } else if (bd.type == Acts::arbitrary) {
    j["type"] = "arbitrary";
    j["boundaries"] = bd.boundaries();
  }
  j["bins"] = bins;
}

void Acts::from_json(const nlohmann::json& j, BinningData& bd) {
  // Common to all bin utilities
  float min = j["min"];
  float max = j["max"];
  int bins = j["bins"];
  auto bValue = j["value"].get<BinningValue>();
  if (bins == 1 && !(j["type"] == "arbitrary")) {
    bd = BinningData(bValue, min, max);
    return;
  }
  Acts::BinningOption bOption = (j["option"] == "open") ? open : closed;
  Acts::BinningType bType =
      (j["type"] == "equidistant") ? equidistant : arbitrary;

  std::unique_ptr<BinningData> subBinning = nullptr;
  bool subBinningAdditive = false;
  if (j.find("subdata") != j.end()) {
    subBinningAdditive = j["subadditive"];
  }

  if (bType == equidistant) {
    bd = BinningData(bOption, bValue, bins, min, max, std::move(subBinning),
                     subBinningAdditive);
  } else {
    std::vector<float> boundaries = j["boundaries"];
    bd = BinningData(bOption, bValue, boundaries, std::move(subBinning));
  }
}
