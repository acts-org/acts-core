// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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
    for (auto ibv : allAxisDirections()) {
      if (e.constrains(ibv)) {
        jrange[axisDirectionName(ibv)] = xrange[toUnderlying(ibv)];
      }
    }
    j["range"] = jrange;
  }

  {
    nlohmann::json jenvelope;
    const auto& envelope = e.envelope();
    for (auto ibv : allAxisDirections()) {
      if (envelope[ibv] != zeroEnvelope) {
        jenvelope[axisDirectionName(ibv)] =
            Range1D<double>(envelope[ibv][0], envelope[ibv][1]);
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
    AxisDirection bval = axisDirectionFromName(key);
    e.set(bval, value["min"], value["max"]);
  }

  if (j.contains("envelope")) {
    const auto& jenvelope = j["envelope"];
    ExtentEnvelope envelope;

    for (const auto& [key, value] : jenvelope.items()) {
      AxisDirection bval = axisDirectionFromName(key);
      envelope[bval] = {value["min"], value["max"]};
    }

    e.setEnvelope(envelope);
  }
}
