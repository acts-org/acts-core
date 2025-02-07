// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Utilities/AxisDefinitions.hpp"

#include <algorithm>
#include <ostream>
#include <stdexcept>

namespace Acts {

namespace {

// Legacy binning value names
//
// NOTE: this should be removed once the BinUtility is removed
const std::vector<std::string> s_legacyBinningValueNames = {
    "binX",    "binY", "binZ",   "binR",  "binPhi",
    "binRPhi", "binH", "binEta", "binMag"};
// end of legacy binning values

const std::vector<std::string> s_axisDirectionNames = {
    "AxisX",    "AxisY",     "AxisZ",   "AxisR",  "AxisPhi",
    "AxisRPhi", "AxisTheta", "AxisEta", "AxisMag"};

const std::vector<AxisDirection> s_axisDirections = {
    AxisDirection::AxisX,     AxisDirection::AxisY,   AxisDirection::AxisZ,
    AxisDirection::AxisR,     AxisDirection::AxisPhi, AxisDirection::AxisRPhi,
    AxisDirection::AxisTheta, AxisDirection::AxisEta, AxisDirection::AxisMag};

}  // namespace

const std::vector<AxisDirection>& allAxisDirections() {
  return s_axisDirections;
}

AxisDirection axisDirectionFromName(const std::string& name) {
  auto it = std::ranges::find(s_axisDirectionNames, name);
  if (it == s_axisDirectionNames.end()) {
    // Legacy binning check - this should be removed once BinUtility is gone
    it = std::ranges::find(s_legacyBinningValueNames, name);
    if (it == s_legacyBinningValueNames.end()) {
      throw std::invalid_argument("Unknown AxisDirection value name: " + name);
    }
    // both legacy and current failed
  }
  return static_cast<AxisDirection>(
      std::distance(s_axisDirectionNames.begin(), it));
}

const std::string& axisDirectionName(AxisDirection aDir) {
  return s_axisDirectionNames.at(
      static_cast<std::underlying_type_t<AxisDirection>>(aDir));
}

std::ostream& operator<<(std::ostream& os, AxisDirection aDir) {
  os << axisDirectionName(aDir);
  return os;
}

}  // namespace Acts
