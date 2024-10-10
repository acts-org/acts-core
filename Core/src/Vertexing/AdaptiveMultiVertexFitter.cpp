// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Vertexing/AdaptiveMultiVertexFitter.hpp"

#include "Acts/Surfaces/PerigeeSurface.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "Acts/Vertexing/KalmanVertexUpdater.hpp"
#include "Acts/Vertexing/VertexingError.hpp"

Acts::Result<void> Acts::AdaptiveMultiVertexFitter::fit(
    State& state, const VertexingOptions& vertexingOptions) const {
  // Reset annealing tool
  state.annealingState = AnnealingUtility::State();

  // Boolean indicating whether any of the vertices has moved more than
  // m_cfg.maxRelativeShift during the last iteration. We will keep iterating
  // until the equilibrium (i.e., the lowest temperature) is reached in
  // the annealing procedure and isSmallShift is true (or until the maximum
  // number of iterations is exceeded).
  bool isSmallShift = true;

  // Number of iterations counter
  unsigned int nIter = 0;

  // Start iterating
  while (nIter < m_cfg.maxIterations &&
         (!state.annealingState.equilibriumReached || !isSmallShift)) {
    // Initial loop over all vertices in state.vertexCollection
    for (auto vtx : state.vertexCollection) {
      VertexInfo& vtxInfo = state.vtxInfoMap[vtx];
      vtxInfo.relinearize = false;
      // Store old position of vertex, i.e. seed position
      // in case of first iteration or position determined
      // in previous iteration afterwards
      vtxInfo.oldPosition = vtx->fullPosition();

      // Calculate the x-y-distance between the current vertex position
      // and the linearization point of the tracks. If it is too large,
      // we relinearize the tracks and recalculate their 3D impact
      // parameters.
      ActsVector<2> xyDiff = vtxInfo.oldPosition.template head<2>() -
                             vtxInfo.linPoint.template head<2>();
      if (xyDiff.norm() > m_cfg.maxDistToLinPoint) {
        // Set flag for relinearization
        vtxInfo.relinearize = true;
        // Recalculate the track impact parameters at the current vertex
        // position
        auto prepareVertexResult =
            prepareVertexForFit(state, vtx, vertexingOptions);
        if (!prepareVertexResult.ok()) {
          // Print vertices and associated tracks if logger is in debug mode
          if (logger().doPrint(Logging::DEBUG)) {
            logDebugData(state, vertexingOptions.geoContext);
          }
          return prepareVertexResult.error();
        }
      }

      // Check if we use the constraint during the vertex fit
      if (state.vtxInfoMap[vtx].constraint.fullCovariance() !=
          SquareMatrix4::Zero()) {
        const Acts::Vertex& constraint = state.vtxInfoMap[vtx].constraint;
        vtx->setFullPosition(constraint.fullPosition());
        vtx->setFitQuality(constraint.fitQuality());
        vtx->setFullCovariance(constraint.fullCovariance());
      } else if (vtx->fullCovariance() == SquareMatrix4::Zero()) {
        return VertexingError::NoCovariance;
      }

      // Set vertexCompatibility for all TrackAtVertex objects
      // at the current vertex
      auto setCompatibilitiesResult =
          setAllVertexCompatibilities(state, vtx, vertexingOptions);
      if (!setCompatibilitiesResult.ok()) {
        // Print vertices and associated tracks if logger is in debug mode
        if (logger().doPrint(Logging::DEBUG)) {
          logDebugData(state, vertexingOptions.geoContext);
        }
        return setCompatibilitiesResult.error();
      }
    }  // End loop over vertex collection

    // Recalculate all track weights and update vertices
    auto setWeightsResult = setWeightsAndUpdate(state, vertexingOptions);
    if (!setWeightsResult.ok()) {
      // Print vertices and associated tracks if logger is in debug mode
      if (logger().doPrint(Logging::DEBUG)) {
        logDebugData(state, vertexingOptions.geoContext);
      }
      return setWeightsResult.error();
    }

    // Cool the system down, i.e., reduce the temperature parameter. At lower
    // temperatures, outlying tracks are downweighted more.
    if (!state.annealingState.equilibriumReached) {
      m_cfg.annealingTool.anneal(state.annealingState);
    }

    isSmallShift = checkSmallShift(state);
    ++nIter;
  }
  // Multivertex fit is finished

  // Check if smoothing is required
  if (m_cfg.doSmoothing) {
    doVertexSmoothing(state);
  }

  return {};
}

