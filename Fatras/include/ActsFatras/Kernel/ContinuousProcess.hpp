// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Material/MaterialSlab.hpp"
#include "ActsFatras/EventData/Particle.hpp"

namespace ActsFatras {

/// A continuous simulation process based on a physics model plus selectors.
///
/// @tparam physics_t is the physics model type
/// @tparam input_particle_selector_t is the input particle selector
/// @tparam output_particle_selector_t is the output particle selector
/// @tparam child_particle_selector_t is the child particle selector
///
/// The physics model type **must** provide a call operator with the following
/// signature
///
///     <Particle Container>
///     operator()(
///         generator_t& generator,
///         const Acts::MaterialSlab& slab,
///         Particle& particle) const
///
/// The return type can be any `Container` with `Particle` elements.
///
/// The input selector defines whether the process is applied while the
/// output selector defines a break condition, i.e. whether to continue
/// simulating the particle propagation. The child selector is used to
/// filter the generated child particles.
///
/// @note The output and child particle selectors are identical unless the
///       child particle selector is explicitly specified.
template <typename physics_t, typename input_particle_selector_t,
          typename output_particle_selector_t,
          typename child_particle_selector_t = output_particle_selector_t>
struct ContinuousProcess {
  /// The physics interactions implementation.
  physics_t physics;
  /// Input selection: if this process applies to this particle.
  input_particle_selector_t selectInputParticle;
  /// Output selection: if the particle is still valid after the interaction.
  output_particle_selector_t selectOutputParticle;
  /// Child selection: if a generated child particle should be kept.
  child_particle_selector_t selectChildParticle;

  /// Execute the physics process considering the configured selectors.
  ///
  /// @param[in]     generator is the random number generator
  /// @param[in]     slab      is the passed material
  /// @param[in,out] particle  is the particle being updated
  /// @param[out]    generated is the container of generated particles
  /// @return Break condition, i.e. whether this process stops the propagation
  ///
  /// @tparam generator_t must be a RandomNumberEngine
  template <typename generator_t>
  bool operator()(generator_t &generator, const Acts::MaterialSlab &slab,
                  Particle &particle, std::vector<Particle> &generated) const {
    // not selecting this process is not a break condition
    if (!selectInputParticle(particle)) {
      return false;
    }
    // modify particle according to the physics process
    auto children = physics(generator, slab, particle);
    // move selected child particles to the output container
    std::copy_if(std::begin(children), std::end(children),
                 std::back_inserter(generated), selectChildParticle);
    // break condition is defined by whether the output particle is still valid
    // or not e.g. because it has fallen below a momentum threshold.
    return !selectOutputParticle(particle);
  }
};

}  // namespace ActsFatras
