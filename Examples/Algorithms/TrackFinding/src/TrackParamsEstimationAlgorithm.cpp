// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/TrackFinding/TrackParamsEstimationAlgorithm.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/ParticleHypothesis.hpp"
#include "Acts/EventData/Seed.hpp"
#include "Acts/EventData/SourceLink.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/MagneticField/MagneticFieldProvider.hpp"
#include "Acts/Seeding/EstimateTrackParamsFromSeed.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Result.hpp"
#include "ActsExamples/EventData/IndexSourceLink.hpp"
#include "ActsExamples/EventData/SimSpacePoint.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"

#include <cmath>
#include <cstddef>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <system_error>
#include <utility>
#include <vector>

namespace ActsExamples {

namespace {

Acts::BoundSquareMatrix makeInitialCovariance(
    const TrackParamsEstimationAlgorithm::Config& config,
    const Acts::BoundVector& params, const SimSpacePoint& sp) {
  Acts::BoundSquareMatrix result = Acts::BoundSquareMatrix::Zero();

  for (std::size_t i = Acts::eBoundLoc0; i < Acts::eBoundSize; ++i) {
    double sigma = config.initialSigmas[i];
    double variance = sigma * sigma;

    if (i == Acts::eBoundQOverP) {
      // note that we rely on the fact that sigma theta is already computed
      double varianceTheta = result(Acts::eBoundTheta, Acts::eBoundTheta);

      // transverse momentum contribution
      variance +=
          std::pow(config.initialSigmaPtRel * params[Acts::eBoundQOverP], 2);

      // theta contribution
      variance +=
          varianceTheta * std::pow(params[Acts::eBoundQOverP] *
                                       std::tan(params[Acts::eBoundTheta]),
                                   2);
    }

    // Inflate the time uncertainty if no time measurement is available
    if (i == Acts::eBoundTime && !sp.t().has_value()) {
      variance *= config.noTimeVarInflation;
    }

    // Inflate the initial covariance
    variance *= config.initialVarInflation[i];

    result(i, i) = variance;
  }

  return result;
}

}  // namespace

ActsExamples::TrackParamsEstimationAlgorithm::TrackParamsEstimationAlgorithm(
    ActsExamples::TrackParamsEstimationAlgorithm::Config cfg,
    Acts::Logging::Level lvl)
    : ActsExamples::IAlgorithm("TrackParamsEstimationAlgorithm", lvl),
      m_cfg(std::move(cfg)) {
  if (m_cfg.inputSeeds.empty()) {
    throw std::invalid_argument("Missing seeds input collection");
  }
  if (m_cfg.outputTrackParameters.empty()) {
    throw std::invalid_argument("Missing track parameters output collection");
  }
  if (!m_cfg.trackingGeometry) {
    throw std::invalid_argument("Missing tracking geometry");
  }
  if (!m_cfg.magneticField) {
    throw std::invalid_argument("Missing magnetic field");
  }

  m_inputSeeds.initialize(m_cfg.inputSeeds);
  m_inputTracks.maybeInitialize(m_cfg.inputProtoTracks);

  m_outputTrackParameters.initialize(m_cfg.outputTrackParameters);
  m_outputSeeds.maybeInitialize(m_cfg.outputSeeds);
  m_outputTracks.maybeInitialize(m_cfg.outputProtoTracks);
}

ActsExamples::ProcessCode ActsExamples::TrackParamsEstimationAlgorithm::execute(
    const ActsExamples::AlgorithmContext& ctx) const {
  auto const& seeds = m_inputSeeds(ctx);
  ACTS_VERBOSE("Read " << seeds.size() << " seeds");

  TrackParametersContainer trackParameters;
  trackParameters.reserve(seeds.size());

  SimSeedContainer outputSeeds;
  if (m_outputSeeds.isInitialized()) {
    outputSeeds.reserve(seeds.size());
  }

  const ProtoTrackContainer* inputTracks = nullptr;
  ProtoTrackContainer outputTracks;
  if (m_inputTracks.isInitialized() && m_outputTracks.isInitialized()) {
    const auto& inputTracksRef = m_inputTracks(ctx);
    if (seeds.size() != inputTracksRef.size()) {
      ACTS_FATAL("Inconsistent number of seeds and prototracks");
      return ProcessCode::ABORT;
    }
    inputTracks = &inputTracksRef;
    outputTracks.reserve(seeds.size());
  }

  auto bCache = m_cfg.magneticField->makeCache(ctx.magFieldContext);

  IndexSourceLink::SurfaceAccessor surfaceAccessor{*m_cfg.trackingGeometry};

  // Loop over all found seeds to estimate track parameters
  for (std::size_t iseed = 0; iseed < seeds.size(); ++iseed) {
    const auto& seed = seeds[iseed];
    // Get the bottom space point and its reference surface
    const auto& bottomSP = seed.sp().front();
    if (bottomSP->sourceLinks().empty()) {
      ACTS_WARNING("Missing source link in the space point");
      continue;
    }
    const auto& sourceLink = bottomSP->sourceLinks()[0];
    const Acts::Surface* surface = surfaceAccessor(sourceLink);

    if (surface == nullptr) {
      ACTS_WARNING(
          "Surface from source link is not found in the tracking geometry");
      continue;
    }

    // Get the magnetic field at the bottom space point
    auto fieldRes = m_cfg.magneticField->getField(
        {bottomSP->x(), bottomSP->y(), bottomSP->z()}, bCache);
    if (!fieldRes.ok()) {
      ACTS_ERROR("Field lookup error: " << fieldRes.error());
      return ProcessCode::ABORT;
    }
    Acts::Vector3 field = *fieldRes;

    // Estimate the track parameters from seed
    auto optParams = Acts::estimateTrackParamsFromSeed(
        ctx.geoContext, seed.sp().begin(), seed.sp().end(), *surface, field,
        m_cfg.bFieldMin, logger());
    if (!optParams.has_value()) {
      ACTS_WARNING("Estimation of track parameters for seed " << iseed
                                                              << " failed.");
      continue;
    }

    const auto& params = optParams.value();

    Acts::BoundSquareMatrix cov =
        makeInitialCovariance(m_cfg, params, *bottomSP);

    trackParameters.emplace_back(surface->getSharedPtr(), params, cov,
                                 m_cfg.particleHypothesis);
    if (m_outputSeeds.isInitialized()) {
      outputSeeds.push_back(seed);
    }
    if (m_outputTracks.isInitialized() && inputTracks != nullptr) {
      outputTracks.push_back(inputTracks->at(iseed));
    }
  }

  ACTS_VERBOSE("Estimated " << trackParameters.size() << " track parameters");

  m_outputTrackParameters(ctx, std::move(trackParameters));
  if (m_outputSeeds.isInitialized()) {
    m_outputSeeds(ctx, std::move(outputSeeds));
  }

  if (m_outputTracks.isInitialized()) {
    m_outputTracks(ctx, std::move(outputTracks));
  }

  return ProcessCode::SUCCESS;
}
}  // namespace ActsExamples
