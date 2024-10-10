// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Vertexing/IterativeVertexFinder.hpp"

#include "Acts/Surfaces/PerigeeSurface.hpp"
#include "Acts/Vertexing/VertexingError.hpp"

Acts::IterativeVertexFinder::IterativeVertexFinder(
    Config cfg, std::unique_ptr<const Logger> logger)
    : m_cfg(std::move(cfg)), m_logger(std::move(logger)) {
  if (!m_cfg.extractParameters.connected()) {
    throw std::invalid_argument(
        "IterativeVertexFinder: "
        "No function to extract parameters "
        "provided.");
  }

  if (!m_cfg.trackLinearizer.connected()) {
    throw std::invalid_argument(
        "IterativeVertexFinder: "
        "No track linearizer provided.");
  }

  if (!m_cfg.seedFinder) {
    throw std::invalid_argument(
        "IterativeVertexFinder: "
        "No seed finder provided.");
  }

  if (!m_cfg.field) {
    throw std::invalid_argument(
        "IterativeVertexFinder: "
        "No magnetic field provider provided.");
  }
}

auto Acts::IterativeVertexFinder::find(
    const std::vector<InputTrack>& trackVector,
    const VertexingOptions& vertexingOptions,
    IVertexFinder::State& anyState) const -> Result<std::vector<Vertex>> {
  auto& state = anyState.as<State>();
  // Original tracks
  const std::vector<InputTrack>& origTracks = trackVector;
  // Tracks for seeding
  std::vector<InputTrack> seedTracks = trackVector;

  // List of vertices to be filled below
  std::vector<Vertex> vertexCollection;

  int nInterations = 0;
  // begin iterating
  while (seedTracks.size() > 1 && nInterations < m_cfg.maxVertices) {
    /// Do seeding
    auto seedRes = getVertexSeed(state, seedTracks, vertexingOptions);

    if (!seedRes.ok()) {
      return seedRes.error();
    }
    const auto& seedOptional = *seedRes;

    if (!seedOptional.has_value()) {
      ACTS_DEBUG("No more seed found. Break and stop primary vertex finding.");
      break;
    }
    const auto& seedVertex = *seedOptional;

    /// End seeding
    /// Now take only tracks compatible with current seed
    // Tracks used for the fit in this iteration
    std::vector<InputTrack> tracksToFit;
    std::vector<InputTrack> tracksToFitSplitVertex;

    // Fill vector with tracks to fit, only compatible with seed:
    auto res = fillTracksToFit(seedTracks, seedVertex, tracksToFit,
                               tracksToFitSplitVertex, vertexingOptions, state);

    if (!res.ok()) {
      return res.error();
    }

    ACTS_DEBUG("Number of tracks used for fit: " << tracksToFit.size());

    /// Begin vertex fit
    Vertex currentVertex;
    Vertex currentSplitVertex;

    if (vertexingOptions.useConstraintInFit && !tracksToFit.empty()) {
      auto fitResult = m_cfg.vertexFitter.fit(tracksToFit, vertexingOptions,
                                              state.fieldCache);
      if (fitResult.ok()) {
        currentVertex = std::move(*fitResult);
      } else {
        return fitResult.error();
      }
    } else if (!vertexingOptions.useConstraintInFit && tracksToFit.size() > 1) {
      auto fitResult = m_cfg.vertexFitter.fit(tracksToFit, vertexingOptions,
                                              state.fieldCache);
      if (fitResult.ok()) {
        currentVertex = std::move(*fitResult);
      } else {
        return fitResult.error();
      }
    }
    if (m_cfg.createSplitVertices && tracksToFitSplitVertex.size() > 1) {
      auto fitResult = m_cfg.vertexFitter.fit(
          tracksToFitSplitVertex, vertexingOptions, state.fieldCache);
      if (fitResult.ok()) {
        currentSplitVertex = std::move(*fitResult);
      } else {
        return fitResult.error();
      }
    }
    /// End vertex fit
    ACTS_DEBUG("Vertex position after fit: "
               << currentVertex.fullPosition().transpose());

    // Number degrees of freedom
    double ndf = currentVertex.fitQuality().second;
    double ndfSplitVertex = currentSplitVertex.fitQuality().second;

    // Number of significant tracks
    int nTracksAtVertex = countSignificantTracks(currentVertex);
    int nTracksAtSplitVertex = countSignificantTracks(currentSplitVertex);

    bool isGoodVertex = ((!vertexingOptions.useConstraintInFit && ndf > 0 &&
                          nTracksAtVertex >= 2) ||
                         (vertexingOptions.useConstraintInFit && ndf > 3 &&
                          nTracksAtVertex >= 2));

    if (!isGoodVertex) {
      removeTracks(tracksToFit, seedTracks);
    } else {
      if (m_cfg.reassignTracksAfterFirstFit && (!m_cfg.createSplitVertices)) {
        // vertex is good vertex here
        // but add tracks which may have been missed

        auto result = reassignTracksToNewVertex(
            vertexCollection, currentVertex, tracksToFit, seedTracks,
            origTracks, vertexingOptions, state);
        if (!result.ok()) {
          return result.error();
        }
        isGoodVertex = *result;

      }  // end reassignTracksAfterFirstFit case
         // still good vertex? might have changed in the meanwhile
      if (isGoodVertex) {
        removeUsedCompatibleTracks(currentVertex, tracksToFit, seedTracks,
                                   vertexingOptions, state);

        ACTS_DEBUG(
            "Number of seed tracks after removal of compatible tracks "
            "and outliers: "
            << seedTracks.size());
      }
    }  // end case if good vertex

    // now splitvertex
    bool isGoodSplitVertex = false;
    if (m_cfg.createSplitVertices) {
      isGoodSplitVertex = (ndfSplitVertex > 0 && nTracksAtSplitVertex >= 2);

      if (!isGoodSplitVertex) {
        removeTracks(tracksToFitSplitVertex, seedTracks);
      } else {
        removeUsedCompatibleTracks(currentSplitVertex, tracksToFitSplitVertex,
                                   seedTracks, vertexingOptions, state);
      }
    }
    // Now fill vertex collection with vertex
    if (isGoodVertex) {
      vertexCollection.push_back(currentVertex);
    }
    if (isGoodSplitVertex && m_cfg.createSplitVertices) {
      vertexCollection.push_back(currentSplitVertex);
    }

    nInterations++;
  }  // end while loop

  return vertexCollection;
}

