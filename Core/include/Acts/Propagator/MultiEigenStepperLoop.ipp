// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Propagator/MultiEigenStepperLoop.hpp"
#include "Acts/Utilities/Logger.hpp"

namespace Acts {

template <typename E, typename R>
auto MultiEigenStepperLoop<E, R>::boundState(
    State& state, const Surface& surface, bool transportCov,
    const FreeToBoundCorrection& freeToBoundCorrection) const
    -> Result<BoundState> {
  assert(!state.components.empty());

  std::vector<std::tuple<double, BoundVector, Covariance>> cmps;
  cmps.reserve(numberComponents(state));
  double accumulatedPathLength = 0.0;

  for (auto i = 0ul; i < numberComponents(state); ++i) {
    auto& cmpState = state.components[i].state;

    // Force the component to be on the surface
    // This needs to be done because of the `averageOnSurface`-option of the
    // `MultiStepperSurfaceReached`-Aborter, which can be configured to end the
    // propagation when the mean of all components reached the destination
    // surface. Thus, it is not garantueed that all states are actually
    // onSurface.
    cmpState.pars.template segment<3>(eFreePos0) =
        surface
            .intersect(state.geoContext,
                       cmpState.pars.template segment<3>(eFreePos0),
                       cmpState.pars.template segment<3>(eFreeDir0),
                       BoundaryTolerance::Infinite())
            .closest()
            .position();

    auto bs = SingleStepper::boundState(cmpState, surface, transportCov,
                                        freeToBoundCorrection);

    if (bs.ok()) {
      const auto& btp = std::get<BoundTrackParameters>(*bs);
      cmps.emplace_back(
          state.components[i].weight, btp.parameters(),
          btp.covariance().value_or(Acts::BoundSquareMatrix::Zero()));
      accumulatedPathLength +=
          std::get<double>(*bs) * state.components[i].weight;
    }
  }

  if (cmps.empty()) {
    return MultiStepperError::AllComponentsConversionToBoundFailed;
  }

  return BoundState{MultiComponentBoundTrackParameters(
                        surface.getSharedPtr(), cmps, state.particleHypothesis),
                    Jacobian::Zero(), accumulatedPathLength};
}

template <typename E, typename R>
auto MultiEigenStepperLoop<E, R>::curvilinearState(
    State& state, bool transportCov) const -> CurvilinearState {
  assert(!state.components.empty());

  std::vector<
      std::tuple<double, Vector4, Vector3, ActsScalar, BoundSquareMatrix>>
      cmps;
  cmps.reserve(numberComponents(state));
  double accumulatedPathLength = 0.0;

  for (auto i = 0ul; i < numberComponents(state); ++i) {
    const auto [cp, jac, pl] = SingleStepper::curvilinearState(
        state.components[i].state, transportCov);

    cmps.emplace_back(state.components[i].weight,
                      cp.fourPosition(state.geoContext), cp.direction(),
                      cp.qOverP(),
                      cp.covariance().value_or(BoundSquareMatrix::Zero()));
    accumulatedPathLength += state.components[i].weight * pl;
  }

  return CurvilinearState{
      MultiComponentCurvilinearTrackParameters(cmps, state.particleHypothesis),
      Jacobian::Zero(), accumulatedPathLength};
}

template <typename E, typename R>
template <typename propagator_state_t, typename navigator_t>
Result<double> MultiEigenStepperLoop<E, R>::step(
    propagator_state_t& state, const navigator_t& navigator) const {
  using Status = Acts::Intersection3D::Status;

  State& stepping = state.stepping;
  auto& components = stepping.components;
  const Logger& logger = *m_logger;

  // Update step count
  stepping.steps++;

  // Check if we abort because of m_stepLimitAfterFirstComponentOnSurface
  if (stepping.stepCounterAfterFirstComponentOnSurface) {
    (*stepping.stepCounterAfterFirstComponentOnSurface)++;

    // If the limit is reached, remove all components which are not on a
    // surface, reweight the components, perform no step and return 0
    if (*stepping.stepCounterAfterFirstComponentOnSurface >=
        m_stepLimitAfterFirstComponentOnSurface) {
      for (auto& cmp : components) {
        if (cmp.status != Status::onSurface) {
          cmp.status = Status::missed;
        }
      }

      removeMissedComponents(stepping);
      reweightComponents(stepping);

      ACTS_VERBOSE("Stepper performed "
                   << m_stepLimitAfterFirstComponentOnSurface
                   << " after the first component hit a surface.");
      ACTS_VERBOSE(
          "-> remove all components not on a surface, perform no step");

      stepping.stepCounterAfterFirstComponentOnSurface.reset();

      return 0.0;
    }
  }

  // Flag indicating if we need to reweight in the end
  bool reweightNecessary = false;

  // If at least one component is on a surface, we can remove all missed
  // components before the step. If not, we must keep them for the case that all
  // components miss and we need to retarget
  const auto cmpsOnSurface =
      std::count_if(components.cbegin(), components.cend(), [&](auto& cmp) {
        return cmp.status == Intersection3D::Status::onSurface;
      });

  if (cmpsOnSurface > 0) {
    removeMissedComponents(stepping);
    reweightNecessary = true;
  }

  // Loop over all components and collect results in vector, write some
  // summary information to a stringstream
  SmallVector<std::optional<Result<double>>> results;
  double accumulatedPathLength = 0.0;
  std::size_t errorSteps = 0;

  // Type of the proxy single propagation2 state
  using ThisSinglePropState =
      detail::SinglePropState<SingleState, decltype(state.navigation),
                              decltype(state.options),
                              decltype(state.geoContext)>;

  // Lambda that performs the step for a component and returns false if the step
  // went ok and true if there was an error
  auto errorInStep = [&](auto& component) {
    if (component.status == Status::onSurface) {
      // We need to add these, so the propagation does not fail if we have only
      // components on surfaces and failing states
      results.emplace_back(std::nullopt);
      return false;
    }

    ThisSinglePropState single_state(component.state, state.navigation,
                                     state.options, state.geoContext);

    results.emplace_back(SingleStepper::step(single_state, navigator));

    if (results.back()->ok()) {
      accumulatedPathLength += component.weight * results.back()->value();
      return false;
    } else {
      ++errorSteps;
      reweightNecessary = true;
      return true;
    }
  };

  // Loop over components and remove errorous components
  stepping.components.erase(
      std::remove_if(components.begin(), components.end(), errorInStep),
      components.end());

  // Reweight if necessary
  if (reweightNecessary) {
    reweightComponents(stepping);
  }

  // Print the result vector to a string so we can log it
  auto summary = [](auto& result_vec) {
    std::stringstream ss;
    for (auto& optRes : result_vec) {
      if (!optRes) {
        ss << "on surface | ";
      } else if (optRes->ok()) {
        ss << optRes->value() << " | ";
      } else {
        ss << optRes->error() << " | ";
      }
    }
    auto str = ss.str();
    str.resize(str.size() - 3);
    return str;
  };

  // Print the summary
  if (errorSteps == 0) {
    ACTS_VERBOSE("Performed steps: " << summary(results));
  } else {
    ACTS_WARNING("Performed steps with errors: " << summary(results));
  }

  // Return error if there is no ok result
  if (stepping.components.empty()) {
    return MultiStepperError::AllComponentsSteppingError;
  }

  // Return the weighted accumulated path length of all successful steps
  stepping.pathAccumulated += accumulatedPathLength;
  return accumulatedPathLength;
}

}  // namespace Acts
