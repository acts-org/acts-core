// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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