auto Acts::IterativeVertexFinder::getVertexSeed(
    State& state, const std::vector<InputTrack>& seedTracks,
    const VertexingOptions& vertexingOptions) const
    -> Result<std::optional<Vertex>> {
  auto finderState = m_cfg.seedFinder->makeState(state.magContext);
  auto res = m_cfg.seedFinder->find(seedTracks, vertexingOptions, finderState);

  if (!res.ok()) {
    ACTS_ERROR("Internal seeding error. Number of input tracks: "
               << seedTracks.size());
    return VertexingError::SeedingError;
  }
  const auto& seedVector = *res;

  ACTS_DEBUG("Found " << seedVector.size() << " seeds");

  if (seedVector.empty()) {
    return std::nullopt;
  }
  const Vertex& seedVertex = seedVector.back();

  ACTS_DEBUG("Use " << seedTracks.size() << " tracks for vertex seed finding.");
  ACTS_DEBUG(
      "Found seed at position: " << seedVertex.fullPosition().transpose());

  return seedVertex;
}

inline void Acts::IterativeVertexFinder::removeTracks(
    const std::vector<InputTrack>& tracksToRemove,
    std::vector<InputTrack>& seedTracks) const {
  for (const auto& trk : tracksToRemove) {
    const BoundTrackParameters& params = m_cfg.extractParameters(trk);
    // Find track in seedTracks
    auto foundIter =
        std::ranges::find_if(seedTracks, [&params, this](const auto seedTrk) {
          return params == m_cfg.extractParameters(seedTrk);
        });
    if (foundIter != seedTracks.end()) {
      // Remove track from seed tracks
      seedTracks.erase(foundIter);
    } else {
      ACTS_WARNING("Track to be removed not found in seed tracks.");
    }
  }
}

