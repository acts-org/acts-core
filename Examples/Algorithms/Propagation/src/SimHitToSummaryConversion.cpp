// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Propagation/SimHitToSummaryConversion.hpp"

#include "ActsExamples/EventData/GeometryContainers.hpp"

namespace {
/// Helper method to cancatenate the steps and push them onto the surface
///
/// @param gctx is the geometry context
/// @param steps is the vector of steps to concatenate
/// @param surface is the surface to push the steps onto
///
/// @return the concatenated step
Acts::detail::Step concatenateSteps(
    const Acts::GeometryContext& gctx,
    const std::vector<Acts::detail::Step>& steps,
    const Acts::Surface& surface) {
  // Average the position and direction
  Acts::detail::Step concatStep;
  if (steps.size() > 1) {
    for (const Acts::detail::Step& step : steps) {
      concatStep.position += step.position;
      concatStep.momentum += step.momentum;
    }
    Acts::ActsScalar weight = 1.0 / static_cast<Acts::ActsScalar>(steps.size());
    concatStep.position *= weight;
    concatStep.momentum *= weight;
  } else {
    concatStep = steps.front();
  }
  // Re-evaulate the position with a surface intersection
  auto intersection =
      surface.intersect(gctx, concatStep.position, concatStep.momentum);
  for (const auto& rsIntersection : intersection.split()) {
    if (rsIntersection.isValid()) {
      concatStep.position = rsIntersection.position();
      break;
    }
  }
  // Set the surface identifier
  concatStep.geoID = surface.geometryId();
  return concatStep;
}
}  // namespace

ActsExamples::SimHitToSummaryConversion::SimHitToSummaryConversion(
    const Config& config, Acts::Logging::Level level)
    : IAlgorithm("SimHitToSummaryConversion", level), m_cfg(config) {
  if (m_cfg.inputSimHits.empty()) {
    throw std::invalid_argument("Missing simulated hits input collection");
  }
  if (m_cfg.inputParticles.empty()) {
    throw std::invalid_argument("Missing simulated particles input collection");
  }
  m_inputSimHits.initialize(m_cfg.inputSimHits);
  m_inputParticles.initialize(m_cfg.inputParticles);
  m_outputSummary.initialize(m_cfg.outputSummaryCollection);
}

ActsExamples::ProcessCode ActsExamples::SimHitToSummaryConversion::execute(
    const AlgorithmContext& context) const {
  // Retrieve the simulated hits
  const auto& simHits = m_inputSimHits(context);
  // Retrieve the particles at its basis
  const auto& particles = m_inputParticles(context);

  PropagationSummaries propagationSummaries;
  propagationSummaries.reserve(particles.size());

  // Prepare and sort
  std::unordered_map<unsigned long,
                     std::vector<std::vector<Acts::detail::Step>>>
      trackSteps;
  for (const auto& simHitsGroup : groupByModule(simHits)) {
    // Manual pair unpacking instead of using
    //   auto [moduleGeoId, moduleSimHits] : ...
    // otherwise clang on macos complains that it is unable to capture the local
    // binding in the lambda used for visiting the smearer below.
    Acts::GeometryIdentifier moduleGeoId = simHitsGroup.first;
    const auto& moduleSimHits = simHitsGroup.second;
    std::unordered_map<unsigned long, std::vector<Acts::detail::Step>>
        moduleSteps;
    for (const auto& simHit : moduleSimHits) {
      unsigned long paritcleId = simHit.particleId().value();
      if (!moduleSteps.contains(paritcleId)) {
        moduleSteps[paritcleId] = std::vector<Acts::detail::Step>();
      }
      Acts::ActsScalar hx = simHit.fourPosition().x() / Acts::UnitConstants::mm;
      Acts::ActsScalar hy = simHit.fourPosition().y() / Acts::UnitConstants::mm;
      Acts::ActsScalar hz = simHit.fourPosition().z() / Acts::UnitConstants::mm;
      Acts::detail::Step step;
      step.position = Acts::Vector3(hx, hy, hz);
      step.momentum = simHit.direction();
      step.geoID = moduleGeoId;
      step.navDir = Acts::Direction::Forward;
      moduleSteps[paritcleId].push_back(step);
    }
    // Loop over and fill into the trackSteps
    for (const auto& [particleId, steps] : moduleSteps) {
      if (!trackSteps.contains(particleId)) {
        trackSteps[particleId] = std::vector<std::vector<Acts::detail::Step>>();
      }
      trackSteps[particleId].push_back(steps);
    }
  }

  // Loop over the particles and create the propagation summaries
  for (const auto& particle : particles) {
    // Create the propagation summary
    Acts::CurvilinearTrackParameters start(
        particle.fourPosition(), particle.direction(),
        particle.charge() / particle.momentum().norm(), std::nullopt,
        particle.hypothesis());
    PropagationSummary propagationSummary(start);
    // Find the associated steps
    auto steps = trackSteps.find(particle.particleId().value());
    if (steps != trackSteps.end()) {
      for (const std::vector<Acts::detail::Step>& moduleSteps : steps->second) {
        // Get the GeometryIdentifier of the surface
        Acts::GeometryIdentifier surface = moduleSteps.front().geoID;
        // Find the surface
        auto surfaceIt = m_cfg.surfaceByIdentifier.find(surface);
        if (surfaceIt == m_cfg.surfaceByIdentifier.end()) {
          throw std::invalid_argument("Surface not found, should not happen");
        }
        Acts::detail::Step concatStep = concatenateSteps(
            context.geoContext, moduleSteps, *surfaceIt->second);
        propagationSummary.steps.push_back(concatStep);
      }
    } else {
      ACTS_WARNING("No steps found for particle "
                   << particle.particleId().value());
    }

    propagationSummaries.push_back(std::move(propagationSummary));
  }

  // Write the propagation step data to the event store
  m_outputSummary(context, std::move(propagationSummaries));

  return ProcessCode::SUCCESS;
}
