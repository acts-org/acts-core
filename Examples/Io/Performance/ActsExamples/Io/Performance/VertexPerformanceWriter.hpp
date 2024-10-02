// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/MagneticField/MagneticFieldProvider.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Vertexing/Vertex.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/EventData/SimVertex.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/EventData/TruthMatching.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Framework/WriterT.hpp"

#include <mutex>
#include <string>
#include <vector>

class TFile;
class TTree;

namespace ActsExamples {

enum class RecoVertexClassification {
  Unknown = 0,
  Clean,
  Merged,
  Split,
};

/// @class VertexPerformanceWriter
///
/// Writes out the number of reconstructed primary vertices along with
/// the number of primary vertices in detector acceptance as well as
/// reconstructable primary vertices after track fitting.
/// Additionally it matches the reco vertices to their truth vertices
/// and write out the difference in x,y and z position.
class VertexPerformanceWriter final
    : public WriterT<std::vector<Acts::Vertex>> {
 public:
  struct Config {
    /// Input vertex collection.
    std::string inputVertices;
    /// Tracks object from track finidng.
    std::string inputTracks;
    /// Input truth vertex collection.
    std::string inputTruthVertices;
    /// All input truth particle collection.
    std::string inputParticles;
    /// All input selected truth particle collection.
    std::string inputSelectedParticles;
    /// Input track-particle matching.
    std::string inputTrackParticleMatching;
    /// Magnetic field
    std::shared_ptr<Acts::MagneticFieldProvider> bField;
    /// Output filename.
    std::string filePath = "vertexingperformance.root";
    /// Name of the output tree.
    std::string treeName = "vertextree";
    /// File access mode.
    std::string fileMode = "RECREATE";

    /// Minimum fraction of track weight matched between truth
    /// and reco vertices to consider as truth matched.
    double vertexMatchThreshold = 0.7;
    /// Minimum fraction of hits associated to particle to consider track
    /// as truth matched.
    double trackMatchThreshold = 0.5;
    /// Whether information about tracks is available
    bool useTracks = true;
    /// Minimum track weight for track to be considered as part of the fit
    double minTrkWeight = 0.1;
    /// Spatial window for vertex density calculation.
    /// @note This is a Z-window
    /// @note This is a half-window around the reconstructed vertex
    double vertexDensityWindow = 1 * Acts::UnitConstants::mm;
  };

  /// Constructor
  ///
  /// @param config Configuration struct
  /// @param level Message level declaration
  VertexPerformanceWriter(const Config& config, Acts::Logging::Level level);

  ~VertexPerformanceWriter() override;

  /// End-of-run hook
  ProcessCode finalize() override;

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 protected:
  /// @brief Write method called by the base class
  /// @param [in] ctx is the algorithm context for event information
  ProcessCode writeT(const AlgorithmContext& ctx,
                     const std::vector<Acts::Vertex>& vertices) override;

 private:
  Config m_cfg;             ///< The config class
  std::mutex m_writeMutex;  ///< Mutex used to protect multi-threaded writes
  TFile* m_outputFile{nullptr};  ///< The output file
  TTree* m_outputTree{nullptr};  ///< The output tree

  /// The event number
  std::uint32_t m_eventNr{0};

  /// Number of reconstructed vertices
  int m_nRecoVtx = -1;
  /// Number of true vertices
  int m_nTrueVtx = -1;
  /// Number of clean vertices
  int m_nCleanVtx = -1;
  /// Number of merged vertices
  int m_nMergedVtx = -1;
  /// Number of split vertices
  int m_nSplitVtx = -1;
  /// Number of vertices in detector acceptance
  int m_nVtxDetAcceptance = -1;
  /// Max. number of reconstructable vertices (detector acceptance + tracking
  /// efficiency)
  int m_nVtxReconstructable = -1;

  /// Number of tracks associated with the reconstructed vertex
  std::vector<int> m_nTracksOnRecoVertex;

  /// Sum of the track weights associated with the reconstructed vertex
  std::vector<double> m_recoVertexTrackWeights;

  // Sum pT^2 of all tracks associated with the vertex
  std::vector<double> m_sumPt2;

  // Reconstructed 4D vertex position
  std::vector<double> m_recoX;
  std::vector<double> m_recoY;
  std::vector<double> m_recoZ;
  std::vector<double> m_recoT;

  // Vertex covariance
  std::vector<double> m_covXX;
  std::vector<double> m_covYY;
  std::vector<double> m_covZZ;
  std::vector<double> m_covTT;
  std::vector<double> m_covXY;
  std::vector<double> m_covXZ;
  std::vector<double> m_covXT;
  std::vector<double> m_covYZ;
  std::vector<double> m_covYT;
  std::vector<double> m_covZT;

  // 4D position of the vertex seed. x and y coordinate are 0 in current
  // implementations, we save them here as a check.
  std::vector<double> m_seedX;
  std::vector<double> m_seedY;
  std::vector<double> m_seedZ;
  std::vector<double> m_seedT;

  // Truth vertex ID
  std::vector<int> m_vertexPrimary;
  std::vector<int> m_vertexSecondary;

  /// Number of tracks associated with the truth vertex
  std::vector<int> m_nTracksOnTruthVertex;

  /// Truth-based primary vertex density for the reconstructed vertex
  std::vector<double> m_truthPrimaryVertexDensity;

  /// Sum of the track weights associated with the truth vertex
  std::vector<double> m_truthVertexTrackWeights;
  /// Fraction of track weight matched between truth and reco vertices
  std::vector<double> m_truthVertexMatchRatio;
  /// Fraction of incorrectly assigned track weight to the reco vertex
  std::vector<double> m_recoVertexContamination;

  /// Classification of the reconstructed vertex see RecoVertexClassification
  std::vector<int> m_recoVertexClassification;

  // True 4D vertex position
  std::vector<double> m_truthX;
  std::vector<double> m_truthY;
  std::vector<double> m_truthZ;
  std::vector<double> m_truthT;

  // Difference of reconstructed and true vertex 4D position
  std::vector<double> m_resX;
  std::vector<double> m_resY;
  std::vector<double> m_resZ;
  std::vector<double> m_resT;

  // Difference between the seed and the true vertex z and t coordinate
  std::vector<double> m_resSeedZ;
  std::vector<double> m_resSeedT;

  // pull(X) = (X_reco - X_true)/Var(X_reco)^(1/2)
  std::vector<double> m_pullX;
  std::vector<double> m_pullY;
  std::vector<double> m_pullZ;
  std::vector<double> m_pullT;

  //--------------------------------------------------------------
  // Track-related variables are contained in a vector of vectors: The inner
  // vectors contain the values of all tracks corresponding to one vertex. The
  // outer vector can then have the same length as the flat vectors of
  // vertex-related variables (see above). E.g.,
  // m_truthPhi = ((truthPhi of 1st trk belonging to vtx 1,
  //                truthPhi of 2nd trk belonging to vtx 1, ...),
  //               (truthPhi of 1st trk belonging to vtx 2,
  //                truthPhi of 2nd trk belonging to vtx 2, ...),
  //                ...)

  // Track weights from vertex fit, will be set to 1 if we do unweighted vertex
  // fitting
  std::vector<std::vector<double>> m_trkWeight;

  // Reconstructed track momenta at the vertex before and after the vertex fit
  std::vector<std::vector<double>> m_recoPhi;
  std::vector<std::vector<double>> m_recoTheta;
  std::vector<std::vector<double>> m_recoQOverP;

  std::vector<std::vector<double>> m_recoPhiFitted;
  std::vector<std::vector<double>> m_recoThetaFitted;
  std::vector<std::vector<double>> m_recoQOverPFitted;

  std::vector<std::vector<std::uint64_t>> m_trkParticleId;

  // True track momenta at the vertex
  std::vector<std::vector<double>> m_truthPhi;
  std::vector<std::vector<double>> m_truthTheta;
  std::vector<std::vector<double>> m_truthQOverP;

  // Difference between reconstructed momenta and true momenta
  std::vector<std::vector<double>> m_resPhi;
  std::vector<std::vector<double>> m_resTheta;
  std::vector<std::vector<double>> m_resQOverP;
  std::vector<std::vector<double>> m_momOverlap;

  std::vector<std::vector<double>> m_resPhiFitted;
  std::vector<std::vector<double>> m_resThetaFitted;
  std::vector<std::vector<double>> m_resQOverPFitted;
  std::vector<std::vector<double>> m_momOverlapFitted;

  // Pulls
  std::vector<std::vector<double>> m_pullPhi;
  std::vector<std::vector<double>> m_pullTheta;
  std::vector<std::vector<double>> m_pullQOverP;

  std::vector<std::vector<double>> m_pullPhiFitted;
  std::vector<std::vector<double>> m_pullThetaFitted;
  std::vector<std::vector<double>> m_pullQOverPFitted;

  ReadDataHandle<ConstTrackContainer> m_inputTracks{this, "InputTracks"};
  ReadDataHandle<SimVertexContainer> m_inputTruthVertices{this,
                                                          "InputTruthVertices"};
  ReadDataHandle<SimParticleContainer> m_inputParticles{this, "InputParticles"};
  ReadDataHandle<SimParticleContainer> m_inputSelectedParticles{
      this, "InputSelectedParticles"};
  ReadDataHandle<TrackParticleMatching> m_inputTrackParticleMatching{
      this, "InputTrackParticleMatching"};
};

}  // namespace ActsExamples
