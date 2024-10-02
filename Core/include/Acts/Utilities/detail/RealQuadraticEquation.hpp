// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

///////////////////////////////////////////////////////////////////
// RealQuadraticEquation.h, Acts project
///////////////////////////////////////////////////////////////////

#pragma once
#include <cmath>
#include <utility>

namespace Acts::detail {

/// @struct RealQuadradicEquation
///   Mathematic struct for solving real quadratic equations
///
///  <b>Mathematical motivation</b>:<br>
///  The equation is given by:<br>
///  @f$ \alpha x^{2} + \beta x + \gamma = 0  @f$
///  and has therefore the analytical solution:<br>
///  @f$ x_{1, 2} = - \frac{\beta \pm
///  \sqrt{\beta^{2}-4\alpha\gamma}}{2\alpha}@f$
/// <br>
/// <br>
///  - case @f$ \beta > 0 @f$:<br>
///  @f$ x_{1} = - \frac{\beta + \sqrt{\beta^{2}-4\alpha\gamma}}{2\alpha}  :=
/// \frac{q}{\alpha}@f$, <br>
///  so that @f$ q= -\frac{1}{2}(\beta+sqrt{\beta^{2}-4\alpha\gamma})@f$.
///  @f$ x_{2} @f$ can now be written as: @f$ x_{2} = \frac{\gamma}{q} =
/// -\frac{2\gamma}{\beta+sqrt{\beta^{2}-4\alpha\gamma}}@f$, since: <br>
///  @f$ -\frac{2\gamma}{\beta+sqrt{\beta^{2}-4\alpha\gamma}} =
/// -\frac{2\gamma}{\beta}\frac{1}{1+\sqrt{1-4\alpha\gamma/\beta^{2}}}@f$, and
/// <br>
///  @f$ x_{2}\frac{1}{1-\sqrt{1-4\alpha\gamma/\beta^{2}}} =
/// -\frac{2\gamma}{\beta}\frac{1}{1-1+4\alpha\gamma/\beta^{2}}=-\frac{\beta}{2\alpha}.@f$<br>
///  Hence,@f$ -\frac{\beta(1-\sqrt{1-4\alpha\gamma/\beta^{2}}}{2\alpha} = -
/// \frac{\beta - \sqrt{\beta^{2}-4\alpha\gamma}}{2\alpha} @f$.<br>
///  - case @f$ \beta > 0 @f$ is similar.
///

struct RealQuadraticEquation {
  double first = 0;
  double second = 0;
  int solutions = 0;

  /// Constructor
  ///
  /// @param alpha is the first parameter of the quad equation
  /// @param beta is the second parameter of the quad equation
  /// @param gamma is the third parameter of the quad equation
  RealQuadraticEquation(double alpha, double beta, double gamma) {
    double discriminant = beta * beta - 4 * alpha * gamma;
    if (discriminant >= 0) {
      solutions = (discriminant == 0) ? 1 : 2;
      double q = -0.5 * (beta + (beta > 0 ? std::sqrt(discriminant)
                                          : -std::sqrt(discriminant)));
      first = q / alpha;
      second = gamma / q;
    }
  }
};

}  // namespace Acts::detail
