// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Surfaces/Surface.hpp"

namespace Acts {

/// @brief This class performs the Ridders algorithm to estimate the propagation
/// of the covariance to a certain point in space.
///
/// The algorithm is based on the small deviations of the start parameters based
/// on their uncertainty at the beginning of the propgation. This deviation is
/// represented here by a vector of relative deviations of these parameters and
/// fix for all parameters. So, a common choice has to be found that is able to
/// actually fit into the order of magnitude of the uncertainty of each
/// parameter. Using these deviations, the propagation is repeated multiple
/// times and the final covariance matrix at a given target surface is
/// afterwards evaluated by first order derivatives of the final state
/// parameters wrt. the initial parameters. Therefore this evaluation represents
/// a first order approximation of the transport jacobian. Since performing
/// multiple propagations and a numerical evaluation of the covariance requires
/// more time than a single propagation towards a target + a common propagation
/// of the covariance, this class just serves to verify the results of the
/// latter classes.
template <typename propagator_t>
class RiddersPropagator {
  using Jacobian = BoundMatrix;
  using Covariance = BoundSquareMatrix;

  ///
  /// @note The result_type_helper struct and the actor_list_t_result_t are
  /// here to allow a look'n'feel of this class like the Propagator itself
  ///

  /// @brief Helper struct determining the result's type
  ///
  /// @tparam parameters_t Type of final track parameters
  /// @tparam actor_list_t    List of propagation action types
  ///
  /// This helper struct provides type definitions to extract the correct
  /// propagation result type from a given TrackParameter type and an
  /// ActorList.
  ///
  template <typename parameters_t, typename actor_list_t>
  struct result_type_helper {
    /// @brief Propagation result type for an arbitrary list of additional
    ///        propagation results
    ///
    /// @tparam args Parameter pack specifying additional propagation results
    ///
    template <typename... args>
    using this_result_type = PropagatorResult<parameters_t, args...>;

    /// @brief Propagation result type derived from a given action list
    using type = typename actor_list_t::template result_type<this_result_type>;
  };

  /// @brief Short-hand type definition for propagation result derived from
  ///        an action list
  ///
  /// @tparam parameters_t Type of the final track parameters
  /// @tparam actor_list_t List of propagation action types
  ///
  template <typename parameters_t, typename actor_list_t>
  using actor_list_t_result_t =
      typename result_type_helper<parameters_t, actor_list_t>::type;

 public:
  struct Config {
    /// Set of deltas which will be added to the nominal track parameters
    std::vector<double> deviations = {-4e-4, -2e-4, 2e-4, 4e-4};
    /// See `deviations` - these are applied for disc surfaces
    std::vector<double> deviationsDisc = {-3e-5, -1e-5, 1e-5, 3e-5};
  };

  template <typename actor_list_t = ActorList<>>
  using Options = typename propagator_t::template Options<actor_list_t>;

  /// @brief Constructor using a propagator
  ///
  /// @param [in] propagator Underlying propagator that will be used
  /// @param [in] config Config for the Ridders propagation
  RiddersPropagator(propagator_t propagator, Config config = {})
      : m_propagator(std::move(propagator)), m_config(std::move(config)) {}

  /// @brief Constructor building a propagator
  ///
  /// @tparam stepper_t Type of the stepper
  /// @tparam navigator_t Type of the navigator
  ///
  /// @param [in] stepper Stepper that will be used
  /// @param [in] navigator Navigator that will be used
  /// @param [in] config Config for the Ridders propagation
  template <typename stepper_t, typename navigator_t = VoidNavigator>
  RiddersPropagator(stepper_t stepper, navigator_t navigator = navigator_t(),
                    Config config = {})
      : m_propagator(std::move(stepper), std::move(navigator)),
        m_config(std::move(config)) {}

