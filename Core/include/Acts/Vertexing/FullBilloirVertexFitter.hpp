// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/MagneticField/MagneticFieldProvider.hpp"
#include "Acts/Propagator/EigenStepper.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/Result.hpp"
#include "Acts/Vertexing/HelicalTrackLinearizer.hpp"
#include "Acts/Vertexing/TrackLinearizer.hpp"
#include "Acts/Vertexing/Vertex.hpp"
#include "Acts/Vertexing/VertexingOptions.hpp"

namespace Acts {

/// @class FullBilloirVertexFitter
///
/// @brief Vertex fitter class implementing the Billoir vertex fitter
///
/// This class implements the Billoir vertex fitter from Ref. (1). It is also
/// useful to have a look at Ref. (2). The cross-covariance matrices are derived
/// in Ref. (3). Note that the Billoir vertex fitter outputs one 4D vertex
/// position and nTrack momenta at this very point.
///
/// Ref. (1):
/// Fast vertex fitting with a local parametrization of tracks.
/// Author(s) Billoir, P ; Qian, S
/// In: Nucl. Instrum. Methods Phys. Res., A 311 (1992) 139-150
/// DOI 10.1016/0168-9002(92)90859-3
///
/// Ref. (2):
/// Pattern Recognition, Tracking and Vertex Reconstruction in Particle
/// Detectors.
/// Author(s) Fruehwirth, R ; Strandli, A
///
/// Ref. (3):
/// ACTS White Paper: Cross-Covariance Matrices in the Billoir Vertex Fit
/// https://acts.readthedocs.io/en/latest/white_papers/billoir-covariances.html
/// Author(s) Russo, F
class FullBilloirVertexFitter {
 public:
  struct Config {
    /// Maximum number of iterations in fitter
    int maxIterations = 5;

    // Function to extract parameters from InputTrack
    InputTrack::Extractor extractParameters;

    TrackLinearizer trackLinearizer;
  };

  /// @brief Constructor for user-defined InputTrack type
  ///
  /// @param cfg Configuration object
  /// @param logger Logging instance
  FullBilloirVertexFitter(const Config& cfg,
                          std::unique_ptr<const Logger> logger =
                              getDefaultLogger("FullBilloirVertexFitter",
                                               Logging::INFO))
      : m_cfg(cfg), m_logger(std::move(logger)) {
    if (!m_cfg.extractParameters.connected()) {
      throw std::invalid_argument(
          "FullBilloirVertexFitter: "
          "No function to extract parameters "
          "provided.");
    }

    if (!m_cfg.trackLinearizer.connected()) {
      throw std::invalid_argument(
          "FullBilloirVertexFitter: "
          "No track linearizer provided.");
    }
  }

  /// @brief Fit method, fitting vertex for provided tracks with constraint
  ///
  /// @param paramVector Vector of track objects to fit vertex to
  /// @param vertexingOptions Vertexing options
  /// @param fieldCache The magnetic field cache
  ///
  /// @return Fitted vertex
  Result<Vertex> fit(const std::vector<InputTrack>& paramVector,
                     const VertexingOptions& vertexingOptions,
                     MagneticFieldProvider::Cache& fieldCache) const;

 private:
  /// Configuration object
  Config m_cfg;

  /// Logging instance
  std::unique_ptr<const Logger> m_logger;

  /// Private access to logging instance
  const Logger& logger() const { return *m_logger; }
};

}  // namespace Acts
