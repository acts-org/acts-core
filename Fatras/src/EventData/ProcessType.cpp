// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsFatras/EventData/ProcessType.hpp"

#include <ostream>

namespace ActsFatras {

std::ostream &operator<<(std::ostream &os, ProcessType processType) {
  switch (processType) {
    case ProcessType::eUndefined:
      return (os << "undefined");
    default:
      return (os << static_cast<std::uint32_t>(processType));
  }
}

}  // namespace ActsFatras
