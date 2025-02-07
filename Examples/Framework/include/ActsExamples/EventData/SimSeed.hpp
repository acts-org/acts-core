// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/EventData/Seed.hpp"
#include "ActsExamples/EventData/SimSpacePoint.hpp"

#include <vector>

namespace ActsExamples {
using SimSeed = Acts::Seed<SimSpacePoint>;
/// Container of sim seed
using SimSeedContainer = std::vector<SimSeed>;

}  // namespace ActsExamples
