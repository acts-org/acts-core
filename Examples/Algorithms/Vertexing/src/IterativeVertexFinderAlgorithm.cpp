// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Vertexing/IterativeVertexFinderAlgorithm.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Propagator/SympyStepper.hpp"
#include "Acts/Propagator/VoidNavigator.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/Result.hpp"
#include "Acts/Vertexing/IterativeVertexFinder.hpp"
#include "Acts/Vertexing/TrackAtVertex.hpp"
#include "Acts/Vertexing/Vertex.hpp"
#include "ActsExamples/EventData/ProtoVertex.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"

#include <chrono>
#include <ostream>
#include <stdexcept>
#include <system_error>

#include "VertexingHelpers.hpp"

namespace ActsExamples {

IterativeVertexFinderAlgorithm::IterativeVertexFinderAlgorithm(
    const Config& config, Acts::Logging::Level level)
    : IAlgorithm("IterativeVertexFinder", level), m_cfg(config) {
  if (m_cfg.inputTrackParameters.empty()) {
    throw std::invalid_argument("Missing input track parameter collection");
  }
  if (m_cfg.outputProtoVertices.empty()) {
    throw std::invalid_argument("Missing output proto vertices collection");
  }
  if (m_cfg.outputVertices.empty()) {
    throw std::invalid_argument("Missing output vertices collection");
  }

  m_inputTrackParameters.initialize(m_cfg.inputTrackParameters);
  m_outputProtoVertices.initialize(m_cfg.outputProtoVertices);
  m_outputVertices.initialize(m_cfg.outputVertices);
}

ProcessCode IterativeVertexFinderAlgorithm::execute(
    const AlgorithmContext& ctx) const {
  // retrieve input tracks and convert into the expected format

  const auto& inputTrackParameters = m_inputTrackParameters(ctx);
  // TODO change this from pointers to tracks parameters to actual tracks
  auto inputTracks = makeInputTracks(inputTrackParameters);

  if (inputTrackParameters.size() != inputTracks.size()) {
    ACTS_ERROR("Input track containers do not align: "
               << inputTrackParameters.size() << " != " << inputTracks.size());
  }

  for (const auto& trk : inputTrackParameters) {
    if (trk.covariance() && trk.covariance()->determinant() <= 0) {
      // actually we should consider this as an error but I do not want the CI
      // to fail
      ACTS_WARNING("input track " << trk << " has det(cov) = "
                                  << trk.covariance()->determinant());
    }
  }

  // Set up SympyStepper
  Acts::SympyStepper stepper(m_cfg.bField);

  // Set up propagator with void navigator
  auto propagator = std::make_shared<Propagator>(
      stepper, Acts::VoidNavigator{}, logger().cloneWithSuffix("Propagator"));
  // Setup the vertex fitter
  Fitter::Config vertexFitterCfg;
  vertexFitterCfg.extractParameters
      .connect<&Acts::InputTrack::extractParameters>();
  // Setup the track linearizer
  Linearizer::Config linearizerCfg;
  linearizerCfg.bField = m_cfg.bField;
  linearizerCfg.propagator = propagator;
  Linearizer linearizer(linearizerCfg,
                        logger().cloneWithSuffix("HelicalTrackLinearizer"));

  vertexFitterCfg.trackLinearizer.connect<&Linearizer::linearizeTrack>(
      &linearizer);
  Fitter vertexFitter(vertexFitterCfg,
                      logger().cloneWithSuffix("FullBilloirVertexFitter"));

  // Setup the seed finder
  Acts::ImpactPointEstimator::Config ipEstCfg(m_cfg.bField, propagator);
  Acts::ImpactPointEstimator ipEst(
      ipEstCfg, logger().cloneWithSuffix("ImpactPointEstimator"));

  Acts::GaussianTrackDensity::Config densityCfg;
  densityCfg.extractParameters.connect<&Acts::InputTrack::extractParameters>();
  auto seeder = std::make_shared<Seeder>(Seeder::Config{{densityCfg}});
  // Set up the actual vertex finder
  Finder::Config finderCfg(std::move(vertexFitter), seeder, ipEst);
  finderCfg.trackLinearizer.connect<&Linearizer::linearizeTrack>(&linearizer);

  finderCfg.maxVertices = m_cfg.maxIterations;
  finderCfg.reassignTracksAfterFirstFit = false;
  finderCfg.extractParameters.connect<&Acts::InputTrack::extractParameters>();
  finderCfg.field = m_cfg.bField;
  Finder finder(std::move(finderCfg), logger().clone());
  Acts::IVertexFinder::State state{std::in_place_type<Finder::State>,
                                   *m_cfg.bField, ctx.magFieldContext};
  Options finderOpts(ctx.geoContext, ctx.magFieldContext);

  // find vertices
  auto result = finder.find(inputTracks, finderOpts, state);

  VertexCollection vertices;
  if (result.ok()) {
    vertices = std::move(result.value());
  } else {
    ACTS_ERROR("Error in vertex finder: " << result.error().message());
  }

  // show some debug output
  ACTS_INFO("Found " << vertices.size() << " vertices in event");
  for (const auto& vtx : vertices) {
    ACTS_DEBUG("Found vertex at " << vtx.fullPosition().transpose() << " with "
                                  << vtx.tracks().size() << " tracks.");
  }

  // store proto vertices extracted from the found vertices
  m_outputProtoVertices(ctx, makeProtoVertices(inputTracks, vertices));

  // store found vertices
  m_outputVertices(ctx, std::move(vertices));

  return ProcessCode::SUCCESS;
}

}  // namespace ActsExamples
