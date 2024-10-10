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

#include <bitset>
#include <tuple>
#include <type_traits>
#include <utility>

namespace ActsFatras {
namespace detail {

/// Retrieve index of the first matching type in the tuple.
///
/// Taken from https://stackoverflow.com/a/18063608.
template <class T, class Tuple>
struct TupleIndexOf;
template <class T, class... Types>
struct TupleIndexOf<T, std::tuple<T, Types...>> {
  static constexpr std::size_t value = 0u;
};
template <class T, class U, class... Types>
struct TupleIndexOf<T, std::tuple<U, Types...>> {
  static constexpr std::size_t value =
      1u + TupleIndexOf<T, std::tuple<Types...>>::value;
};

// Construct an index sequence for a subset of the tuple elements.
//
// Whether an element is part of the subset is defined by the predicate
// template type. It must take the element type as its only template parameter
// and must provide a static `value` member value. If the value evaluates to
// `true`, then the corresponding index will be part of the index sequence.
//
// Example: The tuple contains four elements, where all but the third one (i=2)
// should be selected. This leads to the following recursive expansion
// where the index sequence of the subset is filled from the front.
//
//        TupleFilterImpl<..., kCounter=4>          // select index=3
//     -> TupleFilterImpl<..., kCounter=3, 3>       // skip   index=2
//     -> TupleFilterImpl<..., kCounter=2, 3>       // select index=1
//     -> TupleFilterImpl<..., kCounter=1, 1, 3>    // select index=0
//     -> TupleFilterImpl<..., kCounter=0, 0, 1, 3> // terminate
//
template <template <typename> typename predicate_t, typename tuple_t,
          std::size_t kCounter, std::size_t... kIndices>
struct TupleFilterImpl {
  static constexpr auto kIndex = kCounter - 1u;
  static constexpr bool kElementSelection =
      predicate_t<std::tuple_element_t<kIndex, tuple_t>>::value;
  // recursive type if the element would be selected
  using SelectElement = typename TupleFilterImpl<predicate_t, tuple_t, kIndex,
                                                 kIndex, kIndices...>::Type;
  // recursive type if the element would be skipped
  using SkipElement =
      typename TupleFilterImpl<predicate_t, tuple_t, kIndex, kIndices...>::Type;
  // select recursive type based on the selector decision
  using Type =
      std::conditional_t<kElementSelection, SelectElement, SkipElement>;
};
template <template <typename> typename predicate_t, typename tuple_t,
          std::size_t... kIndices>
struct TupleFilterImpl<predicate_t, tuple_t, 0u, kIndices...> {
  using Type = std::index_sequence<kIndices...>;
};
template <template <typename> typename predicate_t, typename tuple_t>
using TupleFilter = typename TupleFilterImpl<predicate_t, tuple_t,
                                             std::tuple_size_v<tuple_t>>::Type;

/// Check if the given type is a point-like process.
///
/// Only checks for the existence of the templated `generatePathLimits` method
template <typename process_t>
concept PointLikeProcessConcept = requires(
    const process_t& p, std::uniform_int_distribution<unsigned int>& rng,
    const Particle& prt) {
  {
    p.generatePathLimits(rng, prt)
  } -> std::same_as<std::pair<Particle::Scalar, Particle::Scalar>>;
};

template <typename process_t>
concept ContinuousProcessConcept = !PointLikeProcessConcept<process_t>;

template <typename process_t>
struct PointLikeProcessTrait {
  static constexpr bool value = PointLikeProcessConcept<process_t>;
};

template <typename process_t>
struct ContinuousProcessTrait {
  static constexpr bool value = ContinuousProcessConcept<process_t>;
};

template <typename processes_t>
using ContinuousIndices = TupleFilter<ContinuousProcessTrait, processes_t>;
template <typename processes_t>
using PointLikeIndices = TupleFilter<PointLikeProcessTrait, processes_t>;

}  // namespace detail

/// Compile-time set of interaction processes for the simulation.
///
/// Two different type of interaction processes are supported: continuous and
/// point-like interactions.
///
/// Continuous processes scale with the passed material. They tpyically
/// describe effective results of a large number of small interactions such as
/// multiple scattering or ionisation. Continuous process types **must** provide
/// a call operator with the following signature:
///
///     template <typename generator_t>
///     bool
///     operator()(
///         generator_t& rng,
///         const Acts::MaterialSlab& slab,
///         Particle& particle,
///         std::vector<Particle>& generatedParticles) const
///
/// If multiple continuous processes are defined, they are executed serially in
/// the order in which they are given.
///
/// For point-like processes, the passed material only affects the probability
/// with which they occur but not the interaction itself, e.g. photon conversion
/// into electron pairs. They are simulated by first drawing a limit on the
/// material paths and then executing the interaction with the shortest limit
/// when the drawn amount of material has been passed. Point-like process
/// types **must** provide the following two member functions:
///
///     // generate X0/L0 limits
///     template <typename generator_t>
///     std::pair<Scalar, Scalar>
///     generatePathLimits(
///         generator& rng,
///         const Particle& particle) const
///
///     // run the process simulation
///     template <typename generator_t>
///     bool
///     run(
///         generator_t& rng,
///         Particle& particle,
///         std::vector<Particle>& generatedParticles) const
///
/// For both continuous and point-like interactions, the output particle is
/// modified in-place (if needed) and the return value indicates a break
/// condition in the simulation, i.e. the particle is dead (true) or alive
/// (false) after the interaction.
///
/// @note If an interaction destroys the incoming particle, the process
///   simulation should indicate this via the break condition only and not
///   by reducing the particle momentum to zero. The incoming particle should
///   retain its initial kinematic state; the final kinematic state before
///   destruction is typically of more interest to the user and this simplifies
///   validation.
///
/// The physics processes are extendable by the user to accommodate their
/// specific requirements. While the set of available physics processes must be
/// configured at compile-time, within that set, processes can again be
/// selectively disabled at run-time. By default all processes are applied.
template <typename... processes_t>
class InteractionList {
  using Mask = std::bitset<sizeof...(processes_t)>;
  using Processes = std::tuple<processes_t...>;
  using ContinuousIndices = detail::ContinuousIndices<Processes>;
  using PointLikeIndices = detail::PointLikeIndices<Processes>;

