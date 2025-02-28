// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/NavigationPolicyFactory.hpp"

#include <memory>

namespace Acts::Experimental {

struct BlueprintOptions {
  std::shared_ptr<NavigationPolicyFactory> defaultNavigationPolicyFactory{
      makeDefaultNavigationPolicyFactory()};

  void validate() const;

 private:
  static std::unique_ptr<NavigationPolicyFactory>
  makeDefaultNavigationPolicyFactory();
};

}  // namespace Acts::Experimental