Acts::Result<double> Acts::IterativeVertexFinder::getCompatibility(
    const BoundTrackParameters& params, const Vertex& vertex,
    const Surface& perigeeSurface, const VertexingOptions& vertexingOptions,
    State& state) const {
  // Linearize track
  auto result =
      m_cfg.trackLinearizer(params, vertex.fullPosition()[3], perigeeSurface,
                            vertexingOptions.geoContext,
                            vertexingOptions.magFieldContext, state.fieldCache);
  if (!result.ok()) {
    return result.error();
  }

  auto linTrack = std::move(*result);

  // Calculate reduced weight
  SquareMatrix2 weightReduced =
      linTrack.covarianceAtPCA.template block<2, 2>(0, 0);

  SquareMatrix2 errorVertexReduced =
      (linTrack.positionJacobian *
       (vertex.fullCovariance() * linTrack.positionJacobian.transpose()))
          .template block<2, 2>(0, 0);
  weightReduced += errorVertexReduced;
  weightReduced = weightReduced.inverse().eval();

  // Calculate compatibility / chi2
  Vector2 trackParameters2D =
      linTrack.parametersAtPCA.template block<2, 1>(0, 0);
  double compatibility =
      trackParameters2D.dot(weightReduced * trackParameters2D);

  return compatibility;
}

Acts::Result<void> Acts::IterativeVertexFinder::removeUsedCompatibleTracks(
    Vertex& vertex, std::vector<InputTrack>& tracksToFit,
    std::vector<InputTrack>& seedTracks,
    const VertexingOptions& vertexingOptions, State& state) const {
  std::vector<TrackAtVertex> tracksAtVertex = vertex.tracks();

  for (const auto& trackAtVtx : tracksAtVertex) {
    // Check compatibility
    if (trackAtVtx.trackWeight < m_cfg.cutOffTrackWeight) {
      // Do not remove track here, since it is not compatible with the vertex
      continue;
    }
    // Find and remove track from seedTracks
    auto foundSeedIter =
        std::ranges::find(seedTracks, trackAtVtx.originalParams);
    if (foundSeedIter != seedTracks.end()) {
      seedTracks.erase(foundSeedIter);
    } else {
      ACTS_WARNING("Track trackAtVtx not found in seedTracks!");
    }

    // Find and remove track from tracksToFit
    auto foundFitIter =
        std::ranges::find(tracksToFit, trackAtVtx.originalParams);
    if (foundFitIter != tracksToFit.end()) {
      tracksToFit.erase(foundFitIter);
    } else {
      ACTS_WARNING("Track trackAtVtx not found in tracksToFit!");
    }
  }  // end iteration over tracksAtVertex

  ACTS_DEBUG("After removal of tracks belonging to vertex, "
             << seedTracks.size() << " seed tracks left.");

  // Now start considering outliers
  // tracksToFit that are left here were below
  // m_cfg.cutOffTrackWeight threshold and are hence outliers
  ACTS_DEBUG("Number of outliers: " << tracksToFit.size());

  const std::shared_ptr<PerigeeSurface> vertexPerigeeSurface =
      Surface::makeShared<PerigeeSurface>(
          VectorHelpers::position(vertex.fullPosition()));

  for (const auto& trk : tracksToFit) {
    // calculate chi2 w.r.t. last fitted vertex
    auto result =
        getCompatibility(m_cfg.extractParameters(trk), vertex,
                         *vertexPerigeeSurface, vertexingOptions, state);

    if (!result.ok()) {
      return result.error();
    }

    double chi2 = *result;

    // check if sufficiently compatible with last fitted vertex
    // (quite loose constraint)
    if (chi2 < m_cfg.maximumChi2cutForSeeding) {
      auto foundIter = std::ranges::find(seedTracks, trk);
      if (foundIter != seedTracks.end()) {
        // Remove track from seed tracks
        seedTracks.erase(foundIter);
      }

    } else {
      // Track not compatible with vertex
      // Remove track from current vertex
      auto foundIter = std::ranges::find_if(
          tracksAtVertex,
          [&trk](auto trkAtVtx) { return trk == trkAtVtx.originalParams; });
      if (foundIter != tracksAtVertex.end()) {
        // Remove track from seed tracks
        tracksAtVertex.erase(foundIter);
      }
    }
  }

  // set updated (possibly with removed outliers) tracksAtVertex to vertex
  vertex.setTracksAtVertex(tracksAtVertex);

  return {};
}