 public:
  /// Point-like interaction selection.
  struct Selection {
    Particle::Scalar x0Limit =
        std::numeric_limits<Particle::Scalar>::infinity();
    Particle::Scalar l0Limit =
        std::numeric_limits<Particle::Scalar>::infinity();
    std::size_t x0Process = std::numeric_limits<std::size_t>::max();
    std::size_t l0Process = std::numeric_limits<std::size_t>::max();
  };

  /// Disable a specific process identified by index.
  void disable(std::size_t process) { m_mask.set(process); }
  /// Disable a specific process identified by type.
  ///
  /// @note Disables only the first element, if multiple elements of the same
  ///   type exist.
  template <typename process_t>
  void disable() {
    m_mask.set(detail::TupleIndexOf<process_t, Processes>::value);
  }

  /// Access a specific process identified by index.
  template <std::size_t kProcess>
  std::tuple_element_t<kProcess, Processes>& get() {
    return std::get<kProcess>(m_processes);
  }
  /// Access a specific process identified by type.
  ///
  /// @warning This function only works if all configured processes have
  ///   different types.
  template <typename process_t>
  process_t& get() {
    return std::get<process_t>(m_processes);
  }

  /// Simulate the combined effects from all continuous interactions.
  ///
  /// @tparam generator_t must be a RandomNumberEngine
  /// @param[in]     rng       is the random number generator
  /// @param[in]     slab      is the passed material
  /// @param[in,out] particle  is the particle being updated
  /// @param[out]    generated is the container of generated particles
  /// @return Break condition, i.e. whether a process stopped the propagation
  template <typename generator_t>
  bool runContinuous(generator_t& rng, const Acts::MaterialSlab& slab,
                     Particle& particle,
                     std::vector<Particle>& generated) const {
    return runContinuousImpl(rng, slab, particle, generated,
                             ContinuousIndices());
  }

  /// Arm the point-like interactions by generating limits and select processes.
  ///
  /// @tparam generator_t must be a RandomNumberEngine
  /// @param[in] rng      is the random number generator
  /// @param[in] particle is the initial particle state
  /// @return X0/L0 limits for the particle and the process index that should be
  ///   executed once the limit has been reached.
  template <typename generator_t>
  Selection armPointLike(generator_t& rng, const Particle& particle) const {
    Selection selection;
    armPointLikeImpl(rng, particle, selection, PointLikeIndices());
    return selection;
  }

