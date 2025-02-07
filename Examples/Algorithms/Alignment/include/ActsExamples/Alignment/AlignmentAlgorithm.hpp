// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Geometry/GeometryHierarchyMap.hpp"
#include "Acts/TrackFitting/KalmanFitter.hpp"
#include "ActsAlignment/Kernel/Alignment.hpp"
#include "ActsExamples/EventData/Measurement.hpp"
#include "ActsExamples/EventData/ProtoTrack.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"
#include "ActsExamples/MagneticField/MagneticField.hpp"

#include <functional>
#include <memory>
#include <vector>

namespace ActsExamples {

class AlignmentGroup {
 public:
  AlignmentGroup(const std::string& name,
                 const std::vector<Acts::GeometryIdentifier>& geoIds)
      : m_name(name), m_map(constructHierarchyMap(geoIds)) {}

  // Access the name of the group
  std::string getNameOfGroup() const { return m_name; }

  // Useful for testing
  bool has(Acts::GeometryIdentifier geoId) {
    auto it = m_map.find(geoId);
    return (it == m_map.end()) ? false : *it;
  }

 private:
  std::string m_name;  //  storing the name in the class
  Acts::GeometryHierarchyMap<bool> m_map;

  Acts::GeometryHierarchyMap<bool> constructHierarchyMap(
      const std::vector<Acts::GeometryIdentifier>& geoIds) {
    std::vector<Acts::GeometryHierarchyMap<bool>::InputElement> ies;
    for (const auto& geoId : geoIds) {
      ies.emplace_back(geoId, true);
    }
    return Acts::GeometryHierarchyMap<bool>(ies);
  }
};

class AlignmentAlgorithm final : public IAlgorithm {
 public:
  using AlignmentResult = Acts::Result<ActsAlignment::AlignmentResult>;
  using AlignmentParameters =
      std::unordered_map<Acts::DetectorElementBase*, Acts::Transform3>;
  /// Alignment function that takes sets of input measurements, initial
  /// trackstate and alignment options and returns some alignment-specific
  /// result.
  using TrackFitterOptions =
      Acts::KalmanFitterOptions<Acts::VectorMultiTrajectory>;

  /// Alignment function that takes the above parameters and runs alignment
  /// @note This is separated into a virtual interface to keep compilation units
  /// small
  class AlignmentFunction {
   public:
    virtual ~AlignmentFunction() = default;
    virtual AlignmentResult operator()(
        const std::vector<std::vector<IndexSourceLink>>&,
        const TrackParametersContainer&,
        const ActsAlignment::AlignmentOptions<TrackFitterOptions>&) const = 0;
  };

  /// Create the alignment function implementation.
  ///
  /// The magnetic field is intentionally given by-value since the variant
  /// contains shared_ptr anyway.
  static std::shared_ptr<AlignmentFunction> makeAlignmentFunction(
      std::shared_ptr<const Acts::TrackingGeometry> trackingGeometry,
      std::shared_ptr<const Acts::MagneticFieldProvider> magneticField);

  struct Config {
    /// Input measurements collection.
    std::string inputMeasurements;
    /// Input proto tracks collection, i.e. groups of hit indices.
    std::string inputProtoTracks;
    /// Input initial track parameter estimates for for each proto track.
    std::string inputInitialTrackParameters;
    /// Output aligned parameters collection.
    std::string outputAlignmentParameters;
    /// Type erased fitter function.
    std::shared_ptr<AlignmentFunction> align;
    /// The aligned transform updater
    ActsAlignment::AlignedTransformUpdater alignedTransformUpdater;
    /// The surfaces (with detector elements) to be aligned
    std::vector<Acts::DetectorElementBase*> alignedDetElements;
    /// The alignment mask at each iteration
    std::map<unsigned int, std::bitset<6>> iterationState;
    /// Cutoff value for average chi2/ndf
    double chi2ONdfCutOff = 0.10;
    /// Cutoff value for delta of average chi2/ndf within a couple of iterations
    std::pair<std::size_t, double> deltaChi2ONdfCutOff = {10, 0.00001};
    /// Maximum number of iterations
    std::size_t maxNumIterations = 100;
    /// Number of tracks to be used for alignment
    int maxNumTracks = -1;
    std::vector<AlignmentGroup> m_groups;
  };

  /// Constructor of the alignment algorithm
  ///
  /// @param cfg is the config struct to configure the algorithm
  /// @param level is the logging level
  AlignmentAlgorithm(Config cfg, Acts::Logging::Level lvl);

  /// Framework execute method of the alignment algorithm
  ///
  /// @param ctx is the algorithm context that holds event-wise information
  /// @return a process code to steer the algorithm flow
  ActsExamples::ProcessCode execute(
      const ActsExamples::AlgorithmContext& ctx) const override;

 private:
  Config m_cfg;

  ReadDataHandle<MeasurementContainer> m_inputMeasurements{this,
                                                           "InputMeasurements"};
  ReadDataHandle<TrackParametersContainer> m_inputInitialTrackParameters{
      this, "InputInitialTrackParameters"};
  ReadDataHandle<ProtoTrackContainer> m_inputProtoTracks{this,
                                                         "InputProtoTracks"};
  WriteDataHandle<AlignmentParameters> m_outputAlignmentParameters{
      this, "OutputAlignmentParameters"};
};

}  // namespace ActsExamples
