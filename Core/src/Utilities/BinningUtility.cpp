// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
#include "Acts/Utilities/BinUtility.hpp"

void Acts::to_json(nlohmann::json& j, const BinUtility& bu) {
  nlohmann::json jbindata;
  for (const auto& bdata : bu.binningData()) {
    jbindata.push_back(nlohmann::json(bdata));
  }
  j["binningdata"] = jbindata;
  if (!bu.transform().isApprox(Transform3::Identity())) {
    nlohmann::json jtrf = bu.transform();
    j["transform"] = jtrf;
  }
}

void Acts::from_json(const nlohmann::json& j, Acts::BinUtility& bu) {
  bu = Acts::BinUtility();
  if (j.find("transform") != j.end() && !j.at("transform").empty()) {
    Acts::Transform3 trf = j.at("transform");
    bu = Acts::BinUtility(trf);
  }
  for (const auto& jdata : j.at("binningdata")) {
    Acts::BinningData bd;
    from_json(jdata, bd);
    bu += Acts::BinUtility(bd);
  }
}
