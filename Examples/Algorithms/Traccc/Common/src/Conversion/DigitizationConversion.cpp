// This file is part of the Acts project.
//
// Copyright (C) 2023 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// Acts include(s)
#include "Acts/Geometry/GeometryHierarchyMap.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Utilities/BinUtility.hpp"

// Acts Examples include(s)
#include "ActsExamples/Digitization/DigitizationConfig.hpp"
#include "ActsExamples/Traccc/Common/Conversion/DigitizationConversion.hpp"

// Traccc include(s)
#include "traccc/io/digitization_config.hpp"

// System include(s).
#include <utility>
#include <vector>

namespace ActsExamples::Traccc::Common::Conversion {

/// @brief Get the segmentation from a DigiComponentsConfig.
inline Acts::BinUtility getSegmentation(const DigiComponentsConfig& dcc) {
  return dcc.geometricDigiConfig.segmentation;
}

/// @brief Creates a traccc digitalization config from an Acts geometry hierarchy map
/// that contains the digitization configuration.
/// @param config the Acts geometry hierarchy map that contains the digitization configuration.
/// @return a traccc digitization config.
traccc::digitization_config tracccConfig(
    const Acts::GeometryHierarchyMap<DigiComponentsConfig>& config) {
  using ElementType =
      std::pair<Acts::GeometryIdentifier, traccc::module_digitization_config>;
  std::vector<ElementType> vec;
  for (auto& e : config.getElements()) {
    vec.push_back({e.first, traccc::module_digitization_config{
                                getSegmentation(e.second)}});
  }
  return traccc::digitization_config(vec);
}

}  // namespace ActsExamples::Traccc::Common::Conversion
