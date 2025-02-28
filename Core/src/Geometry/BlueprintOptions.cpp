// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/BlueprintOptions.hpp"

#include "Acts/Geometry/NavigationPolicyFactory.hpp"
#include "Acts/Navigation/TryAllNavigationPolicy.hpp"

namespace Acts::Experimental {

void BlueprintOptions::validate() const {
  if (!defaultNavigationPolicyFactory) {
    throw std::invalid_argument("Navigation policy factory is nullptr");
  }
}

std::unique_ptr<NavigationPolicyFactory>
BlueprintOptions::makeDefaultNavigationPolicyFactory() {
  return NavigationPolicyFactory::make()
      .add<TryAllNavigationPolicy>()
      .asUniquePtr();
}

}  // namespace Acts::Experimental
