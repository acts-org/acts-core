// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/Performance/VertexPerformanceWriter.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/GenericBoundTrackParameters.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Propagator/SympyStepper.hpp"
#include "Acts/Surfaces/PerigeeSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/MultiIndex.hpp"
#include "Acts/Utilities/UnitVectors.hpp"
#include "Acts/Utilities/Zip.hpp"
#include "Acts/Vertexing/TrackAtVertex.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/EventData/SimVertex.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/EventData/Trajectories.hpp"
#include "ActsExamples/EventData/TruthMatching.hpp"
#include "ActsExamples/Validation/TrackClassification.hpp"
#include "ActsFatras/EventData/Barcode.hpp"
#include "ActsFatras/EventData/Particle.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <ios>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <set>
#include <stdexcept>
#include <utility>

#include <TFile.h>
#include <TTree.h>

using Acts::VectorHelpers::eta;
using Acts::VectorHelpers::perp;
using Acts::VectorHelpers::phi;
using Acts::VectorHelpers::theta;

namespace ActsExamples {

namespace {

std::uint32_t getNumberOfReconstructableVertices(
    const SimParticleContainer& collection) {
  // map for finding frequency
  std::map<std::uint32_t, std::uint32_t> fmap;

  std::vector<std::uint32_t> reconstructableTruthVertices;

  // traverse the array for frequency
  for (const SimParticle& p : collection) {
    std::uint32_t generation = p.particleId().generation();
    if (generation > 0) {
      // truthparticle from secondary vtx
      continue;
    }
    std::uint32_t priVtxId = p.particleId().vertexPrimary();
    fmap[priVtxId]++;
  }

  // iterate over the map
  for (const auto& [priVtxId, occurrence] : fmap) {
    // Require at least 2 tracks
    if (occurrence > 1) {
      reconstructableTruthVertices.push_back(priVtxId);
    }
  }

  return reconstructableTruthVertices.size();
}

std::uint32_t getNumberOfTruePriVertices(
    const SimParticleContainer& collection) {
  // Vector to store indices of all primary vertices
  std::set<std::uint32_t> allPriVtxIds;
  for (const SimParticle& p : collection) {
    std::uint32_t priVtxId = p.particleId().vertexPrimary();
    std::uint32_t generation = p.particleId().generation();
    if (generation > 0) {
      // truthparticle from secondary vtx
      continue;
    }
    // Insert to set, removing duplicates
    allPriVtxIds.insert(priVtxId);
  }
  // Size of set corresponds to total number of primary vertices
  return allPriVtxIds.size();
}

}  // namespace

VertexPerformanceWriter::VertexPerformanceWriter(
    const VertexPerformanceWriter::Config& config, Acts::Logging::Level level)
    : WriterT(config.inputVertices, "VertexPerformanceWriter", level),
      m_cfg(config) {
  if (m_cfg.filePath.empty()) {
    throw std::invalid_argument("Missing output filename");
  }
  if (m_cfg.treeName.empty()) {
    throw std::invalid_argument("Missing tree name");
  }
  if (m_cfg.inputTruthVertices.empty()) {
    throw std::invalid_argument("Collection with truth vertices missing");
  }
  if (m_cfg.inputParticles.empty()) {
    throw std::invalid_argument("Collection with particles missing");
  }
  if (m_cfg.inputSelectedParticles.empty()) {
    throw std::invalid_argument("Collection with selected particles missing");
  }
  if (m_cfg.inputTrackParticleMatching.empty()) {
    throw std::invalid_argument("Missing input track particles matching");
  }

  m_inputTruthVertices.initialize(m_cfg.inputTruthVertices);
  m_inputParticles.initialize(m_cfg.inputParticles);
  m_inputSelectedParticles.initialize(m_cfg.inputSelectedParticles);
  m_inputTrackParticleMatching.initialize(m_cfg.inputTrackParticleMatching);

  if (m_cfg.useTracks) {
    m_inputTracks.initialize(m_cfg.inputTracks);
  }

  // Setup ROOT I/O
  m_outputFile = TFile::Open(m_cfg.filePath.c_str(), m_cfg.fileMode.c_str());
  if (m_outputFile == nullptr) {
    throw std::ios_base::failure("Could not open '" + m_cfg.filePath + "'");
  }
  m_outputFile->cd();
  m_outputTree = new TTree(m_cfg.treeName.c_str(), m_cfg.treeName.c_str());
  if (m_outputTree == nullptr) {
    throw std::bad_alloc();
  }

  m_outputTree->Branch("event_nr", &m_eventNr);

  m_outputTree->Branch("nRecoVtx", &m_nRecoVtx);
  m_outputTree->Branch("nTrueVtx", &m_nTrueVtx);
  m_outputTree->Branch("nCleanVtx", &m_nCleanVtx);
  m_outputTree->Branch("nMergedVtx", &m_nMergedVtx);
  m_outputTree->Branch("nSplitVtx", &m_nSplitVtx);
  m_outputTree->Branch("nVtxDetectorAcceptance", &m_nVtxDetAcceptance);
  m_outputTree->Branch("nVtxReconstructable", &m_nVtxReconstructable);

  m_outputTree->Branch("nTracksRecoVtx", &m_nTracksOnRecoVertex);

  m_outputTree->Branch("recoVertexTrackWeights", &m_recoVertexTrackWeights);

  m_outputTree->Branch("sumPt2", &m_sumPt2);

  m_outputTree->Branch("recoX", &m_recoX);
  m_outputTree->Branch("recoY", &m_recoY);
  m_outputTree->Branch("recoZ", &m_recoZ);
  m_outputTree->Branch("recoT", &m_recoT);

  m_outputTree->Branch("covXX", &m_covXX);
  m_outputTree->Branch("covYY", &m_covYY);
  m_outputTree->Branch("covZZ", &m_covZZ);
  m_outputTree->Branch("covTT", &m_covTT);
  m_outputTree->Branch("covXY", &m_covXY);
  m_outputTree->Branch("covXZ", &m_covXZ);
  m_outputTree->Branch("covXT", &m_covXT);
  m_outputTree->Branch("covYZ", &m_covYZ);
  m_outputTree->Branch("covYT", &m_covYT);
  m_outputTree->Branch("covZT", &m_covZT);

  m_outputTree->Branch("seedX", &m_seedX);
  m_outputTree->Branch("seedY", &m_seedY);
  m_outputTree->Branch("seedZ", &m_seedZ);
  m_outputTree->Branch("seedT", &m_seedT);

  m_outputTree->Branch("vertex_primary", &m_vertexPrimary);
  m_outputTree->Branch("vertex_secondary", &m_vertexSecondary);

  m_outputTree->Branch("nTracksTruthVtx", &m_nTracksOnTruthVertex);

  m_outputTree->Branch("truthPrimaryVertexDensity",
                       &m_truthPrimaryVertexDensity);

  m_outputTree->Branch("truthVertexTrackWeights", &m_truthVertexTrackWeights);
  m_outputTree->Branch("truthVertexMatchRatio", &m_truthVertexMatchRatio);
  m_outputTree->Branch("recoVertexContamination", &m_recoVertexContamination);

  m_outputTree->Branch("recoVertexClassification", &m_recoVertexClassification);

  m_outputTree->Branch("truthX", &m_truthX);
  m_outputTree->Branch("truthY", &m_truthY);
  m_outputTree->Branch("truthZ", &m_truthZ);
  m_outputTree->Branch("truthT", &m_truthT);

  m_outputTree->Branch("resX", &m_resX);
  m_outputTree->Branch("resY", &m_resY);
  m_outputTree->Branch("resZ", &m_resZ);
  m_outputTree->Branch("resT", &m_resT);

  m_outputTree->Branch("resSeedZ", &m_resSeedZ);
  m_outputTree->Branch("resSeedT", &m_resSeedT);

  m_outputTree->Branch("pullX", &m_pullX);
  m_outputTree->Branch("pullY", &m_pullY);
  m_outputTree->Branch("pullZ", &m_pullZ);
  m_outputTree->Branch("pullT", &m_pullT);

  m_outputTree->Branch("trk_weight", &m_trkWeight);

  m_outputTree->Branch("trk_recoPhi", &m_recoPhi);
  m_outputTree->Branch("trk_recoTheta", &m_recoTheta);
  m_outputTree->Branch("trk_recoQOverP", &m_recoQOverP);

  m_outputTree->Branch("trk_recoPhiFitted", &m_recoPhiFitted);
  m_outputTree->Branch("trk_recoThetaFitted", &m_recoThetaFitted);
  m_outputTree->Branch("trk_recoQOverPFitted", &m_recoQOverPFitted);

  m_outputTree->Branch("trk_particleId", &m_trkParticleId);

  m_outputTree->Branch("trk_truthPhi", &m_truthPhi);
  m_outputTree->Branch("trk_truthTheta", &m_truthTheta);
  m_outputTree->Branch("trk_truthQOverP", &m_truthQOverP);

  m_outputTree->Branch("trk_resPhi", &m_resPhi);
  m_outputTree->Branch("trk_resTheta", &m_resTheta);
  m_outputTree->Branch("trk_resQOverP", &m_resQOverP);
  m_outputTree->Branch("trk_momOverlap", &m_momOverlap);

  m_outputTree->Branch("trk_resPhiFitted", &m_resPhiFitted);
  m_outputTree->Branch("trk_resThetaFitted", &m_resThetaFitted);
  m_outputTree->Branch("trk_resQOverPFitted", &m_resQOverPFitted);
  m_outputTree->Branch("trk_momOverlapFitted", &m_momOverlapFitted);

  m_outputTree->Branch("trk_pullPhi", &m_pullPhi);
  m_outputTree->Branch("trk_pullTheta", &m_pullTheta);
  m_outputTree->Branch("trk_pullQOverP", &m_pullQOverP);

  m_outputTree->Branch("trk_pullPhiFitted", &m_pullPhiFitted);
  m_outputTree->Branch("trk_pullThetaFitted", &m_pullThetaFitted);
  m_outputTree->Branch("trk_pullQOverPFitted", &m_pullQOverPFitted);
}

VertexPerformanceWriter::~VertexPerformanceWriter() {
  if (m_outputFile != nullptr) {
    m_outputFile->Close();
  }
}

ProcessCode VertexPerformanceWriter::finalize() {
  m_outputFile->cd();
  m_outputTree->Write();
  m_outputFile->Close();

  return ProcessCode::SUCCESS;
}

ProcessCode VertexPerformanceWriter::writeT(
    const AlgorithmContext& ctx, const std::vector<Acts::Vertex>& vertices) {
  const double nan = std::numeric_limits<double>::quiet_NaN();

  // Read truth vertex input collection
  const SimVertexContainer& truthVertices = m_inputTruthVertices(ctx);
  // Read truth particle input collection
  const SimParticleContainer& particles = m_inputParticles(ctx);
  const SimParticleContainer& selectedParticles = m_inputSelectedParticles(ctx);
  const TrackParticleMatching& trackParticleMatching =
      m_inputTrackParticleMatching(ctx);

  const ConstTrackContainer* tracks = nullptr;
  SimParticleContainer recoParticles;

  auto findParticle = [&](ConstTrackProxy track) -> std::optional<SimParticle> {
    // Get the truth-matched particle
    auto imatched = trackParticleMatching.find(track.index());
    if (imatched == trackParticleMatching.end() ||
        !imatched->second.particle.has_value()) {
      ACTS_DEBUG("No truth particle associated with this track, index = "
                 << track.index() << " tip index = " << track.tipIndex());
      return {};
    }

    const TrackMatchEntry& particleMatch = imatched->second;

    auto iparticle = particles.find(particleMatch.particle->value());
    if (iparticle == particles.end()) {
      ACTS_DEBUG(
          "Truth particle found but not monitored with this track, index = "
          << track.index() << " tip index = " << track.tipIndex()
          << " and this barcode = " << particleMatch.particle->value());
      return {};
    }

    return *iparticle;
  };

  auto findTrack = [&](const Acts::TrackAtVertex& trkAtVtx)
      -> std::optional<ConstTrackProxy> {
    // Track parameters before the vertex fit
    const Acts::BoundTrackParameters& origTrack =
        *trkAtVtx.originalParams.as<Acts::BoundTrackParameters>();

    // Finding the matching parameters in the container of all track
    // parameters. This allows us to identify the corresponding particle.
    // TODO this should not be necessary if the tracks at vertex would keep
    // this information
    for (ConstTrackProxy inputTrk : *tracks) {
      const auto& params = inputTrk.parameters();

      if (origTrack.parameters() == params) {
        return inputTrk;
      }
    }

    return std::nullopt;
  };

  auto calcSumPt2 = [this](const Acts::Vertex& vtx) {
    double sumPt2 = 0;
    for (const auto& trk : vtx.tracks()) {
      if (trk.trackWeight > m_cfg.minTrkWeight) {
        double pt = trk.originalParams.as<Acts::BoundTrackParameters>()
                        ->transverseMomentum();
        sumPt2 += pt * pt;
      }
    }
    return sumPt2;
  };

  auto weightHighEnough = [this](const Acts::TrackAtVertex& trkAtVtx) {
    return trkAtVtx.trackWeight > m_cfg.minTrkWeight;
  };

  // Helper function for computing the pull
  auto pull =
      [this](const Acts::ActsScalar& diff, const Acts::ActsScalar& variance,
             const std::string& variableStr, const bool& afterFit = true) {
        if (variance <= 0) {
          std::string tempStr;
          if (afterFit) {
            tempStr = "after";
          } else {
            tempStr = "before";
          }
          ACTS_WARNING("Nonpositive variance "
                       << tempStr << " vertex fit: Var(" << variableStr
                       << ") = " << variance << " <= 0.");
          return std::numeric_limits<double>::quiet_NaN();
        }
        double std = std::sqrt(variance);
        return diff / std;
      };

  auto calculateTruthPrimaryVertexDensity =
      [this, truthVertices](const Acts::Vertex& vtx) {
        double z = vtx.fullPosition()[Acts::CoordinateIndices::eZ];
        int count = 0;
        for (const SimVertex& truthVertex : truthVertices) {
          if (truthVertex.vertexId().vertexSecondary() != 0) {
            continue;
          }
          double zTruth = truthVertex.position4[Acts::CoordinateIndices::eZ];
          if (std::abs(z - zTruth) <= m_cfg.vertexDensityWindow) {
            ++count;
          }
        }
        return count / (2 * m_cfg.vertexDensityWindow);
      };

  if (m_cfg.useTracks) {
    tracks = &m_inputTracks(ctx);

    for (ConstTrackProxy track : *tracks) {
      if (!track.hasReferenceSurface()) {
        ACTS_DEBUG("No reference surface on this track, index = "
                   << track.index() << " tip index = " << track.tipIndex());
        continue;
      }

      if (std::optional<SimParticle> particle = findParticle(track);
          particle.has_value()) {
        recoParticles.insert(*particle);
      }
    }
  } else {
    // if not using tracks, then all truth particles are associated with the
    // vertex
    recoParticles = particles;
  }

  // Exclusive access to the tree while writing
  std::lock_guard<std::mutex> lock(m_writeMutex);

  m_nRecoVtx = vertices.size();
  m_nCleanVtx = 0;
  m_nMergedVtx = 0;
  m_nSplitVtx = 0;

  ACTS_DEBUG("Number of reco vertices in event: " << m_nRecoVtx);

  // Get number of generated true primary vertices
  m_nTrueVtx = getNumberOfTruePriVertices(particles);
  // Get number of detector-accepted true primary vertices
  m_nVtxDetAcceptance = getNumberOfTruePriVertices(selectedParticles);

  ACTS_DEBUG("Number of truth particles in event : " << particles.size());
  ACTS_DEBUG("Number of truth primary vertices : " << m_nTrueVtx);
  ACTS_DEBUG("Number of detector-accepted truth primary vertices : "
             << m_nVtxDetAcceptance);

  // Get the event number
  m_eventNr = ctx.eventNumber;

  // Get number of track-associated true primary vertices
  m_nVtxReconstructable = getNumberOfReconstructableVertices(recoParticles);

  ACTS_INFO("Number of reconstructed tracks : "
            << ((tracks != nullptr) ? tracks->size() : 0));
  ACTS_INFO("Number of reco track-associated truth particles in event : "
            << recoParticles.size());
  ACTS_INFO("Maximum number of reconstructible primary vertices : "
            << m_nVtxReconstructable);

  // We compare the reconstructed momenta to the true momenta at the vertex. For
  // this, we propagate the reconstructed tracks to the PCA of the true vertex
  // position. Setting up propagator:
  Acts::SympyStepper stepper(m_cfg.bField);
  using Propagator = Acts::Propagator<Acts::SympyStepper>;
  auto propagator = std::make_shared<Propagator>(stepper);

  struct ToTruthMatching {
    std::optional<SimVertexBarcode> vertexId;
    double totalTrackWeight{};
    double truthMajorityTrackWeights{};
    double matchFraction{};

    RecoVertexClassification classification{RecoVertexClassification::Unknown};
  };
  struct ToRecoMatching {
    std::size_t recoIndex{};

    double recoSumPt2{};
  };

  std::vector<ToTruthMatching> recoToTruthMatching;
  std::map<SimVertexBarcode, ToRecoMatching> truthToRecoMatching;

  // Do truth matching for each reconstructed vertex
  for (const auto& [vtxIndex, vtx] : Acts::enumerate(vertices)) {
    // Reconstructed tracks that contribute to the reconstructed vertex
    const auto& tracksAtVtx = vtx.tracks();

    // Containers for storing truth particles and truth vertices that
    // contribute
    // to the reconstructed vertex
    std::vector<std::pair<SimVertexBarcode, double>> contributingTruthVertices;

    double totalTrackWeight = 0;
    for (const Acts::TrackAtVertex& trk : tracksAtVtx) {
      if (trk.trackWeight < m_cfg.minTrkWeight) {
        continue;
      }

      totalTrackWeight += trk.trackWeight;

      std::optional<ConstTrackProxy> trackOpt = findTrack(trk);
      if (!trackOpt.has_value()) {
        ACTS_DEBUG("Track has no matching input track.");
        continue;
      }
      const ConstTrackProxy& inputTrk = *trackOpt;
      std::optional<SimParticle> particleOpt = findParticle(inputTrk);
      if (!particleOpt.has_value()) {
        ACTS_VERBOSE("Track has no matching truth particle.");
      } else {
        contributingTruthVertices.emplace_back(
            SimBarcode{particleOpt->particleId()}.vertexId(), trk.trackWeight);
      }
    }

    // Find true vertex that contributes most to the reconstructed vertex
    std::map<SimVertexBarcode, std::pair<int, double>> fmap;
    for (const auto& [vtxId, weight] : contributingTruthVertices) {
      ++fmap[vtxId].first;
      fmap[vtxId].second += weight;
    }
    double truthMajorityVertexTrackWeights = 0;
    SimVertexBarcode truthMajorityVertexId = 0;
    for (const auto& [vtxId, counter] : fmap) {
      if (counter.second > truthMajorityVertexTrackWeights) {
        truthMajorityVertexId = vtxId;
        truthMajorityVertexTrackWeights = counter.second;
      }
    }

    double sumPt2 = calcSumPt2(vtx);

    double vertexMatchFraction =
        truthMajorityVertexTrackWeights / totalTrackWeight;
    RecoVertexClassification recoVertexClassification =
        RecoVertexClassification::Unknown;

    if (vertexMatchFraction >= m_cfg.vertexMatchThreshold) {
      recoVertexClassification = RecoVertexClassification::Clean;
    } else {
      recoVertexClassification = RecoVertexClassification::Merged;
    }

    recoToTruthMatching.push_back({truthMajorityVertexId, totalTrackWeight,
                                   truthMajorityVertexTrackWeights,
                                   vertexMatchFraction,
                                   recoVertexClassification});

    auto& recoToTruth = recoToTruthMatching.back();

    // We have to decide if this reco vertex is a split vertex.
    if (auto it = truthToRecoMatching.find(truthMajorityVertexId);
        it != truthToRecoMatching.end()) {
      // This truth vertex is already matched to a reconstructed vertex so we
      // are dealing with a split vertex.

      // We have to decide which of the two reconstructed vertices is the
      // split vertex.
      if (sumPt2 <= it->second.recoSumPt2) {
        // Since the sumPt2 is smaller we can simply call this a split vertex

        recoToTruth.classification = RecoVertexClassification::Split;

        // Keep the existing truth to reco matching
      } else {
        // The sumPt2 is larger, so we call the other vertex a split vertex.

        auto& otherRecoToTruth = recoToTruthMatching.at(it->second.recoIndex);
        // Swap the classification
        recoToTruth.classification = otherRecoToTruth.classification;
        otherRecoToTruth.classification = RecoVertexClassification::Split;

        // Overwrite the truth to reco matching
        it->second = {vtxIndex, sumPt2};
      }
    } else {
      truthToRecoMatching[truthMajorityVertexId] = {vtxIndex, sumPt2};
    }
  }

  // Loop over reconstructed vertices and see if they can be matched to a true
  // vertex.
  for (const auto& [vtxIndex, vtx] : Acts::enumerate(vertices)) {
    // Reconstructed tracks that contribute to the reconstructed vertex
    const auto& tracksAtVtx = vtx.tracks();
    // Input tracks matched to `tracksAtVtx`
    std::vector<std::uint32_t> trackIndices;

    const auto& toTruthMatching = recoToTruthMatching[vtxIndex];

    m_recoX.push_back(vtx.fullPosition()[Acts::CoordinateIndices::eX]);
    m_recoY.push_back(vtx.fullPosition()[Acts::CoordinateIndices::eY]);
    m_recoZ.push_back(vtx.fullPosition()[Acts::CoordinateIndices::eZ]);
    m_recoT.push_back(vtx.fullPosition()[Acts::CoordinateIndices::eTime]);

    Acts::ActsScalar varX = vtx.fullCovariance()(Acts::CoordinateIndices::eX,
                                                 Acts::CoordinateIndices::eX);
    Acts::ActsScalar varY = vtx.fullCovariance()(Acts::CoordinateIndices::eY,
                                                 Acts::CoordinateIndices::eY);
    Acts::ActsScalar varZ = vtx.fullCovariance()(Acts::CoordinateIndices::eZ,
                                                 Acts::CoordinateIndices::eZ);
    Acts::ActsScalar varTime = vtx.fullCovariance()(
        Acts::CoordinateIndices::eTime, Acts::CoordinateIndices::eTime);

    m_covXX.push_back(varX);
    m_covYY.push_back(varY);
    m_covZZ.push_back(varZ);
    m_covTT.push_back(varTime);
    m_covXY.push_back(vtx.fullCovariance()(Acts::CoordinateIndices::eX,
                                           Acts::CoordinateIndices::eY));
    m_covXZ.push_back(vtx.fullCovariance()(Acts::CoordinateIndices::eX,
                                           Acts::CoordinateIndices::eZ));
    m_covXT.push_back(vtx.fullCovariance()(Acts::CoordinateIndices::eX,
                                           Acts::CoordinateIndices::eTime));
    m_covYZ.push_back(vtx.fullCovariance()(Acts::CoordinateIndices::eY,
                                           Acts::CoordinateIndices::eZ));
    m_covYT.push_back(vtx.fullCovariance()(Acts::CoordinateIndices::eY,
                                           Acts::CoordinateIndices::eTime));
    m_covZT.push_back(vtx.fullCovariance()(Acts::CoordinateIndices::eZ,
                                           Acts::CoordinateIndices::eTime));

    double sumPt2 = calcSumPt2(vtx);
    m_sumPt2.push_back(sumPt2);

    double recoVertexTrackWeights = 0;
    for (const Acts::TrackAtVertex& trk : tracksAtVtx) {
      if (trk.trackWeight < m_cfg.minTrkWeight) {
        continue;
      }
      recoVertexTrackWeights += trk.trackWeight;
    }
    m_recoVertexTrackWeights.push_back(recoVertexTrackWeights);

    unsigned int nTracksOnRecoVertex =
        std::count_if(tracksAtVtx.begin(), tracksAtVtx.end(), weightHighEnough);
    m_nTracksOnRecoVertex.push_back(nTracksOnRecoVertex);

    // Saving truth information for the reconstructed vertex
    bool truthInfoWritten = false;
    std::optional<Acts::Vector4> truthPos;
    if (toTruthMatching.vertexId.has_value()) {
      auto iTruthVertex = truthVertices.find(toTruthMatching.vertexId.value());
      if (iTruthVertex == truthVertices.end()) {
        ACTS_ERROR("Truth vertex not found.");
        continue;
      }
      const SimVertex& truthVertex = *iTruthVertex;

      m_vertexPrimary.push_back(truthVertex.vertexId().vertexPrimary());
      m_vertexSecondary.push_back(truthVertex.vertexId().vertexSecondary());

      // Count number of reconstructible tracks on truth vertex
      int nTracksOnTruthVertex = 0;
      for (const auto& particle : selectedParticles) {
        if (particle.particleId().vertexId() == truthVertex.vertexId()) {
          ++nTracksOnTruthVertex;
        }
      }
      m_nTracksOnTruthVertex.push_back(nTracksOnTruthVertex);

      double truthPrimaryVertexDensity =
          calculateTruthPrimaryVertexDensity(vtx);
      m_truthPrimaryVertexDensity.push_back(truthPrimaryVertexDensity);

      double truthVertexTrackWeights =
          toTruthMatching.truthMajorityTrackWeights;
      m_truthVertexTrackWeights.push_back(truthVertexTrackWeights);

      double truthVertexMatchRatio = toTruthMatching.matchFraction;
      m_truthVertexMatchRatio.push_back(truthVertexMatchRatio);

      double recoVertexContamination = 1 - truthVertexMatchRatio;
      m_recoVertexContamination.push_back(recoVertexContamination);

      RecoVertexClassification recoVertexClassification =
          toTruthMatching.classification;
      m_recoVertexClassification.push_back(
          static_cast<int>(recoVertexClassification));

      if (recoVertexClassification == RecoVertexClassification::Clean) {
        ++m_nCleanVtx;
      } else if (recoVertexClassification == RecoVertexClassification::Merged) {
        ++m_nMergedVtx;
      } else if (recoVertexClassification == RecoVertexClassification::Split) {
        ++m_nSplitVtx;
      }

      const Acts::Vector4& truePos = truthVertex.position4;
      truthPos = truePos;
      m_truthX.push_back(truePos[Acts::CoordinateIndices::eX]);
      m_truthY.push_back(truePos[Acts::CoordinateIndices::eY]);
      m_truthZ.push_back(truePos[Acts::CoordinateIndices::eZ]);
      m_truthT.push_back(truePos[Acts::CoordinateIndices::eTime]);

      const Acts::Vector4 diffPos = vtx.fullPosition() - truePos;
      m_resX.push_back(diffPos[Acts::CoordinateIndices::eX]);
      m_resY.push_back(diffPos[Acts::CoordinateIndices::eY]);
      m_resZ.push_back(diffPos[Acts::CoordinateIndices::eZ]);
      m_resT.push_back(diffPos[Acts::CoordinateIndices::eTime]);

      m_pullX.push_back(pull(diffPos[Acts::CoordinateIndices::eX], varX, "X"));
      m_pullY.push_back(pull(diffPos[Acts::CoordinateIndices::eY], varY, "Y"));
      m_pullZ.push_back(pull(diffPos[Acts::CoordinateIndices::eZ], varZ, "Z"));
      m_pullT.push_back(
          pull(diffPos[Acts::CoordinateIndices::eTime], varTime, "T"));

      truthInfoWritten = true;
    }
    if (!truthInfoWritten) {
      m_vertexPrimary.push_back(-1);
      m_vertexSecondary.push_back(-1);

      m_nTracksOnTruthVertex.push_back(-1);

      m_truthPrimaryVertexDensity.push_back(nan);

      m_truthVertexTrackWeights.push_back(nan);
      m_truthVertexMatchRatio.push_back(nan);
      m_recoVertexContamination.push_back(nan);

      m_recoVertexClassification.push_back(
          static_cast<int>(RecoVertexClassification::Unknown));

      m_truthX.push_back(nan);
      m_truthY.push_back(nan);
      m_truthZ.push_back(nan);
      m_truthT.push_back(nan);

      m_resX.push_back(nan);
      m_resY.push_back(nan);
      m_resZ.push_back(nan);
      m_resT.push_back(nan);

      m_pullX.push_back(nan);
      m_pullY.push_back(nan);
      m_pullZ.push_back(nan);
      m_pullT.push_back(nan);
    }

    // Saving the reconstructed/truth momenta. The reconstructed momenta
    // are taken at the PCA to the truth vertex position -> we need to
    // perform a propagation.

    // Get references to inner vectors where all track variables corresponding
    // to the current vertex will be saved
    auto& innerTrkWeight = m_trkWeight.emplace_back();

    auto& innerRecoPhi = m_recoPhi.emplace_back();
    auto& innerRecoTheta = m_recoTheta.emplace_back();
    auto& innerRecoQOverP = m_recoQOverP.emplace_back();

    auto& innerRecoPhiFitted = m_recoPhiFitted.emplace_back();
    auto& innerRecoThetaFitted = m_recoThetaFitted.emplace_back();
    auto& innerRecoQOverPFitted = m_recoQOverPFitted.emplace_back();

    auto& innerTrkParticleId = m_trkParticleId.emplace_back();

    auto& innerTruthPhi = m_truthPhi.emplace_back();
    auto& innerTruthTheta = m_truthTheta.emplace_back();
    auto& innerTruthQOverP = m_truthQOverP.emplace_back();

    auto& innerResPhi = m_resPhi.emplace_back();
    auto& innerResTheta = m_resTheta.emplace_back();
    auto& innerResQOverP = m_resQOverP.emplace_back();

    auto& innerResPhiFitted = m_resPhiFitted.emplace_back();
    auto& innerResThetaFitted = m_resThetaFitted.emplace_back();
    auto& innerResQOverPFitted = m_resQOverPFitted.emplace_back();

    auto& innerMomOverlap = m_momOverlap.emplace_back();
    auto& innerMomOverlapFitted = m_momOverlapFitted.emplace_back();

    auto& innerPullPhi = m_pullPhi.emplace_back();
    auto& innerPullTheta = m_pullTheta.emplace_back();
    auto& innerPullQOverP = m_pullQOverP.emplace_back();

    auto& innerPullPhiFitted = m_pullPhiFitted.emplace_back();
    auto& innerPullThetaFitted = m_pullThetaFitted.emplace_back();
    auto& innerPullQOverPFitted = m_pullQOverPFitted.emplace_back();

    // Perigee at the true vertex position
    std::shared_ptr<Acts::PerigeeSurface> perigeeSurface;
    if (truthPos.has_value()) {
      perigeeSurface =
          Acts::Surface::makeShared<Acts::PerigeeSurface>(truthPos->head<3>());
    }
    // Lambda for propagating the tracks to the PCA
    auto propagateToVtx =
        [&](const auto& params) -> std::optional<Acts::BoundTrackParameters> {
      if (!perigeeSurface) {
        return std::nullopt;
      }

      auto intersection =
          perigeeSurface
              ->intersect(ctx.geoContext, params.position(ctx.geoContext),
                          params.direction(),
                          Acts::BoundaryTolerance::Infinite())
              .closest();

      // Setting the geometry/magnetic field context for the event
      using PropagatorOptions = Propagator::Options<>;
      PropagatorOptions pOptions(ctx.geoContext, ctx.magFieldContext);
      pOptions.direction =
          Acts::Direction::fromScalarZeroAsPositive(intersection.pathLength());

      auto result = propagator->propagate(params, *perigeeSurface, pOptions);
      if (!result.ok()) {
        ACTS_ERROR("Propagation to true vertex position failed.");
        return std::nullopt;
      }
      auto& paramsAtVtx = *result->endParameters;
      return std::make_optional(paramsAtVtx);
    };

    for (const Acts::TrackAtVertex& trk : tracksAtVtx) {
      if (trk.trackWeight < m_cfg.minTrkWeight) {
        continue;
      }

      innerTrkWeight.push_back(trk.trackWeight);

      Acts::Vector3 trueUnitDir = Acts::Vector3::Zero();
      Acts::Vector3 trueMom = Acts::Vector3::Zero();

      std::optional<SimParticle> particleOpt;
      std::optional<ConstTrackProxy> trackOpt = findTrack(trk);
      if (trackOpt.has_value()) {
        particleOpt = findParticle(*trackOpt);
      }

      if (particleOpt.has_value()) {
        const SimParticle& particle = *particleOpt;

        innerTrkParticleId.push_back(particle.particleId().value());

        trueUnitDir = particle.direction();
        trueMom.head<2>() = Acts::makePhiThetaFromDirection(trueUnitDir);
        trueMom[2] = particle.qOverP();

        innerTruthPhi.push_back(trueMom[0]);
        innerTruthTheta.push_back(trueMom[1]);
        innerTruthQOverP.push_back(trueMom[2]);
      } else {
        ACTS_VERBOSE("Track has no matching truth particle.");

        innerTrkParticleId.push_back(-1);

        innerTruthPhi.push_back(nan);
        innerTruthTheta.push_back(nan);
        innerTruthQOverP.push_back(nan);
      }

      // Save track parameters before the vertex fit
      const auto paramsAtVtx = propagateToVtx(
          *(trk.originalParams.as<Acts::BoundTrackParameters>()));
      if (paramsAtVtx.has_value()) {
        Acts::Vector3 recoMom =
            paramsAtVtx->parameters().segment(Acts::eBoundPhi, 3);
        const Acts::ActsMatrix<3, 3>& momCov =
            paramsAtVtx->covariance()->template block<3, 3>(Acts::eBoundPhi,
                                                            Acts::eBoundPhi);
        innerRecoPhi.push_back(recoMom[0]);
        innerRecoTheta.push_back(recoMom[1]);
        innerRecoQOverP.push_back(recoMom[2]);

        if (particleOpt.has_value()) {
          Acts::Vector3 diffMom = recoMom - trueMom;
          // Accounting for the periodicity of phi. We overwrite the
          // previously computed value for better readability.
          diffMom[0] = Acts::detail::difference_periodic(recoMom(0), trueMom(0),
                                                         2 * M_PI);
          innerResPhi.push_back(diffMom[0]);
          innerResTheta.push_back(diffMom[1]);
          innerResQOverP.push_back(diffMom[2]);

          innerPullPhi.push_back(pull(diffMom[0], momCov(0, 0), "phi", false));
          innerPullTheta.push_back(
              pull(diffMom[1], momCov(1, 1), "theta", false));
          innerPullQOverP.push_back(
              pull(diffMom[2], momCov(2, 2), "q/p", false));

          const auto& recoUnitDir = paramsAtVtx->direction();
          double overlap = trueUnitDir.dot(recoUnitDir);
          innerMomOverlap.push_back(overlap);
        } else {
          innerResPhi.push_back(nan);
          innerResTheta.push_back(nan);
          innerResQOverP.push_back(nan);
          innerPullPhi.push_back(nan);
          innerPullTheta.push_back(nan);
          innerPullQOverP.push_back(nan);
          innerMomOverlap.push_back(nan);
        }
      } else {
        innerRecoPhi.push_back(nan);
        innerRecoTheta.push_back(nan);
        innerRecoQOverP.push_back(nan);
        innerResPhi.push_back(nan);
        innerResTheta.push_back(nan);
        innerResQOverP.push_back(nan);
        innerPullPhi.push_back(nan);
        innerPullTheta.push_back(nan);
        innerPullQOverP.push_back(nan);
        innerMomOverlap.push_back(nan);
      }

      // Save track parameters after the vertex fit
      const auto paramsAtVtxFitted = propagateToVtx(trk.fittedParams);
      if (paramsAtVtxFitted.has_value()) {
        Acts::Vector3 recoMomFitted =
            paramsAtVtxFitted->parameters().segment(Acts::eBoundPhi, 3);
        const Acts::ActsMatrix<3, 3>& momCovFitted =
            paramsAtVtxFitted->covariance()->block<3, 3>(Acts::eBoundPhi,
                                                         Acts::eBoundPhi);
        innerRecoPhiFitted.push_back(recoMomFitted[0]);
        innerRecoThetaFitted.push_back(recoMomFitted[1]);
        innerRecoQOverPFitted.push_back(recoMomFitted[2]);

        if (particleOpt.has_value()) {
          Acts::Vector3 diffMomFitted = recoMomFitted - trueMom;
          // Accounting for the periodicity of phi. We overwrite the
          // previously computed value for better readability.
          diffMomFitted[0] = Acts::detail::difference_periodic(
              recoMomFitted(0), trueMom(0), 2 * M_PI);
          innerResPhiFitted.push_back(diffMomFitted[0]);
          innerResThetaFitted.push_back(diffMomFitted[1]);
          innerResQOverPFitted.push_back(diffMomFitted[2]);

          innerPullPhiFitted.push_back(
              pull(diffMomFitted[0], momCovFitted(0, 0), "phi"));
          innerPullThetaFitted.push_back(
              pull(diffMomFitted[1], momCovFitted(1, 1), "theta"));
          innerPullQOverPFitted.push_back(
              pull(diffMomFitted[2], momCovFitted(2, 2), "q/p"));

          const auto& recoUnitDirFitted = paramsAtVtxFitted->direction();
          double overlapFitted = trueUnitDir.dot(recoUnitDirFitted);
          innerMomOverlapFitted.push_back(overlapFitted);
        } else {
          innerResPhiFitted.push_back(nan);
          innerResThetaFitted.push_back(nan);
          innerResQOverPFitted.push_back(nan);
          innerPullPhiFitted.push_back(nan);
          innerPullThetaFitted.push_back(nan);
          innerPullQOverPFitted.push_back(nan);
          innerMomOverlapFitted.push_back(nan);
        }
      } else {
        innerRecoPhiFitted.push_back(nan);
        innerRecoThetaFitted.push_back(nan);
        innerRecoQOverPFitted.push_back(nan);
        innerResPhiFitted.push_back(nan);
        innerResThetaFitted.push_back(nan);
        innerResQOverPFitted.push_back(nan);
        innerPullPhiFitted.push_back(nan);
        innerPullThetaFitted.push_back(nan);
        innerPullQOverPFitted.push_back(nan);
        innerMomOverlapFitted.push_back(nan);
      }
    }
  }

  // fill the variables
  m_outputTree->Fill();

  m_nTracksOnRecoVertex.clear();
  m_recoVertexTrackWeights.clear();
  m_recoX.clear();
  m_recoY.clear();
  m_recoZ.clear();
  m_recoT.clear();
  m_covXX.clear();
  m_covYY.clear();
  m_covZZ.clear();
  m_covTT.clear();
  m_covXY.clear();
  m_covXZ.clear();
  m_covXT.clear();
  m_covYZ.clear();
  m_covYT.clear();
  m_covZT.clear();
  m_seedX.clear();
  m_seedY.clear();
  m_seedZ.clear();
  m_seedT.clear();
  m_vertexPrimary.clear();
  m_vertexSecondary.clear();
  m_nTracksOnTruthVertex.clear();
  m_truthPrimaryVertexDensity.clear();
  m_truthVertexTrackWeights.clear();
  m_truthVertexMatchRatio.clear();
  m_recoVertexContamination.clear();
  m_recoVertexClassification.clear();
  m_truthX.clear();
  m_truthY.clear();
  m_truthZ.clear();
  m_truthT.clear();
  m_resX.clear();
  m_resY.clear();
  m_resZ.clear();
  m_resT.clear();
  m_resSeedZ.clear();
  m_resSeedT.clear();
  m_pullX.clear();
  m_pullY.clear();
  m_pullZ.clear();
  m_pullT.clear();
  m_sumPt2.clear();
  m_trkWeight.clear();
  m_recoPhi.clear();
  m_recoTheta.clear();
  m_recoQOverP.clear();
  m_recoPhiFitted.clear();
  m_recoThetaFitted.clear();
  m_recoQOverPFitted.clear();
  m_trkParticleId.clear();
  m_truthPhi.clear();
  m_truthTheta.clear();
  m_truthQOverP.clear();
  m_resPhi.clear();
  m_resTheta.clear();
  m_resQOverP.clear();
  m_momOverlap.clear();
  m_resPhiFitted.clear();
  m_resThetaFitted.clear();
  m_resQOverPFitted.clear();
  m_momOverlapFitted.clear();
  m_pullPhi.clear();
  m_pullTheta.clear();
  m_pullQOverP.clear();
  m_pullPhiFitted.clear();
  m_pullThetaFitted.clear();
  m_pullQOverPFitted.clear();

  return ProcessCode::SUCCESS;
}

}  // namespace ActsExamples
