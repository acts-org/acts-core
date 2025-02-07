// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/EventData/GenericBoundTrackParameters.hpp"
#include "Acts/EventData/TrackParametersConcept.hpp"

namespace Acts::detail {

/// @brief Shorthand for Bound or Free track parameters
template <class parameters_t>
concept isBoundOrFreeTrackParams =
    Acts::FreeTrackParametersConcept<parameters_t> ||
    Acts::BoundTrackParametersConcept<parameters_t>;

/// @brief Shorthand for GenericBoundTrackParameters
template <class parameters_t>
concept isGenericBoundTrackParams =
    std::same_as<parameters_t, Acts::GenericBoundTrackParameters<
                                   typename parameters_t::ParticleHypothesis>>;

/// @brief Concept that restricts the type of the
/// accumulation grid cell
template <typename grid_t>
concept TrackParamsGrid = requires {
  typename grid_t::value_type::first_type;
  typename grid_t::value_type::second_type;

  requires isBoundOrFreeTrackParams<
      typename grid_t::value_type::first_type::element_type>;
  requires isBoundOrFreeTrackParams<
      typename grid_t::value_type::second_type::element_type>;

  requires requires(typename grid_t::value_type val) {
    {
      val.first
    } -> std::same_as<
          std::shared_ptr<typename decltype(val.first)::element_type>&>;
    { val.second } -> std::same_as<decltype(val.first)&>;
  };
};

}  // namespace Acts::detail
