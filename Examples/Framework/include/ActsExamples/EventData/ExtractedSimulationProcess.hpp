// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "ActsExamples/EventData/SimParticle.hpp"

#include <vector>

namespace ActsExamples {

/// Stores the initial properties of a particle, the properties before the
/// interaction and the particle properties after the interaction
struct ExtractedSimulationProcess {
  SimParticle initial;
  SimParticle before;
  std::vector<SimParticle> after;
};

using ExtractedSimulationProcessContainer =
    std::vector<ExtractedSimulationProcess>;
}  // namespace ActsExamples