  /// @brief Propagation method targeting curvilinear parameters
  ///
  /// @tparam parameters_t Type of the start parameters
  /// @tparam propagator_options_t Type of the propagator options
  ///
  /// @param [in] start Start parameters
  /// @param [in] options Options of the propagations
  ///
  /// @return Result of the propagation
  template <typename parameters_t, typename propagator_options_t>
  Result<actor_list_t_result_t<CurvilinearTrackParameters,
                               typename propagator_options_t::actor_list_type>>
  propagate(const parameters_t& start,
            const propagator_options_t& options) const;

  /// @brief Propagation method targeting bound parameters
  ///
  /// @tparam parameters_t Type of the start parameters
  /// @tparam propagator_options_t Type of the propagator options
  ///
  /// @param [in] start Start parameters
  /// @param [in] target The target surface
  /// @param [in] options Options of the propagations
  ///
  /// @return Result of the propagation
  /// @note If the target surface is a disc, the resulting covariance may be
  /// inconsistent. In this case a zero matrix is returned.
  template <typename parameters_t, typename propagator_options_t>
  Result<actor_list_t_result_t<BoundTrackParameters,
                               typename propagator_options_t::actor_list_type>>
  propagate(const parameters_t& start, const Surface& target,
            const propagator_options_t& options) const;

 private:
  /// Does the actual ridders propagation by wiggling the parameters and
  /// propagating again. This function is called from the different propagation
  /// overloads in order to deduplicate code.
  ///
  /// @param [in] options Options of the propagations
  /// @param [in] start Start parameters
  /// @param [in] nominalResult The result of the nominal propagation
  template <typename propagator_options_t, typename parameters_t,
            typename result_t>
  Jacobian wiggleAndCalculateJacobian(const propagator_options_t& options,
                                      const parameters_t& start,
                                      result_t& nominalResult) const;

  /// @brief This function tests whether the variations on a disc as target
  /// surface lead to results on different sides wrt the center of the disc.
  /// This would lead to a flip of the phi value on the surface and therewith to
  /// a huge variance in that parameter. It can only occur in this algorithm
  /// since the ridders algorithm is unaware of the target surface.
  ///
  /// @param [in] derivatives Derivatives of a single parameter
  ///
  /// @return Boolean result whether a phi jump occurred
  static bool inconsistentDerivativesOnDisc(
      const std::vector<BoundVector>& derivatives);

  /// @brief This function wiggles one dimension of the starting parameters,
  /// performs the propagation to a surface and collects for each change of the
  /// start parameters the slope
  ///
  /// @tparam options_t PropagatorOptions object
  /// @tparam parameters_t Type of the parameters to start the propagation with
  ///
  /// @param [in] options Options do define how to wiggle
  /// @param [in] start Start parameters which will be modified
  /// @param [in] param Index to get the parameter that will be modified
  /// @param [in] target Target surface
  /// @param [in] nominal Nominal end parameters
  /// @param [in] deviations Vector of deviations
  ///
  /// @return Vector containing each slope
  template <typename propagator_options_t, typename parameters_t>
  std::vector<BoundVector> wiggleParameter(
      const propagator_options_t& options, const parameters_t& start,
      unsigned int param, const Surface& target, const BoundVector& nominal,
      const std::vector<double>& deviations) const;

  /// @brief This function fits the jacobian with the deviations and derivatives as input.
  ///
  /// @param [in] deviations Vector of deviations
  /// @param [in] derivatives Slopes of each modification of the parameters
  ///
  /// @return Propagated jacobian matrix
  static Jacobian calculateJacobian(
      const std::vector<double>& deviations,
      const std::array<std::vector<BoundVector>, eBoundSize>& derivatives);

  /// @brief This function fits a linear function through the final state
  /// parametrisations
  ///
  /// @param [in] deviations Vector of deviations
  /// @param [in] derivatives Vector of resulting derivatives
  ///
  /// @return Vector containing the linear fit
  static BoundVector fitLinear(const std::vector<double>& deviations,
                               const std::vector<BoundVector>& derivatives);

  propagator_t m_propagator;
  Config m_config;
};
}  // namespace Acts

#include "Acts/Propagator/RiddersPropagator.ipp"
