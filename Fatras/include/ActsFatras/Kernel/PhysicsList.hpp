// This file is part of the Acts project.
//
// Copyright (C) 2018-2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Material/MaterialSlab.hpp"
#include "ActsFatras/EventData/Particle.hpp"

#include <bitset>
#include <tuple>

namespace ActsFatras {

/// Combined set of physics processes and interactions for the simulation.
///
/// The physics processes are extendable by the user to be able to accomodate
/// the specific requirements. While the set of available physics processes must
/// be configured at compile-time, within that set processes can be selectively
/// disabled at run-time. By default all processes are applied.
template <typename... processes_t>
class PhysicsList {
 public:
  /// Disable a specific process by type.
  template <typename process_t>
  void disable() {
    m_mask.set(Index<process_t, Processes>::value);
  }

  /// Access a specific process by type.
  template <typename process_t>
  process_t& get() {
    return std::get<process_t>(m_processes);
  }

  /// Run the physics list for a given material and particle.
  ///
  /// @tparam generator_t must be a RandomNumberEngine
  /// @param[in]     generator is the random number generator
  /// @param[in]     slab      is the passed material
  /// @param[in,out] particle  is the particle being updated
  /// @param[out]    generated is the container of generated particles
  /// @return Break condition, i.e. whether a process stoped the propagation
  template <typename generator_t>
  bool run(generator_t& generator, const Acts::MaterialSlab& slab,
           Particle& particle, std::vector<Particle>& generated) const {
    static_assert(
        (true && ... &&
         std::is_same_v<bool, decltype(processes_t()(generator, slab, particle,
                                                     generated))>),
        "Not all processes conform to the expected interface");

    return runImpl(generator, slab, particle, generated,
                   std::index_sequence_for<processes_t...>());
  }

 private:
  // TODO check that all processes are unique types.

  // utility struct to retrieve index of the first matching type in the tuple.
  // from https://stackoverflow.com/a/18063608.
  template <class T, class Tuple>
  struct Index;
  template <class T, class... Types>
  struct Index<T, std::tuple<T, Types...>> {
    static constexpr std::size_t value = 0u;
  };
  template <class T, class U, class... Types>
  struct Index<T, std::tuple<U, Types...>> {
    static constexpr std::size_t value =
        1u + Index<T, std::tuple<Types...>>::value;
  };

  using Mask = std::bitset<sizeof...(processes_t)>;
  using Processes = std::tuple<processes_t...>;

  // allow processes to be masked. defaults to zeros -> no masked processes
  Mask m_mask;
  Processes m_processes;

  // compile-time index-based recursive function call for all processes
  template <typename generator_t, std::size_t I0, std::size_t... INs>
  bool runImpl(generator_t& generator, const Acts::MaterialSlab& slab,
               Particle& particle, std::vector<Particle>& generated,
               std::index_sequence<I0, INs...>) const {
    // only call process if it is not masked
    if (not m_mask[I0] and
        std::get<I0>(m_processes)(generator, slab, particle, generated)) {
      // exit early in case the process signals an abort
      return true;
    }
    return runImpl(generator, slab, particle, generated,
                   std::index_sequence<INs...>());
  }
  template <typename generator_t>
  bool runImpl(generator_t&, const Acts::MaterialSlab&, Particle&,
               std::vector<Particle>&, std::index_sequence<>) const {
    return false;
  }
};

}  // namespace ActsFatras
