// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Units.hpp"

#include <functional>
#include <limits>

namespace Acts {

class GeometryContext;
class MagneticFieldContext;

struct StepperPlainOptions {
  /// StepperPlainOptions with context
  StepperPlainOptions(const GeometryContext& gctx,
                      const MagneticFieldContext& mctx)
      : geoContext(gctx), magFieldContext(mctx) {}

  /// Context object for the geometry
  std::reference_wrapper<const GeometryContext> geoContext;

  /// Context object for the magnetic field
  std::reference_wrapper<const MagneticFieldContext> magFieldContext;

  /// Tolerance for the error of the integration
  double stepTolerance = 1e-4;

  /// Cut-off value for the step size
  double stepSizeCutOff = 0.;

  /// Initial step size
  double initialStepSize = 10 * Acts::UnitConstants::m;

  /// Absolute maximum step size
  double maxStepSize = std::numeric_limits<double>::max();

  /// Maximum number of Runge-Kutta steps for the stepper step call
  unsigned int maxRungeKuttaStepTrials = 10000;

  struct Dense {
    /// Toggle between mean and mode evaluation of energy loss
    bool meanEnergyLoss = true;

    /// Boolean flag for inclusion of d(dEds)d(q/p) into energy loss
    bool includeGradient = true;

    /// Cut-off value for the momentum in SI units
    double momentumCutOff = 0.;
  } dense;
};

}  // namespace Acts
