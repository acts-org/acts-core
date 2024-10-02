// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/EventData/detail/CorrectedTransformationFreeToBound.hpp"
#include "Acts/Surfaces/Surface.hpp"

#include <tuple>
#include <type_traits>

namespace Acts::detail {

template <typename stepper_t>
struct stepper_bound_parameters {
  using result_type = decltype(std::declval<stepper_t>().boundState(
      std::declval<typename stepper_t::State&>(), std::declval<Acts::Surface>(),
      std::declval<bool>(), std::declval<Acts::FreeToBoundCorrection>()));
  using type =
      std::decay_t<decltype(std::get<0>(*std::declval<result_type>()))>;
};

template <typename stepper_t>
using stepper_bound_parameters_type_t =
    typename stepper_bound_parameters<stepper_t>::type;

template <typename stepper_t>
struct stepper_curvilinear_parameters {
  using result_type = decltype(std::declval<stepper_t>().curvilinearState(
      std::declval<typename stepper_t::State&>(), std::declval<bool>()));
  using type = std::decay_t<decltype(std::get<0>(std::declval<result_type>()))>;
};

template <typename stepper_t>
using stepper_curvilinear_parameters_type_t =
    typename stepper_curvilinear_parameters<stepper_t>::type;
}  // namespace Acts::detail
