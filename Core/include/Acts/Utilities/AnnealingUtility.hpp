// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"

#include <cmath>
#include <vector>

namespace Acts {
/// @brief Implements a deterministic thermodynamic annealing scheme
/// Ref. (1): CERN-THESIS-2010-027
class AnnealingUtility {
 public:
  /// @brief The annealing state
  /// Resetting the state is done by just creating a new instance
  struct State {
    // Points to current temperature value in m_cfg.setOfTemperatures
    unsigned int currentTemperatureIndex{0};

    // Checks if equilibrium is reached
    bool equilibriumReached{false};
  };

  /// @brief The configuration struct
  struct Config {
    Config();

    Config(double cutOff_, std::vector<double> setOfTemperatures_)
        : cutOff(cutOff_), setOfTemperatures(std::move(setOfTemperatures_)) {}

    // Insensitivity of calculated weight at cutoff
    double cutOff{9.};

    // Set of temperatures, annealing starts at setOfTemperatures[0]
    // and anneals towards setOfTemperatures[last]
    std::vector<double> setOfTemperatures{64., 16., 4., 2., 1.5, 1.};
  };

  /// Constructor
  AnnealingUtility(const Config& cfg = Config()) : m_cfg(cfg) {
    // Set Gaussian cut-off terms for each temperature
    for (double temp : cfg.setOfTemperatures) {
      m_gaussCutTempVec.push_back(std::exp(-cfg.cutOff / (2. * temp)));
    }
  }

  /// Does the actual annealing step
  void anneal(State& state) const;

  /// @brief Weight access
  ///
  /// @param state The state object
  /// @param chi2 Chi^2 for e.g. current track, i.e. compatibility
  /// of track to current vertex candidate
  /// @param allChi2 Vector of all chi^2 values, i.e. e.g. compatibilities
  /// of current track to all vertices it is currently attached to
  ///
  /// @return Calculated weight according to Eq.(5.46) in Ref.(1)
  double getWeight(State& state, double chi2,
                   const std::vector<double>& allChi2) const;

  /// @brief Weight access
  ///
  /// @param state The state object
  /// @param chi2 Chi^2
  ///
  /// @return Calculated weight
  double getWeight(State& state, double chi2) const;

 private:
  /// Configuration object
  Config m_cfg;

  // For each temperature, a Gaussian term with the chi2 cut-off value
  // is calculated and stored here
  std::vector<double> m_gaussCutTempVec;
};
}  // namespace Acts
