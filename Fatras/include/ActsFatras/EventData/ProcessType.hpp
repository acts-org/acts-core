// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <cstdint>
#include <iosfwd>

namespace ActsFatras {

/// Process type identifier.
///
/// Encodes the type of process that generated a particle.
enum class ProcessType : std::uint32_t {
  eUndefined = 0,
  eDecay = 1,
  ePhotonConversion = 2,
  eBremsstrahlung = 3,
  eNuclearInteraction = 4,
};

std::ostream &operator<<(std::ostream &os, ProcessType processType);

}  // namespace ActsFatras
