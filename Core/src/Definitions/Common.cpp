// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Definitions/Common.hpp"

#include <cassert>
#include <cstdlib>
#include <ostream>

std::ostream& Acts::operator<<(std::ostream& os,
                               MaterialUpdateStage matUpdate) {
  switch (matUpdate) {
    case MaterialUpdateStage::PreUpdate:
      os << "PreUpdate (-1)";
      break;
    case MaterialUpdateStage::PostUpdate:
      os << "PostUpdate (1)";
      break;
    case MaterialUpdateStage::FullUpdate:
      os << "FullUpdate (0)";
      break;
    default:
      assert(false && "Invalid material update stage (shouldn't happen)");
      std::abort();
  }
  return os;
}
