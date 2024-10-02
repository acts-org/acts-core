// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Json/ExtentJsonConverter.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Plugins/Json/UtilitiesJsonConverter.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Enumerate.hpp"
#include "Acts/Utilities/RangeXD.hpp"

#include <array>
#include <iterator>
#include <vector>

void Acts::to_json(nlohmann::json& j, const Acts::Extent& e) {
  {
    nlohmann::json jrange;
    const auto& xrange = e.range();
    for (auto ibv : allBinningValues()) {
      if (e.constrains(ibv)) {
        jrange[binningValueName(ibv)] = xrange[toUnderlying(ibv)];
      }
    }
    j["range"] = jrange;
  }

  {
    nlohmann::json jenvelope;
    const auto& envelope = e.envelope();
    for (auto ibv : allBinningValues()) {
      if (envelope[ibv] != zeroEnvelope) {
        jenvelope[binningValueName(ibv)] =
            Range1D<ActsScalar>(envelope[ibv][0], envelope[ibv][1]);
      }
    }
    if (!jenvelope.empty()) {
      j["envelope"] = jenvelope;
    }
  }
}

void Acts::from_json(const nlohmann::json& j, Acts::Extent& e) {
  const auto& jrange = j["range"];

  for (const auto& [key, value] : jrange.items()) {
    BinningValue bval = binningValueFromName(key);
    e.set(bval, value["min"], value["max"]);
  }

  if (j.contains("envelope")) {
    const auto& jenvelope = j["envelope"];
    ExtentEnvelope envelope;

    for (const auto& [key, value] : jenvelope.items()) {
      BinningValue bval = binningValueFromName(key);
      envelope[bval] = {value["min"], value["max"]};
    }

    e.setEnvelope(envelope);
  }
}
