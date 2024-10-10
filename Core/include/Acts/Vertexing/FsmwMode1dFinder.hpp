// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/Result.hpp"

#include <sstream>
#include <utility>
#include <vector>

namespace Acts {

/// @class FsmwMode1dFinder

/// Calculates the mode of a unidimenensional distribution
/// using the Fraction of Sample Mode with Weights algorithm
/// For reference, see:
/// On a Fast, Robust Estimator of the Mode:
/// Comparisons to Other Robust Estimators
/// with Applications, David R. Bickel, Rudolf Fruehwirth, arXiv:math/0505419

/// It's like an iterative "Half Sample Mode", but the fraction you take at each
/// step can be configured by the user.

/// Configuration possibilities:
/// (1) fraction (default is 50 %)
/// (2) firstFraction (default is 50 %)
class FsmwMode1dFinder {
 public:
  /// Default constructor
  FsmwMode1dFinder() = default;

  /// Overload constructor
  ///
  /// @param firstFraction first fraction in FSMW algorithm
  /// @param fraction all other fractions in FSMW algorithm
  FsmwMode1dFinder(double firstFraction, double fraction);

  /// @brief Function to calculate mode with FSMW algorithm
  ///
  /// @param inputVector Input collection to calculate mode from
  ///
  /// @return mode value
  Result<double> getMode(
      std::vector<std::pair<double, double>> inputVector) const;

 private:
  double m_firstFraction = 0.5;
  double m_fraction = 0.5;
};

}  // namespace Acts