Acts::Result<void> Acts::AdaptiveMultiVertexFitter::addVtxToFit(
    State& state, Vertex& newVertex,
    const VertexingOptions& vertexingOptions) const {
  if (state.vtxInfoMap[&newVertex].trackLinks.empty()) {
    ACTS_ERROR(
        "newVertex does not have any associated tracks (i.e., its trackLinks "
        "are empty).");
    return VertexingError::EmptyInput;
  }

  std::vector<Vertex*> verticesToFit = {&newVertex};

  // List of vertices added in last iteration
  std::vector<Vertex*> lastIterAddedVertices = {&newVertex};
  // List of vertices added in current iteration
  std::vector<Vertex*> currentIterAddedVertices;

  // Fill verticesToFit with vertices that are connected to newVertex (via
  // tracks and/or other vertices).
  while (!lastIterAddedVertices.empty()) {
    for (auto& lastIterAddedVertex : lastIterAddedVertices) {
      // Loop over all tracks at lastIterAddedVertex
      const std::vector<InputTrack>& trks =
          state.vtxInfoMap[lastIterAddedVertex].trackLinks;
      for (const auto& trk : trks) {
        // Range of vertices that are associated with trk. The range is
        // represented via its bounds: begin refers to the first iterator of the
        // range; end refers to the iterator after the last iterator of the
        // range.
        auto [begin, end] = state.trackToVerticesMultiMap.equal_range(trk);

        for (auto it = begin; it != end; ++it) {
          // it->first corresponds to trk, it->second to one of its associated
          // vertices
          auto vtxToFit = it->second;
          // Add vertex to the fit if it is not already included
          if (!isAlreadyInList(vtxToFit, verticesToFit)) {
            verticesToFit.push_back(vtxToFit);

            // Collect vertices that were added this iteration
            if (vtxToFit != lastIterAddedVertex) {
              currentIterAddedVertices.push_back(vtxToFit);
            }
          }
        }  // End for loop over range of associated vertices
      }  // End loop over trackLinks
    }  // End loop over lastIterAddedVertices

    lastIterAddedVertices = currentIterAddedVertices;
    currentIterAddedVertices.clear();
  }  // End while loop

  state.vertexCollection = verticesToFit;

  // Save the 3D impact parameters of all tracks associated with newVertex.
  auto res = prepareVertexForFit(state, &newVertex, vertexingOptions);
  if (!res.ok()) {
    // Print vertices and associated tracks if logger is in debug mode
    if (logger().doPrint(Logging::DEBUG)) {
      logDebugData(state, vertexingOptions.geoContext);
    }
    return res.error();
  }

  // Perform fit on all added vertices
  auto fitRes = fit(state, vertexingOptions);
  if (!fitRes.ok()) {
    return fitRes.error();
  }

  return {};
}

bool Acts::AdaptiveMultiVertexFitter::isAlreadyInList(
    Vertex* vtx, const std::vector<Vertex*>& vertices) const {
  return rangeContainsValue(vertices, vtx);
}

Acts::Result<void> Acts::AdaptiveMultiVertexFitter::prepareVertexForFit(
    State& state, Vertex* vtx, const VertexingOptions& vertexingOptions) const {
  // Vertex info object
  auto& vtxInfo = state.vtxInfoMap[vtx];
  // Vertex seed position
  const Vector3& seedPos = vtxInfo.seedPosition.template head<3>();

  // Loop over all tracks at the vertex
  for (const auto& trk : vtxInfo.trackLinks) {
    auto res = m_cfg.ipEst.estimate3DImpactParameters(
        vertexingOptions.geoContext, vertexingOptions.magFieldContext,
        m_cfg.extractParameters(trk), seedPos, state.ipState);
    if (!res.ok()) {
      return res.error();
    }
    // Save 3D impact parameters of the track
    vtxInfo.impactParams3D.emplace(trk, res.value());
  }
  return {};
}

