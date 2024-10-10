// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Utilities/BinningType.hpp"

#include <algorithm>
#include <ostream>
#include <stdexcept>

namespace Acts {

namespace {

static const std::vector<std::string> s_binningValueNames = {
    "binX",    "binY", "binZ",   "binR",  "binPhi",
    "binRPhi", "binH", "binEta", "binMag"};

static const std::vector<BinningValue> s_binningValues = {
    BinningValue::binX, BinningValue::binY,   BinningValue::binZ,
    BinningValue::binR, BinningValue::binPhi, BinningValue::binRPhi,
    BinningValue::binH, BinningValue::binEta, BinningValue::binMag};

}  // namespace

const std::vector<BinningValue>& allBinningValues() {
  return s_binningValues;
}

BinningValue binningValueFromName(const std::string& name) {
  auto it = std::ranges::find(s_binningValueNames, name);
  if (it == s_binningValueNames.end()) {
    throw std::invalid_argument("Unknown binning value name: " + name);
  }
  return static_cast<BinningValue>(
      std::distance(s_binningValueNames.begin(), it));
}

const std::string& binningValueName(BinningValue bValue) {
  return s_binningValueNames.at(
      static_cast<std::underlying_type_t<BinningValue>>(bValue));
}

std::ostream& operator<<(std::ostream& os, BinningValue bValue) {
  os << binningValueName(bValue);
  return os;
}

}  // namespace Acts
