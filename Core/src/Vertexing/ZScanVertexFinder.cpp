// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Vertexing/ZScanVertexFinder.hpp"

Acts::ZScanVertexFinder::ZScanVertexFinder(const Config& cfg,
                                           std::unique_ptr<const Logger> logger)
    : m_cfg(cfg), m_logger(std::move(logger)) {
  if (!m_cfg.extractParameters.connected()) {
    throw std::invalid_argument(
        "ZScanVertexFinder: "
        "No track parameter extractor provided.");
  }
}

Acts::Result<std::vector<Acts::Vertex>> Acts::ZScanVertexFinder::find(
    const std::vector<InputTrack>& trackVector,
    const VertexingOptions& vertexingOptions,
    IVertexFinder::State& /*state*/) const {
  double ZResult = 0.;
  // Prepare the vector of points, on which the 3d mode has later to be
  // calculated
  std::vector<std::pair<double, double>> zPositions;

  for (const auto& iTrk : trackVector) {
    // Extract BoundTrackParameters from InputTrack_t object
    const BoundTrackParameters& params = m_cfg.extractParameters(iTrk);

    std::pair<double, double> z0AndWeight;
    ImpactParametersAndSigma ipas;
    if (vertexingOptions.useConstraintInFit &&
        vertexingOptions.constraint.covariance()(0, 0) != 0) {
      auto estRes = m_cfg.ipEstimator.getImpactParameters(
          params, vertexingOptions.constraint, vertexingOptions.geoContext,
          vertexingOptions.magFieldContext);
      if (estRes.ok()) {
        ipas = *estRes;
      } else {
        return estRes.error();
      }
    }

    if (ipas.sigmaD0 > 0) {
      // calculate z0
      z0AndWeight.first = ipas.z0 + vertexingOptions.constraint.position().z();

      // calculate chi2 of IP
      double chi2IP = std::pow(ipas.d0 / ipas.sigmaD0, 2);

      if (!m_cfg.disableAllWeights) {
        z0AndWeight.second =
            1. / (1. + std::exp((chi2IP - m_cfg.constraintcutoff) /
                                m_cfg.constrainttemp));
        // overflow protection
        if (!std::isnormal(z0AndWeight.second)) {
          z0AndWeight.second = 0.;
        }
      } else {
        z0AndWeight.second = 1.;
      }
    } else {
      ACTS_DEBUG(
          "Unable to compute IP significance. "
          "Setting IP weight to 1.");

      z0AndWeight.first = params.position(vertexingOptions.geoContext)[eZ];
      z0AndWeight.second = 1.;
    }

    // apply pT weighting as/if configured
    if (!m_cfg.disableAllWeights && (m_cfg.usePt || m_cfg.useLogPt)) {
      double Pt =
          std::abs(1. / params.parameters()[BoundIndices::eBoundQOverP]) *
          std::sin(params.parameters()[BoundIndices::eBoundTheta]);
      if (m_cfg.usePt) {
        z0AndWeight.second *= std::pow(Pt, m_cfg.expPt);
      } else {
        z0AndWeight.second *=
            Pt > m_cfg.minPt ? std::log(Pt / m_cfg.minPt) : 0.;
      }
    }

    if (z0AndWeight.second >= m_cfg.minWeight) {
      zPositions.push_back(z0AndWeight);
    }
  }  // end of loop over perigeeList

  if (!zPositions.empty()) {
    auto res = m_cfg.mode1dFinder.getMode(zPositions);
    if (res.ok()) {
      ZResult = *res;
    } else {
      return res.error();
    }

    ACTS_DEBUG("Resulting mean Z position found: " << ZResult);
  }

  // constraint x()/y() equals 0 if no constraint
  Vector4 output(vertexingOptions.constraint.position().x(),
                 vertexingOptions.constraint.position().y(), ZResult,
                 vertexingOptions.constraint.time());
  Vertex vtxResult = Vertex(output);

  return std::vector<Vertex>{vtxResult};
}