Acts::Result<void> Acts::AdaptiveMultiVertexFitter::setAllVertexCompatibilities(
    State& state, Vertex* vtx, const VertexingOptions& vertexingOptions) const {
  VertexInfo& vtxInfo = state.vtxInfoMap[vtx];

  // Loop over all tracks that are associated with vtx and estimate their
  // compatibility
  for (const auto& trk : vtxInfo.trackLinks) {
    auto& trkAtVtx = state.tracksAtVerticesMap.at(std::make_pair(trk, vtx));
    // Recover from cases where linearization point != 0 but
    // more tracks were added later on
    if (!vtxInfo.impactParams3D.contains(trk)) {
      auto res = m_cfg.ipEst.estimate3DImpactParameters(
          vertexingOptions.geoContext, vertexingOptions.magFieldContext,
          m_cfg.extractParameters(trk),
          VectorHelpers::position(vtxInfo.linPoint), state.ipState);
      if (!res.ok()) {
        return res.error();
      }
      // Set impactParams3D for current trackAtVertex
      vtxInfo.impactParams3D.emplace(trk, res.value());
    }
    // Set compatibility with current vertex
    Acts::Result<double> compatibilityResult(0.);
    if (m_cfg.useTime) {
      compatibilityResult = m_cfg.ipEst.getVertexCompatibility(
          vertexingOptions.geoContext, &(vtxInfo.impactParams3D.at(trk)),
          vtxInfo.oldPosition);
    } else {
      Acts::Vector3 vertexPosOnly =
          VectorHelpers::position(vtxInfo.oldPosition);
      compatibilityResult = m_cfg.ipEst.getVertexCompatibility(
          vertexingOptions.geoContext, &(vtxInfo.impactParams3D.at(trk)),
          vertexPosOnly);
    }

    if (!compatibilityResult.ok()) {
      return compatibilityResult.error();
    }
    trkAtVtx.vertexCompatibility = *compatibilityResult;
  }
  return {};
}

Acts::Result<void> Acts::AdaptiveMultiVertexFitter::setWeightsAndUpdate(
    State& state, const VertexingOptions& vertexingOptions) const {
  for (auto vtx : state.vertexCollection) {
    VertexInfo& vtxInfo = state.vtxInfoMap[vtx];

    if (vtxInfo.relinearize) {
      vtxInfo.linPoint = vtxInfo.oldPosition;
    }

    const std::shared_ptr<PerigeeSurface> vtxPerigeeSurface =
        Surface::makeShared<PerigeeSurface>(
            VectorHelpers::position(vtxInfo.linPoint));

    for (const auto& trk : vtxInfo.trackLinks) {
      auto& trkAtVtx = state.tracksAtVerticesMap.at(std::make_pair(trk, vtx));

      // Set trackWeight for current track
      trkAtVtx.trackWeight = m_cfg.annealingTool.getWeight(
          state.annealingState, trkAtVtx.vertexCompatibility,
          collectTrackToVertexCompatibilities(state, trk));

      if (trkAtVtx.trackWeight > m_cfg.minWeight) {
        // Check if track is already linearized and whether we need to
        // relinearize
        if (!trkAtVtx.isLinearized || vtxInfo.relinearize) {
          auto result = m_cfg.trackLinearizer(
              m_cfg.extractParameters(trk), vtxInfo.linPoint[3],
              *vtxPerigeeSurface, vertexingOptions.geoContext,
              vertexingOptions.magFieldContext, state.fieldCache);
          if (!result.ok()) {
            return result.error();
          }

          trkAtVtx.linearizedState = *result;
          trkAtVtx.isLinearized = true;
        }
        // Update the vertex with the new track. The second template
        // argument corresponds to the number of fitted vertex dimensions
        // (i.e., 3 if we only fit spatial coordinates and 4 if we also fit
        // time).
        KalmanVertexUpdater::updateVertexWithTrack(*vtx, trkAtVtx,
                                                   m_cfg.useTime ? 4 : 3);
      } else {
        ACTS_VERBOSE("Track weight too low. Skip track.");
      }
    }  // End loop over tracks at vertex
    ACTS_VERBOSE("New vertex position: " << vtx->fullPosition().transpose());
  }  // End loop over vertex collection

  return {};
}

