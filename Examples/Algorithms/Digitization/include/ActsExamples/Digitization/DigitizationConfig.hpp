// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Geometry/GeometryHierarchyMap.hpp"
#include "ActsExamples/Digitization/GeometricConfig.hpp"
#include "ActsExamples/Digitization/SmearingConfig.hpp"

namespace ActsExamples {

/// Configuration struct for the Digitization algorithm
///
/// It contains:
/// - optional GeometricConfig
/// - optional SmearingConfig
struct DigiComponentsConfig {
  GeometricConfig geometricDigiConfig;
  SmearingConfig smearingDigiConfig;
};

using DigiConfigContainer = Acts::GeometryHierarchyMap<DigiComponentsConfig>;

}  // namespace ActsExamples
