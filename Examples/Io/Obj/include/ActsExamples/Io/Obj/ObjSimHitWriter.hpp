// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Units.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Framework/WriterT.hpp"

#include <cstdint>
#include <mutex>
#include <string>

namespace ActsExamples {
struct AlgorithmContext;

/// Write out a simhit collection before detector digitization as wavefront obj
/// file(s per event).
///
/// This writes two files per event, one for the hits one for the trajectory.
/// The latter can be smoothed using a spline interpolation.
///
///     event000000001-<stem>.obj
///     event000000001-<stem>_trajectory.obj
///     event000000002-<stem>.obj
///     event000000002-<stem>_trajectory.obj
///
///
/// The trajectory can be smoothed using a spline interpolation, where
/// nInterpolatedPoints points are added between each hit.
class ObjSimHitWriter : public WriterT<SimHitContainer> {
 public:
  struct Config {
    /// Input sim hit collection to write.
    std::string inputSimHits;
    /// Where to place output files
    std::string outputDir;
    /// Output filename stem.
    std::string outputStem = "simhits";
    /// Number of decimal digits for floating point precision in output.
    std::size_t outputPrecision = std::numeric_limits<float>::max_digits10;
    /// Draw line connections between hits
    bool drawConnections = true;
    /// Momentum threshold for hits
    double momentumThreshold = 0.05 * Acts::UnitConstants::GeV;
    /// Momentum threshold for trajectories
    double momentumThresholdTraj = 0.05 * Acts::UnitConstants::GeV;
    /// Number of points to interpolated between hits to smooth the
    /// trajectory view in the obj file.
    std::size_t nInterpolatedPoints = 4;
    /// Keep the original hits in the trajectory file
    bool keepOriginalHits = false;
  };

  /// Construct the particle writer.
  ///
  /// @param config is the configuration object
  /// @param level is the logging level
  ObjSimHitWriter(const Config& config, Acts::Logging::Level level);

  /// Ensure underlying file is closed.
  ~ObjSimHitWriter() override = default;

  /// End-of-run hook
  ProcessCode finalize() override;

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 protected:
  /// Type-specific write implementation.
  ///
  /// @param[in] ctx is the algorithm context
  /// @param[in] hits are the hits to be written
  ProcessCode writeT(const AlgorithmContext& ctx,
                     const SimHitContainer& hits) override;

 private:
  Config m_cfg;
  std::mutex m_writeMutex;
};

}  // namespace ActsExamples