std::vector<double>
Acts::AdaptiveMultiVertexFitter::collectTrackToVertexCompatibilities(
    State& state, const InputTrack& trk) const {
  // Compatibilities of trk wrt all of its associated vertices
  std::vector<double> trkToVtxCompatibilities;

  // Range of vertices that are associated with trk. The range is
  // represented via its bounds: begin refers to the first iterator of the
  // range; end refers to the iterator after the last iterator of the range.
  auto [begin, end] = state.trackToVerticesMultiMap.equal_range(trk);
  // Allocate space in memory for the vector of compatibilities
  trkToVtxCompatibilities.reserve(std::distance(begin, end));

  for (auto it = begin; it != end; ++it) {
    // it->first corresponds to trk, it->second to one of its associated
    // vertices
    trkToVtxCompatibilities.push_back(
        state.tracksAtVerticesMap.at(std::make_pair(trk, it->second))
            .vertexCompatibility);
  }

  return trkToVtxCompatibilities;
}

bool Acts::AdaptiveMultiVertexFitter::checkSmallShift(State& state) const {
  for (auto* vtx : state.vertexCollection) {
    Vector3 diff =
        state.vtxInfoMap[vtx].oldPosition.template head<3>() - vtx->position();
    const SquareMatrix3& vtxCov = vtx->covariance();
    double relativeShift = diff.dot(vtxCov.inverse() * diff);
    if (relativeShift > m_cfg.maxRelativeShift) {
      return false;
    }
  }
  return true;
}

void Acts::AdaptiveMultiVertexFitter::doVertexSmoothing(State& state) const {
  for (const auto vtx : state.vertexCollection) {
    for (const auto& trk : state.vtxInfoMap[vtx].trackLinks) {
      auto& trkAtVtx = state.tracksAtVerticesMap.at(std::make_pair(trk, vtx));
      if (trkAtVtx.trackWeight > m_cfg.minWeight) {
        // Update the new track under the assumption that it originates at the
        // vertex. The second template argument corresponds to the number of
        // fitted vertex dimensions (i.e., 3 if we only fit spatial coordinates
        // and 4 if we also fit time).
        KalmanVertexUpdater::updateTrackWithVertex(trkAtVtx, *vtx,
                                                   m_cfg.useTime ? 4 : 3);
      }
    }
  }
}

void Acts::AdaptiveMultiVertexFitter::logDebugData(
    const State& state, const Acts::GeometryContext& geoContext) const {
  ACTS_DEBUG("Encountered an error when fitting the following "
             << state.vertexCollection.size() << " vertices:");
  for (std::size_t vtxInd = 0; vtxInd < state.vertexCollection.size();
       ++vtxInd) {
    auto vtx = state.vertexCollection[vtxInd];
    ACTS_DEBUG("Position of " << vtxInd << ". vertex seed:\n"
                              << state.vtxInfoMap.at(vtx).seedPosition);
    ACTS_DEBUG("Position of said vertex after the last fitting step:\n"
               << state.vtxInfoMap.at(vtx).oldPosition);
    ACTS_DEBUG("Associated tracks:");
    const auto& trks = state.vtxInfoMap.at(vtx).trackLinks;
    for (std::size_t trkInd = 0; trkInd < trks.size(); ++trkInd) {
      const auto& trkAtVtx =
          state.tracksAtVerticesMap.at(std::make_pair(trks[trkInd], vtx));
      const auto& trkParams = m_cfg.extractParameters(trkAtVtx.originalParams);
      ACTS_DEBUG(trkInd << ". track parameters:\n" << trkParams.parameters());
      ACTS_DEBUG(trkInd << ". track covariance matrix:\n"
                        << trkParams.covariance().value());
      ACTS_DEBUG("Origin of corresponding reference surface:\n"
                 << trkParams.referenceSurface().center(geoContext));
    }
  }
}
