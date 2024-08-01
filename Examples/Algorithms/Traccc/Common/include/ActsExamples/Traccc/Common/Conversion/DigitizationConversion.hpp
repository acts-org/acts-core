// This file is part of the Acts project.
//
// Copyright (C) 2023 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

// Acts include(s)
#include "Acts/Geometry/GeometryHierarchyMap.hpp"

// Acts Examples include(s)
#include "ActsExamples/Digitization/DigitizationConfig.hpp"

// Traccc include(s)
#include "traccc/io/digitization_config.hpp"

namespace ActsExamples::Traccc::Common::Conversion {

/// @brief Creates a traccc digitalization config from an Acts geometry hierarchy map
/// that contains the digitization configuration.
/// @param config the Acts geometry hierarchy map that contains the digitization configuration.
/// @return a traccc digitization config.
traccc::digitization_config tracccConfig(
    const Acts::GeometryHierarchyMap<DigiComponentsConfig>& config);

}  // namespace ActsExamples::Traccc::Common::Conversion
