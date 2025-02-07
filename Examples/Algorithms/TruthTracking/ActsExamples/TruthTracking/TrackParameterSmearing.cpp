// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/TruthTracking/TrackParameterSmearing.hpp"

#include "Acts/Seeding/EstimateTrackParamsFromSeed.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Framework/RandomNumbers.hpp"

#include <random>
#include <stdexcept>
#include <utility>

namespace ActsExamples {

TrackParameterSmearing::TrackParameterSmearing(const Config& config,
                                               Acts::Logging::Level level)
    : IAlgorithm("TrackParameterSmearing", level), m_cfg(config) {
  if (m_cfg.inputTrackParameters.empty()) {
    throw std::invalid_argument("Missing input track parameters collection");
  }
  if (m_cfg.outputTrackParameters.empty()) {
    throw std::invalid_argument("Missing output track parameters collection");
  }
  if (m_cfg.randomNumbers == nullptr) {
    throw std::invalid_argument("Missing random numbers tool");
  }

  if (m_cfg.particleHypothesis) {
    ACTS_INFO("Override truth particle hypothesis with "
              << *m_cfg.particleHypothesis);
  }

  m_inputTrackParameters.initialize(m_cfg.inputTrackParameters);
  m_outputTrackParameters.initialize(m_cfg.outputTrackParameters);

  ACTS_DEBUG("smearing track param loc0 " << m_cfg.sigmaLoc0 << " A "
                                          << m_cfg.sigmaLoc0PtA << " B "
                                          << m_cfg.sigmaLoc0PtB);
  ACTS_DEBUG("smearing track param loc1 " << m_cfg.sigmaLoc1 << " A "
                                          << m_cfg.sigmaLoc1PtA << " B "
                                          << m_cfg.sigmaLoc1PtB);
  ACTS_DEBUG("smearing track param time " << m_cfg.sigmaTime);
  ACTS_DEBUG("smearing track param phi " << m_cfg.sigmaPhi);
  ACTS_DEBUG("smearing track param theta " << m_cfg.sigmaTheta);
  ACTS_DEBUG("smearing track param q/p " << m_cfg.sigmaPtRel);
  ACTS_DEBUG(
      "initial sigmas "
      << Acts::BoundVector(
             m_cfg.initialSigmas.value_or(std::array<double, 6>()).data())
             .transpose());
  ACTS_DEBUG("initial sigma pt rel " << m_cfg.initialSigmaPtRel);
  ACTS_DEBUG(
      "initial var inflation "
      << Acts::BoundVector(m_cfg.initialVarInflation.data()).transpose());
  if (m_cfg.particleHypothesis) {
    ACTS_DEBUG("particle hypothesis " << *m_cfg.particleHypothesis);
  } else {
    ACTS_DEBUG("particle hypothesis truth");
  }
}

ProcessCode TrackParameterSmearing::execute(const AlgorithmContext& ctx) const {
  // setup input and output containers
  const auto& inputTrackParametersContainer = m_inputTrackParameters(ctx);

  ACTS_VERBOSE("Smearing " << inputTrackParametersContainer.size()
                           << " track parameters");

  TrackParametersContainer outputTrackParametersContainer;
  outputTrackParametersContainer.reserve(inputTrackParametersContainer.size());

  // setup random number generator and standard gaussian
  auto rng = m_cfg.randomNumbers->spawnGenerator(ctx);
  std::normal_distribution<double> stdNormal(0.0, 1.0);

  for (const auto& inputTrackParameters : inputTrackParametersContainer) {
    const auto position = inputTrackParameters.localPosition();
    const auto time = inputTrackParameters.time();
    const auto phi = inputTrackParameters.phi();
    const auto theta = inputTrackParameters.theta();
    const auto pt = inputTrackParameters.transverseMomentum();
    const auto qOverP = inputTrackParameters.qOverP();
    const auto particleHypothesis = m_cfg.particleHypothesis.value_or(
        inputTrackParameters.particleHypothesis());

    // compute momentum-dependent resolutions
    const double sigmaLoc0 =
        m_cfg.sigmaLoc0 +
        m_cfg.sigmaLoc0PtA * std::exp(-1.0 * std::abs(m_cfg.sigmaLoc0PtB) * pt);
    const double sigmaLoc1 =
        m_cfg.sigmaLoc1 +
        m_cfg.sigmaLoc1PtA * std::exp(-1.0 * std::abs(m_cfg.sigmaLoc1PtB) * pt);
    // shortcuts for other resolutions
    const double sigmaTime = m_cfg.sigmaTime;
    const double sigmaPhi = m_cfg.sigmaPhi;
    const double sigmaTheta = m_cfg.sigmaTheta;
    const double sigmaQOverP =
        std::sqrt(std::pow(m_cfg.sigmaPtRel * qOverP, 2) +
                  std::pow(sigmaTheta * (qOverP * std::tan(theta)), 2));

    Acts::BoundVector params = Acts::BoundVector::Zero();
    // smear the position/time
    // note that we smear d0 and z0 in the perigee frame
    params[Acts::eBoundLoc0] = position[0] + sigmaLoc0 * stdNormal(rng);
    params[Acts::eBoundLoc1] = position[1] + sigmaLoc1 * stdNormal(rng);
    params[Acts::eBoundTime] = time + sigmaTime * stdNormal(rng);
    // smear direction angles phi,theta ensuring correct bounds
    const auto [newPhi, newTheta] = Acts::detail::normalizePhiTheta(
        phi + sigmaPhi * stdNormal(rng), theta + sigmaTheta * stdNormal(rng));
    params[Acts::eBoundPhi] = newPhi;
    params[Acts::eBoundTheta] = newTheta;
    // compute smeared q/p
    params[Acts::eBoundQOverP] = qOverP + sigmaQOverP * stdNormal(rng);

    // build the track covariance matrix using the smearing sigmas
    Acts::BoundSquareMatrix cov = Acts::BoundSquareMatrix::Zero();
    if (m_cfg.initialSigmas) {
      // use the initial sigmas if set

      Acts::EstimateTrackParamCovarianceConfig config{
          .initialSigmas =
              Eigen::Map<const Acts::BoundVector>{m_cfg.initialSigmas->data()},
          .initialSigmaPtRel = m_cfg.initialSigmaPtRel,
          .initialVarInflation = Eigen::Map<const Acts::BoundVector>{
              m_cfg.initialVarInflation.data()}};

      cov = Acts::estimateTrackParamCovariance(config, params, false);
    } else {
      // otherwise use the smearing sigmas

      Acts::BoundVector sigmas = Acts::BoundVector(
          {sigmaLoc0, sigmaLoc1, sigmaPhi, sigmaTheta, sigmaQOverP, sigmaTime});

      for (std::size_t i = Acts::eBoundLoc0; i < Acts::eBoundSize; ++i) {
        double sigma = sigmas[i];
        double variance = sigma * sigma;

        // Inflate the initial covariance
        variance *= m_cfg.initialVarInflation[i];

        cov(i, i) = variance;
      }
    }

    const auto& outputTrackParameters =
        outputTrackParametersContainer.emplace_back(
            inputTrackParameters.referenceSurface().shared_from_this(), params,
            cov, particleHypothesis);

    ACTS_VERBOSE("Smearing particle (pos, time, phi, theta, q/p):");
    ACTS_VERBOSE(
        " from: " << inputTrackParameters.position(ctx.geoContext).transpose()
                  << ", " << time << ", " << phi << ", " << theta << ", "
                  << qOverP);
    ACTS_VERBOSE(
        "   to: " << outputTrackParameters.position(ctx.geoContext).transpose()
                  << ", " << params[Acts::eBoundTime] << ", "
                  << params[Acts::eBoundPhi] << ", "
                  << params[Acts::eBoundTheta] << ", "
                  << params[Acts::eBoundQOverP]);
  }

  m_outputTrackParameters(ctx, std::move(outputTrackParametersContainer));

  return ProcessCode::SUCCESS;
}

}  // namespace ActsExamples
