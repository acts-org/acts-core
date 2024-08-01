// This file is part of the Acts project.
//
// Copyright (C) 2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ActsExamples/Traccc/Common/TracccChainConfig.hpp"

#include "Acts/Plugins/Python/Utilities.hpp"
#include "Acts/Utilities/TypeTraits.hpp"
#include "ActsExamples/EventData/Index.hpp"

#include <memory>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

namespace Acts::Python {

void addSeedFinderConfig(pybind11::module_ m) {
  using Config = typename ActsExamples::Traccc::Common::TracccChainConfig::
      SeedfinderConfigType;

  auto c = py::class_<Config>(m, "SeedFinderConfig").def(py::init<>());

  ACTS_PYTHON_STRUCT_BEGIN(c, Config);
  ACTS_PYTHON_MEMBER(zMin);
  ACTS_PYTHON_MEMBER(zMax);
  ACTS_PYTHON_MEMBER(rMax);
  ACTS_PYTHON_MEMBER(rMin);
  ACTS_PYTHON_MEMBER(collisionRegionMin);
  ACTS_PYTHON_MEMBER(collisionRegionMax);
  ACTS_PYTHON_MEMBER(phiMin);
  ACTS_PYTHON_MEMBER(phiMax);
  ACTS_PYTHON_MEMBER(minPt);
  ACTS_PYTHON_MEMBER(cotThetaMax);
  ACTS_PYTHON_MEMBER(deltaRMin);
  ACTS_PYTHON_MEMBER(deltaRMax);
  ACTS_PYTHON_MEMBER(impactMax);
  ACTS_PYTHON_MEMBER(sigmaScattering);
  ACTS_PYTHON_MEMBER(maxPtScattering);
  ACTS_PYTHON_MEMBER(maxSeedsPerSpM);
  ACTS_PYTHON_MEMBER(bFieldInZ);
  ACTS_PYTHON_MEMBER(beamPos);
  ACTS_PYTHON_MEMBER(radLengthPerSeed);
  ACTS_PYTHON_MEMBER(zAlign);
  ACTS_PYTHON_MEMBER(rAlign);
  ACTS_PYTHON_MEMBER(sigmaError);
  ACTS_PYTHON_MEMBER(highland);
  ACTS_PYTHON_MEMBER(maxScatteringAngle2);
  ACTS_PYTHON_MEMBER(pTPerHelixRadius);
  ACTS_PYTHON_MEMBER(minHelixDiameter2);
  ACTS_PYTHON_MEMBER(pT2perRadius);
  ACTS_PYTHON_MEMBER(phiBinDeflectionCoverage);
  ACTS_PYTHON_MEMBER(neighbor_scope);
  ACTS_PYTHON_STRUCT_END();
}

void addSpacePointGridConfig(py::module_ m) {
  using Config = typename ActsExamples::Traccc::Common::TracccChainConfig::
      SpacepointGridConfigType;

  auto c = py::class_<Config>(m, "SpacePointGridConfig")
               .def(py::init<const typename ActsExamples::Traccc::Common::
                                 TracccChainConfig::SeedfinderConfigType&>());

  ACTS_PYTHON_STRUCT_BEGIN(c, Config);
  ACTS_PYTHON_MEMBER(bFieldInZ);
  ACTS_PYTHON_MEMBER(minPt);
  ACTS_PYTHON_MEMBER(rMax);
  ACTS_PYTHON_MEMBER(zMax);
  ACTS_PYTHON_MEMBER(zMin);
  ACTS_PYTHON_MEMBER(deltaRMax);
  ACTS_PYTHON_MEMBER(cotThetaMax);
  ACTS_PYTHON_MEMBER(impactMax);
  ACTS_PYTHON_MEMBER(phiMin);
  ACTS_PYTHON_MEMBER(phiMax);
  ACTS_PYTHON_MEMBER(phiBinDeflectionCoverage);
  ACTS_PYTHON_STRUCT_END();
}

void addSeedFilterConfig(py::module_ m) {
  using Config = typename ActsExamples::Traccc::Common::TracccChainConfig::
      SeedfilterConfigType;

  auto c = py::class_<Config>(m, "SeedFilterConfig").def(py::init<>());

  ACTS_PYTHON_STRUCT_BEGIN(c, Config);
  ACTS_PYTHON_MEMBER(deltaInvHelixDiameter);
  ACTS_PYTHON_MEMBER(impactWeightFactor);
  ACTS_PYTHON_MEMBER(compatSeedWeight);
  ACTS_PYTHON_MEMBER(deltaRMin);
  ACTS_PYTHON_MEMBER(maxSeedsPerSpM);
  ACTS_PYTHON_MEMBER(compatSeedLimit);
  ACTS_PYTHON_MEMBER(max_triplets_per_spM);
  ACTS_PYTHON_MEMBER(good_spB_min_radius);
  ACTS_PYTHON_MEMBER(good_spB_weight_increase);
  ACTS_PYTHON_MEMBER(good_spT_max_radius);
  ACTS_PYTHON_MEMBER(good_spT_weight_increase);
  ACTS_PYTHON_MEMBER(good_spB_min_weight);
  ACTS_PYTHON_MEMBER(seed_min_weight);
  ACTS_PYTHON_MEMBER(spB_min_radius);
  ACTS_PYTHON_STRUCT_END();
}

void addFindingConfig(py::module_ m) {
  using Config = typename ActsExamples::Traccc::Common::TracccChainConfig::
      FindingConfigType;

  auto c = py::class_<Config>(m, "FindingConfig").def(py::init<>());

  ACTS_PYTHON_STRUCT_BEGIN(c, Config);
  ACTS_PYTHON_MEMBER(max_num_branches_per_seed);
  ACTS_PYTHON_MEMBER(max_num_branches_per_surface);
  ACTS_PYTHON_MEMBER(min_track_candidates_per_track);
  ACTS_PYTHON_MEMBER(max_track_candidates_per_track);
  ACTS_PYTHON_MEMBER(max_num_skipping_per_cand);
  ACTS_PYTHON_MEMBER(min_step_length_for_next_surface);
  ACTS_PYTHON_MEMBER(max_step_counts_for_next_surface);
  ACTS_PYTHON_MEMBER(chi2_max);
  ACTS_PYTHON_MEMBER(propagation);
  ACTS_PYTHON_MEMBER(n_measurements_per_thread);
  ACTS_PYTHON_MEMBER(navigation_buffer_size_scaler);
  ACTS_PYTHON_STRUCT_END();
}

void addFittingConfig(py::module_ m) {
  using Config = typename ActsExamples::Traccc::Common::TracccChainConfig::
      FittingConfigType;

  auto c = py::class_<Config>(m, "FittingConfig").def(py::init<>());

  ACTS_PYTHON_STRUCT_BEGIN(c, Config);
  ACTS_PYTHON_MEMBER(n_iterations);
  ACTS_PYTHON_MEMBER(propagation);
  ACTS_PYTHON_STRUCT_END();
}

void addGreedyAmbiguityResolutionAlgorithmConfig(py::module_ m) {
  using Config = typename ActsExamples::Traccc::Common::TracccChainConfig::
      AmbiguityResolutionConfigType;

  auto c = py::class_<Config>(m, "GreedyAmbiguityResolutionAlgorithmConfig")
               .def(py::init<>());

  ACTS_PYTHON_STRUCT_BEGIN(c, Config);
  ACTS_PYTHON_MEMBER(maximum_shared_hits);
  ACTS_PYTHON_MEMBER(maximum_iterations);
  ACTS_PYTHON_MEMBER(n_measurements_min);
  ACTS_PYTHON_MEMBER(check_obvious_errs);
  ACTS_PYTHON_MEMBER(measurement_id_0_warning_threshold);
  ACTS_PYTHON_MEMBER(verbose_error);
  ACTS_PYTHON_MEMBER(verbose_warning);
  ACTS_PYTHON_MEMBER(verbose_info);
  ACTS_PYTHON_MEMBER(verbose_debug);
  ACTS_PYTHON_STRUCT_END();
}

void addTracccChainConfig(Context& ctx) {
  auto m = ctx.get("examples");

  addSeedFinderConfig(m);
  addSpacePointGridConfig(m);
  addSeedFilterConfig(m);
  addFindingConfig(m);
  addFittingConfig(m);
  addGreedyAmbiguityResolutionAlgorithmConfig(m);

  using Config = typename ActsExamples::Traccc::Common::TracccChainConfig;

  auto c = py::class_<Config, std::shared_ptr<Config>>(m, "TracccChainConfig")
               .def(py::init<>());

  ACTS_PYTHON_STRUCT_BEGIN(c, Config);
  ACTS_PYTHON_MEMBER(seedfinderConfig);
  ACTS_PYTHON_MEMBER(spacepointGridConfig);
  ACTS_PYTHON_MEMBER(seedfilterConfig);
  ACTS_PYTHON_MEMBER(findingConfig);
  ACTS_PYTHON_MEMBER(fittingConfig);
  ACTS_PYTHON_MEMBER(ambiguityResolutionConfig);
  ACTS_PYTHON_STRUCT_END();
}

}  // namespace Acts::Python