Acts::Result<void> Acts::IterativeVertexFinder::fillTracksToFit(
    const std::vector<InputTrack>& seedTracks, const Vertex& seedVertex,
    std::vector<InputTrack>& tracksToFitOut,
    std::vector<InputTrack>& tracksToFitSplitVertexOut,
    const VertexingOptions& vertexingOptions, State& state) const {
  int numberOfTracks = seedTracks.size();

  // Count how many tracks are used for fit
  int count = 0;
  // Fill tracksToFit vector with tracks compatible with seed
  for (const auto& sTrack : seedTracks) {
    // If there are only few tracks left, add them to fit regardless of their
    // position:
    if (numberOfTracks <= 2) {
      tracksToFitOut.push_back(sTrack);
      ++count;
    } else if (numberOfTracks <= 4 && !m_cfg.createSplitVertices) {
      tracksToFitOut.push_back(sTrack);
      ++count;
    } else if (numberOfTracks <= 4 * m_cfg.splitVerticesTrkInvFraction &&
               m_cfg.createSplitVertices) {
      if (count % m_cfg.splitVerticesTrkInvFraction != 0) {
        tracksToFitOut.push_back(sTrack);
        ++count;
      } else {
        tracksToFitSplitVertexOut.push_back(sTrack);
        ++count;
      }
    }
    // If a large amount of tracks is available, we check their compatibility
    // with the vertex before adding them to the fit:
    else {
      const BoundTrackParameters& sTrackParams =
          m_cfg.extractParameters(sTrack);
      auto distanceRes = m_cfg.ipEst.calculateDistance(
          vertexingOptions.geoContext, sTrackParams, seedVertex.position(),
          state.ipState);
      if (!distanceRes.ok()) {
        return distanceRes.error();
      }

      if (!sTrackParams.covariance()) {
        return VertexingError::NoCovariance;
      }

      // sqrt(sigma(d0)^2+sigma(z0)^2), where sigma(d0)^2 is the variance of d0
      double hypotVariance =
          sqrt((*(sTrackParams.covariance()))(eBoundLoc0, eBoundLoc0) +
               (*(sTrackParams.covariance()))(eBoundLoc1, eBoundLoc1));

      if (hypotVariance == 0.) {
        ACTS_WARNING(
            "Track impact parameter covariances are zero. Track was not "
            "assigned to vertex.");
        continue;
      }

      if (*distanceRes / hypotVariance < m_cfg.significanceCutSeeding) {
        if (!m_cfg.createSplitVertices ||
            count % m_cfg.splitVerticesTrkInvFraction != 0) {
          tracksToFitOut.push_back(sTrack);
          ++count;
        } else {
          tracksToFitSplitVertexOut.push_back(sTrack);
          ++count;
        }
      }
    }
  }
  return {};
}

