// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Geometry/BlueprintOptions.hpp"

#include "Acts/Geometry/NavigationPolicyFactory.hpp"
#include "Acts/Navigation/TryAllNavigationPolicy.hpp"

namespace Acts {

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

}  // namespace Acts
