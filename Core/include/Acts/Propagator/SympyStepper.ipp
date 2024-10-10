// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
