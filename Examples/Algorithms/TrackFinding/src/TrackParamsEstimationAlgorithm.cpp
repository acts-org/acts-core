// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/TrackFinding/TrackParamsEstimationAlgorithm.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/Seed.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/MagneticField/MagneticFieldProvider.hpp"
#include "Acts/Seeding/EstimateTrackParamsFromSeed.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "ActsExamples/EventData/IndexSourceLink.hpp"
#include "ActsExamples/EventData/SimSpacePoint.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"

#include <cstddef>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace ActsExamples {

TrackParamsEstimationAlgorithm::TrackParamsEstimationAlgorithm(
    TrackParamsEstimationAlgorithm::Config cfg, Acts::Logging::Level lvl)
    : IAlgorithm("TrackParamsEstimationAlgorithm", lvl), m_cfg(std::move(cfg)) {
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

ProcessCode TrackParamsEstimationAlgorithm::execute(
    const AlgorithmContext& ctx) const {
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

    if (field.norm() < m_cfg.bFieldMin) {
      ACTS_WARNING("Magnetic field at seed " << iseed << " is too small "
                                             << field.norm());
      continue;
    }

    // Estimate the track parameters from seed
    const auto paramsResult = Acts::estimateTrackParamsFromSeed(
        ctx.geoContext, seed.sp(), *surface, field);
    if (!paramsResult.ok()) {
      ACTS_WARNING("Skip track because param estimation failed "
                   << paramsResult.error());
      continue;
    }
    const auto& params = *paramsResult;

    Acts::EstimateTrackParamCovarianceConfig config{
        .initialSigmas =
            Eigen::Map<const Acts::BoundVector>{m_cfg.initialSigmas.data()},
        .initialSigmaPtRel = m_cfg.initialSigmaPtRel,
        .initialVarInflation = Eigen::Map<const Acts::BoundVector>{
            m_cfg.initialVarInflation.data()}};

    Acts::BoundSquareMatrix cov = Acts::estimateTrackParamCovariance(
        config, params, bottomSP->t().has_value());

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
