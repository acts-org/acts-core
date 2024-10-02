// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/ContextualDetector/InternallyAlignedDetectorElement.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Framework/IContextDecorator.hpp"
#include "ActsExamples/Framework/RandomNumbers.hpp"

#include <iostream>
#include <mutex>
#include <vector>

namespace ActsExamples::Contextual {

/// @brief A mockup service that rotates the modules in a
/// simple tracking geometry
///
/// It acts on the PayloadDetectorElement, i.e. the
/// geometry context carries the full transform store (payload)
class AlignmentDecorator : public IContextDecorator {
 public:
  /// @brief nested configuration struct
  struct Config {
    /// Alignment frequency - every X events
    unsigned int iovSize = 100;

    /// Flush store size - garbage collection
    bool doGarbageCollection = true;
    unsigned int flushSize = 200;

    std::shared_ptr<RandomNumbers> randomNumberSvc = nullptr;

    /// Gaussian module parameters - 6 Degrees of freedom
    double gSigmaX = 0.;  // smear position along local x Axis
    double gSigmaY = 0.;  // smear position along local y Axis
    double gSigmaZ = 0.;  // smear position along local z Axis
    double aSigmaX = 0.;  // rotate around local x Axis
    double aSigmaY = 0.;  // rotate around local y Axis
    double aSigmaZ = 0.;  // rotate around local z Axis

    bool firstIovNominal = false;
  };

 protected:
  static void applyTransform(Acts::Transform3& trf, const Config& cfg,
                             RandomEngine& rng, unsigned int iov) {
    std::normal_distribution<double> gauss(0., 1.);
    if (iov != 0 || !cfg.firstIovNominal) {
      // the shifts in x, y, z
      double tx = cfg.gSigmaX != 0 ? cfg.gSigmaX * gauss(rng) : 0.;
      double ty = cfg.gSigmaY != 0 ? cfg.gSigmaY * gauss(rng) : 0.;
      double tz = cfg.gSigmaZ != 0 ? cfg.gSigmaZ * gauss(rng) : 0.;
      // Add a translation - if there is any
      if (tx != 0. || ty != 0. || tz != 0.) {
        const auto& tMatrix = trf.matrix();
        auto colX = tMatrix.block<3, 1>(0, 0).transpose();
        auto colY = tMatrix.block<3, 1>(0, 1).transpose();
        auto colZ = tMatrix.block<3, 1>(0, 2).transpose();
        Acts::Vector3 newCenter = tMatrix.block<3, 1>(0, 3).transpose() +
                                  tx * colX + ty * colY + tz * colZ;
        trf.translation() = newCenter;
      }
      // now modify it - rotation around local X
      if (cfg.aSigmaX != 0.) {
        trf *=
            Acts::AngleAxis3(cfg.aSigmaX * gauss(rng), Acts::Vector3::UnitX());
      }
      if (cfg.aSigmaY != 0.) {
        trf *=
            Acts::AngleAxis3(cfg.aSigmaY * gauss(rng), Acts::Vector3::UnitY());
      }
      if (cfg.aSigmaZ != 0.) {
        trf *=
            Acts::AngleAxis3(cfg.aSigmaZ * gauss(rng), Acts::Vector3::UnitZ());
      }
    }
  }
};
}  // namespace ActsExamples::Contextual
