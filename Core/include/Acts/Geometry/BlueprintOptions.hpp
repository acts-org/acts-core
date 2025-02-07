// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Geometry/NavigationPolicyFactory.hpp"

#include <memory>

namespace Acts {

struct BlueprintOptions {
  std::shared_ptr<NavigationPolicyFactory> defaultNavigationPolicyFactory{
      makeDefaultNavigationPolicyFactory()};

  void validate() const;

 private:
  static std::unique_ptr<NavigationPolicyFactory>
  makeDefaultNavigationPolicyFactory();
};

}  // namespace Acts
