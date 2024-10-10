// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/EventData/Seed.hpp"
#include "ActsExamples/EventData/SimSpacePoint.hpp"

#include <vector>

namespace ActsExamples {
using SimSeed = Acts::Seed<SimSpacePoint>;
/// Container of sim seed
using SimSeedContainer = std::vector<SimSeed>;

}  // namespace ActsExamples
