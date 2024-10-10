// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Direction.hpp"
#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Propagator/ActorList.hpp"
#include "Acts/Propagator/NavigatorOptions.hpp"
#include "Acts/Propagator/StepperOptions.hpp"

#include <limits>

namespace Acts {

namespace detail {

/// @brief Holds the generic pure propagator options
struct PurePropagatorPlainOptions {
  /// Propagation direction
  Direction direction = Direction::Forward;

  /// Maximum number of steps for one propagate call
  unsigned int maxSteps = 1000;

  /// Absolute maximum path length
  double pathLimit = std::numeric_limits<double>::max();

  /// Loop protection step, it adapts the pathLimit
  bool loopProtection = true;
  /// Allowed loop fraction, 1 is a full loop
  double loopFraction = 0.5;

  /// Required tolerance to reach surface
  double surfaceTolerance = s_onSurfaceTolerance;

  /// Constrain the propagation to selected volumes
  /// @note ignored if empty
  /// @note requires `VolumeConstraintAborter` aborter
  std::vector<std::uint32_t> constrainToVolumeIds;
  /// Additional volumes to be considered as end of world
  /// @note ignored if empty
  /// @note requires `VolumeConstraintAborter` aborter
  std::vector<std::uint32_t> endOfWorldVolumeIds;
};

}  // namespace detail

/// @brief Holds the generic propagator options
struct PropagatorPlainOptions : public detail::PurePropagatorPlainOptions {
  /// PropagatorPlainOptions with context
  PropagatorPlainOptions(const GeometryContext& gctx,
                         const MagneticFieldContext& mctx)
      : geoContext(gctx), magFieldContext(mctx) {}

  /// The context object for the geometry
  std::reference_wrapper<const GeometryContext> geoContext;

  /// The context object for the magnetic field
  std::reference_wrapper<const MagneticFieldContext> magFieldContext;

  /// Stepper plain options
  StepperPlainOptions stepping;

  /// Navigator plain options
  NavigatorPlainOptions navigation;
};

/// @brief Options for propagate() call
///
/// @tparam actor_list_t List of action types called after each
///    propagation step with the current propagation and stepper state
///
template <typename stepper_options_t, typename navigator_options_t,
          typename actor_list_t = ActorList<>>
struct PropagatorOptions : public detail::PurePropagatorPlainOptions {
  using stepper_options_type = stepper_options_t;
  using navigator_options_type = navigator_options_t;
  using actor_list_type = actor_list_t;

  /// PropagatorOptions with context
  PropagatorOptions(const GeometryContext& gctx,
                    const MagneticFieldContext& mctx)
      : geoContext(gctx), magFieldContext(mctx) {}

  /// PropagatorOptions with context and plain options
  PropagatorOptions(const PropagatorPlainOptions& pOptions)
      : geoContext(pOptions.geoContext),
        magFieldContext(pOptions.magFieldContext) {
    setPlainOptions(pOptions);
  }

  /// @brief Convert to plain options
  operator PropagatorPlainOptions() const {
    PropagatorPlainOptions pOptions(geoContext, magFieldContext);
    static_cast<PurePropagatorPlainOptions&>(pOptions) =
        static_cast<const PurePropagatorPlainOptions&>(*this);
    pOptions.stepping = static_cast<const StepperPlainOptions&>(stepping);
    pOptions.navigation = static_cast<const NavigatorPlainOptions&>(navigation);
    return pOptions;
  }

  /// @brief Expand the options with extended actors
  ///
  /// @tparam extended_actor_list_t Type of the new actor list
  ///
  /// @param extendedActorList The new actor list to be used (internally)
  template <typename extended_actor_list_t>
  PropagatorOptions<stepper_options_t, navigator_options_t,
                    extended_actor_list_t>
  extend(extended_actor_list_t extendedActorList) const {
    PropagatorOptions<stepper_options_t, navigator_options_t,
                      extended_actor_list_t>
        eoptions(geoContext, magFieldContext);

    // Copy the base options
    static_cast<PurePropagatorPlainOptions&>(eoptions) =
        static_cast<const PurePropagatorPlainOptions&>(*this);

    // Stepper / Navigator options
    eoptions.stepping = stepping;
    eoptions.navigation = navigation;

    // Action / Abort list
    eoptions.actorList = extendedActorList;

    // And return the options
    return eoptions;
  }

  /// @brief Set the plain options
  ///
  /// @param pOptions The plain options
  void setPlainOptions(const PropagatorPlainOptions& pOptions) {
    static_cast<PurePropagatorPlainOptions&>(*this) =
        static_cast<const PurePropagatorPlainOptions&>(pOptions);

    stepping.setPlainOptions(pOptions.stepping);
    navigation.setPlainOptions(pOptions.navigation);
  }

  /// The context object for the geometry
  std::reference_wrapper<const GeometryContext> geoContext;

  /// The context object for the magnetic field
  std::reference_wrapper<const MagneticFieldContext> magFieldContext;

  /// Stepper options
  stepper_options_t stepping;

  /// Navigator options
  navigator_options_t navigation;

  /// List of actions
  actor_list_t actorList;
};

}  // namespace Acts
