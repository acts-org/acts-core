// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Propagator/StraightLineStepper.hpp"

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/TransformationHelpers.hpp"
#include "Acts/Propagator/detail/CovarianceEngine.hpp"

namespace Acts {

StraightLineStepper::State StraightLineStepper::makeState(
    const Options& options) const {
  State state{options};
  return state;
}

void StraightLineStepper::initialize(State& state,
                                     const BoundTrackParameters& par) const {
  initialize(state, par.parameters(), par.covariance(),
             par.particleHypothesis(), par.referenceSurface());
}

void StraightLineStepper::initialize(State& state,
                                     const BoundVector& boundParams,
                                     const std::optional<BoundMatrix>& cov,
                                     ParticleHypothesis particleHypothesis,
                                     const Surface& surface) const {
  FreeVector freeParams = transformBoundToFreeParameters(
      surface, state.options.geoContext, boundParams);

  state.particleHypothesis = particleHypothesis;

  state.pathAccumulated = 0;
  state.nSteps = 0;
  state.nStepTrials = 0;
  state.stepSize = ConstrainedStep();
  state.stepSize.setAccuracy(state.options.initialStepSize);
  state.stepSize.setUser(state.options.maxStepSize);
  state.previousStepSize = 0;
  state.statistics = StepperStatistics();

  state.pars = freeParams;

  // Init the jacobian matrix if needed
  state.covTransport = cov.has_value();
  if (state.covTransport) {
    state.cov = *cov;
    state.jacToGlobal = surface.boundToFreeJacobian(
        state.options.geoContext, freeParams.segment<3>(eFreePos0),
        freeParams.segment<3>(eFreeDir0));
    state.jacobian = BoundMatrix::Identity();
    state.jacTransport = FreeMatrix::Identity();
    state.derivative = FreeVector::Zero();
  }
}

Result<std::tuple<BoundTrackParameters, BoundMatrix, double>>
StraightLineStepper::boundState(
    State& state, const Surface& surface, bool transportCov,
    const FreeToBoundCorrection& freeToBoundCorrection) const {
  return detail::boundState(
      state.options.geoContext, surface, state.cov, state.jacobian,
      state.jacTransport, state.derivative, state.jacToGlobal, state.pars,
      state.particleHypothesis, state.covTransport && transportCov,
      state.pathAccumulated, freeToBoundCorrection);
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
      state.options.geoContext, freeParams.template segment<3>(eFreePos0),
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
      state.options.geoContext, surface, state.cov, state.jacobian,
      state.jacTransport, state.derivative, state.jacToGlobal, state.pars,
      freeToBoundCorrection);
}

}  // namespace Acts
