// This file is part of the Acts project.
//
// Copyright (C) 2022 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Definitions/Common.hpp"

#include <ostream>

namespace Acts {

std::ostream& operator<<(std::ostream& os, NavigationDirection navDir) {
  switch (navDir) {
    case NavigationDirection::Forward:
      os << "forward";
      break;
    case NavigationDirection::Backward:
      os << "backward";
      break;
    default:
      assert(false && "Invalid direction (shouldn't happen)");
      std::abort();
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, MaterialUpdateStage matUpdate) {
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

}  // namespace Acts
