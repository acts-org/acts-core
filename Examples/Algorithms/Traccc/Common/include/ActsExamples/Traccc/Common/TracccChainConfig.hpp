// This file is part of the Acts project.
//
// Copyright (C) 2023 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

// Traccc include(s)
#include "traccc/ambiguity_resolution/greedy_ambiguity_resolution_algorithm.hpp"
#include "traccc/clusterization/clusterization_algorithm.hpp"
#include "traccc/clusterization/spacepoint_formation_algorithm.hpp"
#include "traccc/definitions/primitives.hpp"
#include "traccc/finding/finding_algorithm.hpp"
#include "traccc/finding/finding_config.hpp"
#include "traccc/fitting/fitting_algorithm.hpp"
#include "traccc/fitting/fitting_config.hpp"
#include "traccc/seeding/seeding_algorithm.hpp"
#include "traccc/seeding/track_params_estimation.hpp"

namespace ActsExamples::Traccc::Common {

struct TracccChainConfig {
  using ScalarType = traccc::scalar;

  using SeedfinderConfigType = typename traccc::seedfinder_config;
  using SpacepointGridConfigType = typename traccc::spacepoint_grid_config;
  using SeedfilterConfigType = typename traccc::seedfilter_config;
  using FindingConfigType = typename traccc::finding_config<ScalarType>;
  using FittingConfigType = typename traccc::fitting_config;
  using AmbiguityResolutionConfigType =
      typename traccc::greedy_ambiguity_resolution_algorithm::config_t;

  SeedfinderConfigType seedfinderConfig;
  SpacepointGridConfigType spacepointGridConfig{seedfinderConfig};
  SeedfilterConfigType seedfilterConfig;
  FindingConfigType findingConfig;
  FittingConfigType fittingConfig;
  AmbiguityResolutionConfigType ambiguityResolutionConfig;
};

}  // namespace ActsExamples::Traccc::Common
