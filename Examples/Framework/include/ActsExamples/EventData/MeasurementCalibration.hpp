// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/EventData/MultiTrajectory.hpp"
#include "Acts/EventData/SourceLink.hpp"
#include "Acts/EventData/VectorMultiTrajectory.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Utilities/CalibrationContext.hpp"
#include "ActsExamples/EventData/Cluster.hpp"
#include <ActsExamples/EventData/Measurement.hpp>

#include <cassert>

namespace Acts {
class VectorMultiTrajectory;
}  // namespace Acts

namespace ActsExamples {

/// Abstract base class for measurement-based calibration
class MeasurementCalibrator {
 public:
  virtual void calibrate(
      const MeasurementContainer& measurements,
      const ClusterContainer* clusters, const Acts::GeometryContext& gctx,
      const Acts::CalibrationContext& cctx, const Acts::SourceLink& sourceLink,
      Acts::VectorMultiTrajectory::TrackStateProxy& trackState) const = 0;

  virtual ~MeasurementCalibrator() = default;
  virtual bool needsClusters() const { return false; }
};

// Calibrator to convert an index source link to a measurement as-is
class PassThroughCalibrator : public MeasurementCalibrator {
 public:
  /// Find the measurement corresponding to the source link.
  ///
  /// @tparam parameters_t Track parameters type
  /// @param gctx The geometry context (unused)
  /// @param trackState The track state to calibrate
  void calibrate(
      const MeasurementContainer& measurements,
      const ClusterContainer* clusters, const Acts::GeometryContext& gctx,
      const Acts::CalibrationContext& cctx, const Acts::SourceLink& sourceLink,
      Acts::VectorMultiTrajectory::TrackStateProxy& trackState) const override;
};

// Adapter class that wraps a MeasurementCalibrator to conform to the
// core ACTS calibration interface
class MeasurementCalibratorAdapter {
 public:
  MeasurementCalibratorAdapter(const MeasurementCalibrator& calibrator,
                               const MeasurementContainer& measurements,
                               const ClusterContainer* clusters = nullptr);

  MeasurementCalibratorAdapter() = delete;

  void calibrate(const Acts::GeometryContext& gctx,
                 const Acts::CalibrationContext& cctx,
                 const Acts::SourceLink& sourceLink,
                 Acts::VectorMultiTrajectory::TrackStateProxy trackState) const;

 private:
  const MeasurementCalibrator& m_calibrator;
  const MeasurementContainer& m_measurements;
  const ClusterContainer* m_clusters;
};

}  // namespace ActsExamples
