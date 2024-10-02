// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/SpacePointFormation/SpacePointBuilder.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/IndexSourceLink.hpp"
#include "ActsExamples/EventData/Measurement.hpp"
#include "ActsExamples/EventData/SimSpacePoint.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"

#include <memory>
#include <string>
#include <vector>

namespace Acts {
class TrackingGeometry;
}

namespace ActsExamples {
struct AlgorithmContext;

/// Create space point representations from measurements.
///
/// This implements simple space point construction, where each surface-based
/// measurement translates into one space point using the surface
/// local-to-global transform.
///
/// The algorithm takes both the source links and measurements container as
/// input. The source link container is geometry-sorted and each element is
/// small compared to a measurement. The geometry selection is therefore much
/// easier to perform on the source links than on the unsorted measurements.
///
/// There are no explicit requirements on the content of the input measurements.
/// If no local positions are measured, the transformed global positions will
/// always be the position of the module origin.
class SpacePointMaker final : public IAlgorithm {
 public:
  struct Config {
    /// Input source links collection.
    std::string inputSourceLinks;
    /// Input measurements collection.
    std::string inputMeasurements;
    /// Output space points collection.
    std::string outputSpacePoints;
    /// Tracking geometry for transformation lookup.
    std::shared_ptr<const Acts::TrackingGeometry> trackingGeometry;
    /// For which part of the detector geometry should space points be created.
    ///
    /// Only volumes and layers can be set. Zero values can be used as wildcards
    /// to select larger parts of the hierarchy, i.e. setting only the volume
    /// selects all measurements within that volume. Adding a single identifier
    /// with all components set to zero selects all available measurements. The
    /// selection must not have duplicates.
    std::vector<Acts::GeometryIdentifier> geometrySelection;
  };

  /// Construct the space point maker.
  ///
  /// @param cfg is the algorithm configuration
  /// @param lvl is the logging level
  SpacePointMaker(Config cfg, Acts::Logging::Level lvl);

  /// Run the space point construction.
  ///
  /// @param ctx is the algorithm context with event information
  /// @return a process code indication success or failure
  ProcessCode execute(const AlgorithmContext& ctx) const override;

  /// Const access to the config
  const Config& config() const { return m_cfg; }

 private:
  Config m_cfg;

  std::optional<IndexSourceLink::SurfaceAccessor> m_slSurfaceAccessor;

  Acts::SpacePointBuilder<SimSpacePoint> m_spacePointBuilder;

  ReadDataHandle<IndexSourceLinkContainer> m_inputSourceLinks{
      this, "InputSourceLinks"};

  ReadDataHandle<MeasurementContainer> m_inputMeasurements{this,
                                                           "InputMeasurements"};

  WriteDataHandle<SimSpacePointContainer> m_outputSpacePoints{
      this, "OutputSpacePoints"};
};
}  // namespace ActsExamples
