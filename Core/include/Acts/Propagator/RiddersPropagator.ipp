// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Definitions/TrackParametrization.hpp"

#include <numbers>

template <typename propagator_t>
template <typename parameters_t, typename propagator_options_t>
auto Acts::RiddersPropagator<propagator_t>::propagate(
    const parameters_t& start, const propagator_options_t& options) const
    -> Result<actor_list_t_result_t<
        BoundTrackParameters, typename propagator_options_t::actor_list_type>> {
  using ThisResult = Result<actor_list_t_result_t<
      BoundTrackParameters, typename propagator_options_t::actor_list_type>>;

  // Remove the covariance from our start parameters in order to skip jacobian
  // transport for the nominal propagation
  BoundTrackParameters startWithoutCov = start;
  startWithoutCov.covariance() = std::nullopt;

  // Propagate the nominal parameters
  auto result = m_propagator.propagate(startWithoutCov, options);
  if (!result.ok()) {
    return ThisResult::failure(result.error());
  }
  // Extract results from the nominal propagation
  auto nominalResult = result.value();
  assert(nominalResult.endParameters);
  const auto& nominalFinalParameters = *nominalResult.endParameters;

  Jacobian jacobian = wiggleAndCalculateJacobian(options, start, nominalResult);
  nominalResult.transportJacobian = jacobian;

  if (start.covariance()) {
    // use nominal parameters and Ridders covariance
    auto cov = jacobian * (*start.covariance()) * jacobian.transpose();
    // replace the covariance of the nominal result w/ the ridders covariance
    nominalResult.endParameters = BoundTrackParameters::makeCurvilinear(
        nominalFinalParameters.fourPosition(options.geoContext),
        nominalFinalParameters.direction(), nominalFinalParameters.qOverP(),
        std::move(cov), nominalFinalParameters.particleHypothesis());
  }

  return ThisResult::success(std::move(nominalResult));
}

template <typename propagator_t>
template <typename parameters_t, typename propagator_options_t>
auto Acts::RiddersPropagator<propagator_t>::propagate(
    const parameters_t& start, const Surface& target,
    const propagator_options_t& options) const
    -> Result<actor_list_t_result_t<
        BoundTrackParameters, typename propagator_options_t::actor_list_type>> {
  using ThisResult = Result<actor_list_t_result_t<
      BoundTrackParameters, typename propagator_options_t::actor_list_type>>;

  // Remove the covariance from our start parameters in order to skip jacobian
  // transport for the nominal propagation
  BoundTrackParameters startWithoutCov = start;
  startWithoutCov.covariance() = std::nullopt;

  // Propagate the nominal parameters
  auto result = m_propagator.propagate(startWithoutCov, target, options);
  if (!result.ok()) {
    return ThisResult::failure(result.error());
  }
  // Extract results from the nominal propagation
  auto nominalResult = result.value();
  assert(nominalResult.endParameters);
  const auto& nominalFinalParameters = *nominalResult.endParameters;

  Jacobian jacobian = wiggleAndCalculateJacobian(options, start, nominalResult);
  nominalResult.transportJacobian = jacobian;

  if (start.covariance()) {
    // use nominal parameters and Ridders covariance
    auto cov = jacobian * (*start.covariance()) * jacobian.transpose();
    // replace the covariance of the nominal result w/ the ridders covariance
    nominalResult.endParameters = BoundTrackParameters(
        nominalFinalParameters.referenceSurface().getSharedPtr(),
        nominalFinalParameters.parameters(), std::move(cov),
        nominalFinalParameters.particleHypothesis());
  }

  return ThisResult::success(std::move(nominalResult));
}

template <typename propagator_t>
template <typename propagator_options_t, typename parameters_t,
          typename result_t>
auto Acts::RiddersPropagator<propagator_t>::wiggleAndCalculateJacobian(
    const propagator_options_t& options, const parameters_t& start,
    result_t& nominalResult) const -> Jacobian {
  auto nominalFinalParameters = *nominalResult.endParameters;
  // Use the curvilinear surface of the propagated parameters as target
  const Surface& target = nominalFinalParameters.referenceSurface();

  // TODO add to propagator options
  // Steps for estimating derivatives
  const std::vector<double>* deviations = &m_config.deviations;
  if (target.type() == Surface::Disc) {
    deviations = &m_config.deviationsDisc;
  }

  // - for planar surfaces the dest surface is a perfect destination
  // surface for the numerical propagation, as reference frame
  // aligns with the referenceSurface.transform().rotation() at
  // at any given time
  //
  // - for straw & cylinder, where the error is given
  // in the reference frame that re-aligns with a slightly different
  // intersection solution

  // Allow larger distances for the oscillation
  propagator_options_t opts = options;
  opts.pathLimit *= 2.;

  // Derivations of each parameter around the nominal parameters
  std::array<std::vector<BoundVector>, eBoundSize> derivatives;

  // Wiggle each dimension individually
  for (unsigned int i = 0; i < eBoundSize; i++) {
    derivatives[i] =
        wiggleParameter(opts, start, i, target,
                        nominalFinalParameters.parameters(), *deviations);
  }

  // Test if target is disc - this may lead to inconsistent results
  if (target.type() == Surface::Disc) {
    for ([[maybe_unused]] const auto& d : derivatives) {
      assert(!inconsistentDerivativesOnDisc(d));
    }
  }

  return calculateJacobian(*deviations, derivatives);
}

