// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryHierarchyMap.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/Cluster.hpp"
#include "ActsExamples/EventData/Index.hpp"
#include "ActsExamples/EventData/Measurement.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Framework/WriterT.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class TFile;
class TTree;
namespace Acts {
class Surface;
class TrackingGeometry;
}  // namespace Acts

namespace ActsExamples {
struct AlgorithmContext;

/// @class RootMeasurementWriter
///
/// Write out a planar cluster collection into a root file
/// to avoid immense long vectors, each cluster is one entry
/// in the root file for optimised data writing speed
/// The event number is part of the written data.
///
/// A common file can be provided for the writer to attach his TTree,
/// this is done by setting the Config::rootFile pointer to an existing file
///
/// Safe to use from multiple writer threads - uses a std::mutex lock.
class RootMeasurementWriter final : public WriterT<MeasurementContainer> {
 public:
  struct Config {
    /// Which measurement collection to write.
    std::string inputMeasurements;
    /// Which cluster collection to write (optional)
    std::string inputClusters;
    /// Which simulated (truth) hits collection to use.
    std::string inputSimHits;
    /// Input collection to map measured hits to simulated hits.
    std::string inputMeasurementSimHitsMap;

    /// path of the output file
    std::string filePath = "";
    /// file access mode
    std::string fileMode = "RECREATE";

    /// The indices for this digitization configurations
    Acts::GeometryHierarchyMap<std::vector<Acts::BoundIndices>> boundIndices;
    /// Map of the geometry identifier to the surface
    std::unordered_map<Acts::GeometryIdentifier, const Acts::Surface*>
        surfaceByIdentifier;
  };

  /// Constructor with
  /// @param cfg configuration struct
  /// @param output logging level
  RootMeasurementWriter(const Config& config, Acts::Logging::Level level);

  /// Virtual destructor
  ~RootMeasurementWriter() override;

  /// End-of-run hook
  ProcessCode finalize() override;

  /// Get const access to the config
  const Config& config() const { return m_cfg; }

 protected:
  /// This implementation holds the actual writing method
  /// and is called by the WriterT<>::write interface
  ///
  /// @param ctx The Algorithm context with per event information
  /// @param measurements is the data to be written out
  ProcessCode writeT(const AlgorithmContext& ctx,
                     const MeasurementContainer& measurements) override;

 private:
  struct DigitizationTree;

  Config m_cfg;
  /// protect multi-threaded writes
  std::mutex m_writeMutex;
  /// the output file
  TFile* m_outputFile;
  /// the output tree
  std::unique_ptr<DigitizationTree> m_outputTree;

  ReadDataHandle<ClusterContainer> m_inputClusters{this, "InputClusters"};
  ReadDataHandle<SimHitContainer> m_inputSimHits{this, "InputSimHits"};
  ReadDataHandle<IndexMultimap<Index>> m_inputMeasurementSimHitsMap{
      this, "InputMeasurementSimHitsMap"};
};

}  // namespace ActsExamples
