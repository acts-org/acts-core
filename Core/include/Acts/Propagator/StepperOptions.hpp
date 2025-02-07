// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Units.hpp"

#include <functional>

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

  /// Absolute maximum step size
  double maxStepSize = 10 * Acts::UnitConstants::m;

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
