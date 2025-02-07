// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Propagator/EigenStepperError.hpp"

namespace Acts {

template <typename propagator_state_t, typename navigator_t>
Result<double> SympyStepper::step(propagator_state_t& state,
                                  const navigator_t& /*navigator*/) const {
  return stepImpl(state.stepping, state.options.direction,
                  state.options.stepping.stepTolerance,
                  state.options.stepping.stepSizeCutOff,
                  state.options.stepping.maxRungeKuttaStepTrials);
}

}  // namespace Acts