  /// Simulate the effects from a single point-like interaction.
  ///
  /// @tparam generator_t must be a RandomNumberEngine
  /// @param[in]     rng          is the random number generator
  /// @param[in]     processIndex is the index of the process to be executed
  /// @param[in,out] particle     is the particle being updated
  /// @param[out]    generated    is the container of generated particles
  /// @return Break condition, i.e. whether a process killed the particle
  ///
  /// The process index is expected to originate from a previous
  /// `armPointLike(...)` call, but this is not enforced. How to select the
  /// correct process requires more information that is not available here.
  template <typename generator_t>
  bool runPointLike(generator_t& rng, std::size_t processIndex,
                    Particle& particle,
                    std::vector<Particle>& generated) const {
    return runPointLikeImpl(rng, processIndex, particle, generated,
                            PointLikeIndices());
  }

 private:
  // allow processes to be masked. defaults to zeros -> no masked processes
  Mask m_mask;
  Processes m_processes;

  // for the `runContinuous` call, we need to iterate over all available
  // processes and apply the ones that implement the continuous process
  // interface. this is done using an index-based compile-time recursive call.
  template <typename generator_t, std::size_t kI0, std::size_t... kIs>
  bool runContinuousImpl(generator_t& rng, const Acts::MaterialSlab& slab,
                         Particle& particle, std::vector<Particle>& generated,
                         std::index_sequence<kI0, kIs...> /*indices*/) const {
    const auto& process = std::get<kI0>(m_processes);
    // only call process if it is not masked
    if (!m_mask[kI0] && process(rng, slab, particle, generated)) {
      // exit early in case the process signals an abort
      return true;
    }
    return runContinuousImpl(rng, slab, particle, generated,
                             std::index_sequence<kIs...>());
  }
  template <typename generator_t>
  bool runContinuousImpl(generator_t& /*rng*/,
                         const Acts::MaterialSlab& /*slab*/,
                         Particle& /*particle*/,
                         std::vector<Particle>& /*generated*/,
                         std::index_sequence<> /*indices*/) const {
    return false;
  }

  // for the `armPointLike` call, we need to iterate over all available
  // processes and select the ones that generate the smallest limits. this is
  // done using an index-based compile-time recursive call.
  template <typename generator_t, std::size_t kI0, std::size_t... kIs>
  void armPointLikeImpl(generator_t& rng, const Particle& particle,
                        Selection& selection,
                        std::index_sequence<kI0, kIs...> /*indices*/) const {
    // only arm the process if it is not masked
    if (!m_mask[kI0]) {
      auto [x0Limit, l0Limit] =
          std::get<kI0>(m_processes).generatePathLimits(rng, particle);
      if (x0Limit < selection.x0Limit) {
        selection.x0Limit = x0Limit;
        selection.x0Process = kI0;
      }
      if (l0Limit < selection.l0Limit) {
        selection.l0Limit = l0Limit;
        selection.l0Process = kI0;
      }
    }
    // continue with the remaining processes
    armPointLikeImpl(rng, particle, selection, std::index_sequence<kIs...>());
  }
  template <typename generator_t>
  void armPointLikeImpl(generator_t& /*rng*/, const Particle& /*particle*/,
                        Selection& /*selection*/,
                        std::index_sequence<> /*indices*/) const {}

  // for the `runPointLike` call we need to call just one process. since we can
  // not select a tuple element with a runtime index, we need to iterate over
  // all processes with a compile-time recursive function until we reach the
  // requested one.
  template <typename generator_t, std::size_t kI0, std::size_t... kIs>
  bool runPointLikeImpl(generator_t& rng, std::size_t processIndex,
                        Particle& particle, std::vector<Particle>& generated,
                        std::index_sequence<kI0, kIs...> /*indices*/) const {
    if (kI0 == processIndex) {
      if (m_mask[kI0]) {
        // the selected process is masked. since nothing is executed the
        // particle continues to be alive; not a break condition.
        return false;
      }
      return std::get<kI0>(m_processes).run(rng, particle, generated);
    }
    // continue the iteration with the remaining processes
    return runPointLikeImpl(rng, processIndex, particle, generated,
                            std::index_sequence<kIs...>());
  }
  template <typename generator_t>
  bool runPointLikeImpl(generator_t& /*rng*/, std::size_t /*processIndex*/,
                        Particle& /*particle*/,
                        std::vector<Particle>& /*generated*/,
                        std::index_sequence<> /*indices*/) const {
    // the requested process index is outside the possible range. **do not**
    // treat this as an error to simplify the case of an empty physics lists or
    // a default process index.
    return false;
  }
};

}  // namespace ActsFatras
