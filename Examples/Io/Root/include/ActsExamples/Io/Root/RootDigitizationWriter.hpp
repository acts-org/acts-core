// This file is part of the Acts project.
//
// Copyright (C) 2017-2018 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "ActsExamples/Digitization/SmearingAlgorithm.hpp"
#include "ActsExamples/EventData/DigitizedHit.hpp"
#include "ActsExamples/EventData/GeometryContainers.hpp"
#include "ActsExamples/Framework/WriterT.hpp"
#include <Acts/EventData/Measurement.hpp>
#include <Acts/Geometry/GeometryContext.hpp>
#include <Acts/Geometry/GeometryIdentifier.hpp>
#include <Acts/Utilities/Helpers.hpp>
#include <Acts/Utilities/ParameterDefinitions.hpp>

#include <TTree.h>

#include <memory>
#include <mutex>

class TFile;
class TTree;

namespace ActsExamples {

/// @class RootDigitizationWriter
///
/// Write out a planar cluster collection into a root file
/// to avoid immense long vectors, each cluster is one entry
/// in the root file for optimised data writing speed
/// The event number is part of the written data.
///
/// A common file can be provided for to the writer to attach his TTree,
/// this is done by setting the Config::rootFile pointer to an existing file
///
/// Safe to use from multiple writer threads - uses a std::mutex lock.
class RootDigitizationWriter
    : public WriterT<GeometryIdMultimap<
          Acts::FittableMeasurement<ActsExamples::DigitizedHit>>> {
 public:
  struct Config {
    /// Which measurement collection to write.
    std::string inputMeasurements;
    /// Which simulated (truth) hits collection to use.
    std::string inputSimulatedHits;
    std::string filePath = "";          ///< path of the output file
    std::string fileMode = "RECREATE";  ///< file access mode

    TFile* rootFile = nullptr;  ///< common root file
    /// Optional the smearFunctions
    ActsExamples::GeometryIdMultimap<SmearingAlgorithm::SupportedSmearer>
        smearers;
  };

  struct DigitizationTree {
    const std::array<std::string, Acts::eBoundSize> bNames = {
        "l0", "l1", "phi", "theta", "qop", "time"};

    TTree* tree = nullptr;
    // Identification parameters
    int eventNr;
    int volumeID;
    int layerID;
    int surfaceID;

    /// Type 0 - free, 1 - bound
    int measType = 1.;

    // Truth parameters
    float trueBound[Acts::eBoundSize];
    float trueGx = 0.;
    float trueGy = 0.;
    float trueGz = 0.;

    float recBound[Acts::eBoundSize];

    /// Setup helper to create the tree and
    /// register the branches
    ///
    /// @param treeName the name of the tree to be registered
    void setupTree(const std::string& treeName) {
      tree = new TTree(treeName.c_str(), treeName.c_str());
      // Declare the branches
      tree->Branch("event_nr", &eventNr);
      tree->Branch("volume_id", &volumeID);
      tree->Branch("layer_id", &layerID);
      tree->Branch("surface_id", &surfaceID);
      tree->Branch("measurement_type", &measType);
      for (unsigned int ib = 0; ib < int(Acts::eBoundSize); ++ib) {
        if (ib != int(Acts::eBoundQOverP)) {
          tree->Branch(std::string("true_" + bNames[ib]).c_str(),
                       &trueBound[ib]);
        }
      }
      tree->Branch("true_x", &trueGx);
      tree->Branch("true_y", &trueGy);
      tree->Branch("true_z", &trueGz);
    }

    /// Constructor from GeometryIdentifier
    DigitizationTree(Acts::GeometryIdentifier geoID) {
      auto vID = geoID.volume();
      auto lID = geoID.layer();
      auto mID = geoID.sensitive();
      std::string treeName = "vol" + std::to_string(vID);
      if (lID > 0) {
        treeName += "_lay" + std::to_string(lID);
      }
      if (mID > 0) {
        treeName += "_mod" + std::to_string(mID);
      }
      setupTree(treeName);
    }

    /// Setup the dimension depended branches
    ///
    /// @tparam parset_t Type of the parameter set
    ///
    /// @param ps A dummy bound parameter set
    template <typename parset_t>
    void setupBoundRecBranches(const parset_t& ps) {
      if (ps.template contains<Acts::eBoundLoc0>()) {
        tree->Branch(std::string("rec_" + bNames[Acts::eBoundLoc0]).c_str(),
                     &recBound[Acts::eBoundLoc0]);
      }
      if (ps.template contains<Acts::eBoundLoc1>()) {
        tree->Branch(std::string("rec_" + bNames[Acts::eBoundLoc1]).c_str(),
                     &recBound[Acts::eBoundLoc1]);
      }
      if (ps.template contains<Acts::eBoundPhi>()) {
        tree->Branch(std::string("rec_" + bNames[Acts::eBoundPhi]).c_str(),
                     &recBound[Acts::eBoundPhi]);
      }
      if (ps.template contains<Acts::eBoundTheta>()) {
        tree->Branch(std::string("rec_" + bNames[Acts::eBoundTheta]).c_str(),
                     &recBound[Acts::eBoundTheta]);
      }
      if (ps.template contains<Acts::eBoundTime>()) {
        tree->Branch(std::string("rec_" + bNames[Acts::eBoundTime]).c_str(),
                     &recBound[Acts::eBoundTime]);
      }
    }

    /// Convenience function to register idenfication
    ///
    /// @param eventNr The event number
    /// @param geoID The geometry identifier of the measurement
    void fillIdentification(int evnt, Acts::GeometryIdentifier geoId) {
      eventNr = evnt;
      volumeID = geoId.volume();
      layerID = geoId.layer();
      surfaceID = geoId.sensitive();
    }

    /// Convenience function to register the truth parameters
    ///
    /// @param lp The true local position
    /// @param xt The true 4D global position
    /// @param dir The true particle direction
    void fillTruthParameters(const Acts::Vector2D& lp, const Acts::Vector4D& xt,
                             const Acts::Vector3D& dir) {
      trueBound[Acts::eBoundLoc0] = lp[Acts::eBoundLoc0];
      trueBound[Acts::eBoundLoc1] = lp[Acts::eBoundLoc1];
      trueBound[Acts::eBoundPhi] = Acts::VectorHelpers::phi(dir);
      trueBound[Acts::eBoundTheta] = Acts::VectorHelpers::theta(dir);
      trueBound[Acts::eBoundTime] = xt[Acts::eTime];

      trueGx = xt[Acts::ePos0];
      trueGy = xt[Acts::ePos1];
      trueGz = xt[Acts::ePos2];
    }

    /// Convenience function to fill bound parameters
    ///
    /// @tparam measurement_t Type of the parameter set
    ///
    /// @param m The measurement set
    template <typename measurement_t>
    void fillBoundMeasurement(const measurement_t& m) {
      auto fullVect = m.projector().transpose() * m.parameters();
      recBound[Acts::eBoundLoc0] = fullVect[Acts::eBoundLoc0];
      recBound[Acts::eBoundLoc1] = fullVect[Acts::eBoundLoc1];
      recBound[Acts::eBoundPhi] = fullVect[Acts::eBoundPhi];
      recBound[Acts::eBoundTheta] = fullVect[Acts::eBoundTheta];
      recBound[Acts::eBoundTime] = fullVect[Acts::eBoundTime];
    }
  };

  /// Constructor with
  /// @param cfg configuration struct
  /// @param output logging level
  RootDigitizationWriter(const Config& cfg, Acts::Logging::Level lvl);

  /// Virtual destructor
  ~RootDigitizationWriter() override;

  /// End-of-run hook
  ProcessCode endRun() final override;

 protected:
  /// This implementation holds the actual writing method
  /// and is called by the WriterT<>::write interface
  ///
  /// @param ctx The Algorithm context with per event information
  /// @param measurements is the data to be written out
  ProcessCode writeT(const AlgorithmContext& ctx,
                     const GeometryIdMultimap<
                         Acts::FittableMeasurement<ActsExamples::DigitizedHit>>&
                         measurements) final override;

  /// Helper function to create a unique true represenation
  /// for truth parameters from simulated hits
  ///
  /// @param gCtx The geometry context for this
  /// @param surface The reference surface of the measurement
  /// @param simulatedHits The simulated hits used for this measurement
  ///
  /// @return a local position, a 4D global position, a direction
  std::tuple<Acts::Vector2D, Acts::Vector4D, Acts::Vector3D> truthParameters(
      const Acts::GeometryContext& gCtx, const Acts::Surface& surface,
      const std::vector<SimHit>& simulatedHits);

 private:
  Config m_cfg;
  std::mutex m_writeMutex;  ///< protect multi-threaded writes
  TFile* m_outputFile;      ///< the output file
  GeometryIdMultimap<std::unique_ptr<DigitizationTree>>
      m_outputTrees;  ///< the output trees

  std::unordered_map<Acts::GeometryIdentifier, const Acts::Surface*>
      m_dSurfaces;  ///< All surfaces that could carry measurements
};

}  // namespace ActsExamples
