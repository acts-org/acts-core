// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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