Acts::Result<bool> Acts::IterativeVertexFinder::reassignTracksToNewVertex(
    std::vector<Vertex>& vertexCollection, Vertex& currentVertex,
    std::vector<InputTrack>& tracksToFit, std::vector<InputTrack>& seedTracks,
    const std::vector<InputTrack>& /* origTracks */,
    const VertexingOptions& vertexingOptions, State& state) const {
  int numberOfAddedTracks = 0;

  const std::shared_ptr<PerigeeSurface> currentVertexPerigeeSurface =
      Surface::makeShared<PerigeeSurface>(
          VectorHelpers::position(currentVertex.fullPosition()));

  // iterate over all vertices and check if tracks need to be reassigned
  // to new (current) vertex
  for (auto& vertexIt : vertexCollection) {
    // tracks at vertexIt
    std::vector<TrackAtVertex> tracksAtVertex = vertexIt.tracks();
    auto tracksBegin = tracksAtVertex.begin();
    auto tracksEnd = tracksAtVertex.end();

    const std::shared_ptr<PerigeeSurface> vertexItPerigeeSurface =
        Surface::makeShared<PerigeeSurface>(
            VectorHelpers::position(vertexIt.fullPosition()));

    for (auto tracksIter = tracksBegin; tracksIter != tracksEnd;) {
      // consider only tracks that are not too tightly assigned to other
      // vertex
      if (tracksIter->trackWeight > m_cfg.cutOffTrackWeightReassign) {
        tracksIter++;
        continue;
      }
      // use original perigee parameters
      BoundTrackParameters origParams =
          m_cfg.extractParameters(tracksIter->originalParams);

      // compute compatibility
      auto resultNew = getCompatibility(origParams, currentVertex,
                                        *currentVertexPerigeeSurface,
                                        vertexingOptions, state);
      if (!resultNew.ok()) {
        return Result<bool>::failure(resultNew.error());
      }
      double chi2NewVtx = *resultNew;

      auto resultOld =
          getCompatibility(origParams, vertexIt, *vertexItPerigeeSurface,
                           vertexingOptions, state);
      if (!resultOld.ok()) {
        return Result<bool>::failure(resultOld.error());
      }
      double chi2OldVtx = *resultOld;

      ACTS_DEBUG("Compatibility to new vs old vertex: " << chi2NewVtx << " vs "
                                                        << chi2OldVtx);

      if (chi2NewVtx < chi2OldVtx) {
        tracksToFit.push_back(tracksIter->originalParams);
        // origTrack was already deleted from seedTracks previously
        // (when assigned to old vertex)
        // add it now back to seedTracks to be able to consistently
        // delete it later
        // when all tracks used to fit current vertex are deleted
        seedTracks.push_back(tracksIter->originalParams);
        // seedTracks.push_back(*std::ranges::find_if(
        //     origTracks,
        //     [&origParams, this](auto origTrack) {
        //       return origParams == m_extractParameters(*origTrack);
        //     }));

        numberOfAddedTracks += 1;

        // remove track from old vertex
        tracksIter = tracksAtVertex.erase(tracksIter);
        tracksBegin = tracksAtVertex.begin();
        tracksEnd = tracksAtVertex.end();

      }  // end chi2NewVtx < chi2OldVtx

      else {
        // go and check next track
        ++tracksIter;
      }
    }  // end loop over tracks at old vertexIt

    vertexIt.setTracksAtVertex(tracksAtVertex);
  }  // end loop over all vertices

  ACTS_DEBUG("Added " << numberOfAddedTracks
                      << " tracks from old (other) vertices for new fit");

  // override current vertex with new fit
  // set first to default vertex to be able to check if still good vertex
  // later
  currentVertex = Vertex();
  if (vertexingOptions.useConstraintInFit && !tracksToFit.empty()) {
    auto fitResult =
        m_cfg.vertexFitter.fit(tracksToFit, vertexingOptions, state.fieldCache);
    if (fitResult.ok()) {
      currentVertex = std::move(*fitResult);
    } else {
      return Result<bool>::success(false);
    }
  } else if (!vertexingOptions.useConstraintInFit && tracksToFit.size() > 1) {
    auto fitResult =
        m_cfg.vertexFitter.fit(tracksToFit, vertexingOptions, state.fieldCache);
    if (fitResult.ok()) {
      currentVertex = std::move(*fitResult);
    } else {
      return Result<bool>::success(false);
    }
  }

  // Number degrees of freedom
  double ndf = currentVertex.fitQuality().second;

  // Number of significant tracks
  int nTracksAtVertex = countSignificantTracks(currentVertex);

  bool isGoodVertex = ((!vertexingOptions.useConstraintInFit && ndf > 0 &&
                        nTracksAtVertex >= 2) ||
                       (vertexingOptions.useConstraintInFit && ndf > 3 &&
                        nTracksAtVertex >= 2));

  if (!isGoodVertex) {
    removeTracks(tracksToFit, seedTracks);

    ACTS_DEBUG("Going to new iteration with "
               << seedTracks.size() << "seed tracks after BAD vertex.");
  }

  return Result<bool>::success(isGoodVertex);
}

int Acts::IterativeVertexFinder::countSignificantTracks(
    const Vertex& vtx) const {
  return std::count_if(vtx.tracks().begin(), vtx.tracks().end(),
                       [this](const TrackAtVertex& trk) {
                         return trk.trackWeight > m_cfg.cutOffTrackWeight;
                       });
}