template <typename propagator_t>
bool Acts::RiddersPropagator<propagator_t>::inconsistentDerivativesOnDisc(
    const std::vector<Acts::BoundVector>& derivatives) {
  // Test each component with each other
  for (unsigned int i = 0; i < derivatives.size(); i++) {
    bool jumpedAngle = true;
    for (unsigned int j = 0; j < derivatives.size(); j++) {
      // If there is at least one with a similar angle then it seems to work
      // properly
      if (i != j && std::abs(derivatives[i](1) - derivatives[j](1)) <
                        std::numbers::pi / 2.) {
        jumpedAngle = false;
        break;
      }
    }
    // Break if a jump was detected
    if (jumpedAngle) {
      return true;
    }
  }
  return false;
}

template <typename propagator_t>
template <typename propagator_options_t, typename parameters_t>
std::vector<Acts::BoundVector>
Acts::RiddersPropagator<propagator_t>::wiggleParameter(
    const propagator_options_t& options, const parameters_t& start,
    const unsigned int param, const Surface& target,
    const Acts::BoundVector& nominal,
    const std::vector<double>& deviations) const {
  // Storage of the results
  std::vector<BoundVector> derivatives;
  derivatives.reserve(deviations.size());
  for (double h : deviations) {
    // Treatment for theta
    if (param == eBoundTheta) {
      const double current_theta = start.template get<eBoundTheta>();
      if (current_theta + h > std::numbers::pi) {
        h = std::numbers::pi - current_theta;
      }
      if (current_theta + h < 0) {
        h = -current_theta;
      }
    }

    // Modify start parameter
    BoundVector values = start.parameters();
    values[param] += h;

    // Propagate with updated start parameters
    BoundTrackParameters tp(start.referenceSurface().getSharedPtr(), values,
                            start.covariance(), start.particleHypothesis());
    const auto& r = m_propagator.propagate(tp, target, options).value();
    // Collect the slope
    derivatives.push_back((r.endParameters->parameters() - nominal) / h);

    // Correct for a possible variation of phi around
    if (param == eBoundPhi) {
      double phi0 = nominal(Acts::eBoundPhi);
      double phi1 = r.endParameters->parameters()(Acts::eBoundPhi);
      if (std::abs(phi1 + 2. * std::numbers::pi - phi0) <
          std::abs(phi1 - phi0)) {
        derivatives.back()[Acts::eBoundPhi] =
            (phi1 + 2. * std::numbers::pi - phi0) / h;
      } else if (std::abs(phi1 - 2. * std::numbers::pi - phi0) <
                 std::abs(phi1 - phi0)) {
        derivatives.back()[Acts::eBoundPhi] =
            (phi1 - 2. * std::numbers::pi - phi0) / h;
      }
    }
  }

  return derivatives;
}

template <typename propagator_t>
auto Acts::RiddersPropagator<propagator_t>::calculateJacobian(
    const std::vector<double>& deviations,
    const std::array<std::vector<Acts::BoundVector>, Acts::eBoundSize>&
        derivatives) -> Jacobian {
  Jacobian jacobian;
  jacobian.col(eBoundLoc0) = fitLinear(deviations, derivatives[eBoundLoc0]);
  jacobian.col(eBoundLoc1) = fitLinear(deviations, derivatives[eBoundLoc1]);
  jacobian.col(eBoundPhi) = fitLinear(deviations, derivatives[eBoundPhi]);
  jacobian.col(eBoundTheta) = fitLinear(deviations, derivatives[eBoundTheta]);
  jacobian.col(eBoundQOverP) = fitLinear(deviations, derivatives[eBoundQOverP]);
  jacobian.col(eBoundTime) = fitLinear(deviations, derivatives[eBoundTime]);
  return jacobian;
}

template <typename propagator_t>
Acts::BoundVector Acts::RiddersPropagator<propagator_t>::fitLinear(
    const std::vector<double>& deviations,
    const std::vector<Acts::BoundVector>& derivatives) {
  assert(!derivatives.empty() && "no fit without data");
  if (derivatives.size() == 1) {
    return derivatives[0];
  }

  BoundVector A = BoundVector::Zero();
  BoundVector C = BoundVector::Zero();
  double B = 0;
  double D = 0;
  const unsigned int N = deviations.size();

  for (unsigned int i = 0; i < N; ++i) {
    A += deviations.at(i) * derivatives.at(i);
    B += deviations.at(i);
    C += derivatives.at(i);
    D += deviations.at(i) * deviations.at(i);
  }

  BoundVector b = (N * A - B * C) / (N * D - B * B);
  BoundVector a = (C - B * b) / N;

  return a;
}
