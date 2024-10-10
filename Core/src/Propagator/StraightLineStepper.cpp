// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Propagator/StraightLineStepper.hpp"

#include "Acts/EventData/TransformationHelpers.hpp"
#include "Acts/Propagator/detail/CovarianceEngine.hpp"

namespace Acts {

Result<std::tuple<BoundTrackParameters, BoundMatrix, double>>
StraightLineStepper::boundState(
    State& state, const Surface& surface, bool transportCov,
    const FreeToBoundCorrection& freeToBoundCorrection) const {
  return detail::boundState(
      state.geoContext, surface, state.cov, state.jacobian, state.jacTransport,
      state.derivative, state.jacToGlobal, state.pars, state.particleHypothesis,
      state.covTransport && transportCov, state.pathAccumulated,
      freeToBoundCorrection);
}

std::tuple<CurvilinearTrackParameters, BoundMatrix, double>
StraightLineStepper::curvilinearState(State& state, bool transportCov) const {
  return detail::curvilinearState(
      state.cov, state.jacobian, state.jacTransport, state.derivative,
      state.jacToGlobal, state.pars, state.particleHypothesis,
      state.covTransport && transportCov, state.pathAccumulated);
}

void StraightLineStepper::update(State& state, const FreeVector& freeParams,
                                 const BoundVector& /*boundParams*/,
                                 const Covariance& covariance,
                                 const Surface& surface) const {
  state.pars = freeParams;
  state.cov = covariance;
  state.jacToGlobal = surface.boundToFreeJacobian(
      state.geoContext, freeParams.template segment<3>(eFreePos0),
      freeParams.template segment<3>(eFreeDir0));
}

void StraightLineStepper::update(State& state, const Vector3& uposition,
                                 const Vector3& udirection, double qop,
                                 double time) const {
  state.pars.template segment<3>(eFreePos0) = uposition;
  state.pars.template segment<3>(eFreeDir0) = udirection;
  state.pars[eFreeTime] = time;
  state.pars[eFreeQOverP] = qop;
}

void StraightLineStepper::transportCovarianceToCurvilinear(State& state) const {
  detail::transportCovarianceToCurvilinear(
      state.cov, state.jacobian, state.jacTransport, state.derivative,
      state.jacToGlobal, state.pars.template segment<3>(eFreeDir0));
}

void StraightLineStepper::transportCovarianceToBound(
    State& state, const Surface& surface,
    const FreeToBoundCorrection& freeToBoundCorrection) const {
  detail::transportCovarianceToBound(
      state.geoContext, surface, state.cov, state.jacobian, state.jacTransport,
      state.derivative, state.jacToGlobal, state.pars, freeToBoundCorrection);
}

void StraightLineStepper::resetState(State& state,
                                     const BoundVector& boundParams,
                                     const BoundSquareMatrix& cov,
                                     const Surface& surface,
                                     const double stepSize) const {
  FreeVector freeParams =
      transformBoundToFreeParameters(surface, state.geoContext, boundParams);

  // Update the stepping state
  state.pars = freeParams;
  state.cov = cov;
  state.stepSize = ConstrainedStep(stepSize);
  state.pathAccumulated = 0.;

  // Reinitialize the stepping jacobian
  state.jacToGlobal = surface.boundToFreeJacobian(
      state.geoContext, freeParams.template segment<3>(eFreePos0),
      freeParams.template segment<3>(eFreeDir0));
  state.jacobian = BoundMatrix::Identity();
  state.jacTransport = FreeMatrix::Identity();
  state.derivative = FreeVector::Zero();
}

}  // namespace Acts
