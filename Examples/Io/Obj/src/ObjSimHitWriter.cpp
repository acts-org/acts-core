// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/Io/Obj/ObjSimHitWriter.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Common.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Visualization/Interpolation3D.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Utilities/Paths.hpp"
#include "ActsFatras/EventData/Barcode.hpp"
#include "ActsFatras/EventData/Hit.hpp"

#include <fstream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

ActsExamples::ObjSimHitWriter::ObjSimHitWriter(
    const ActsExamples::ObjSimHitWriter::Config& config,
    Acts::Logging::Level level)
    : WriterT(config.inputSimHits, "ObjSimHitWriter", level), m_cfg(config) {
  // inputSimHits is already checked by base constructor
  if (m_cfg.outputStem.empty()) {
    throw std::invalid_argument("Missing output filename stem");
  }
}

ActsExamples::ProcessCode ActsExamples::ObjSimHitWriter::writeT(
    const AlgorithmContext& ctx, const ActsExamples::SimHitContainer& simHits) {
  // ensure exclusive access to tree/file while writing
  std::scoped_lock lock(m_writeMutex);

  // open per-event file for all simhit components
  std::string pathSimHit = perEventFilepath(
      m_cfg.outputDir, m_cfg.outputStem + ".obj", ctx.eventNumber);
  std::string pathSimTrajectory = perEventFilepath(
      m_cfg.outputDir, m_cfg.outputStem + "_trajectory.obj", ctx.eventNumber);

  std::ofstream osHits(pathSimHit, std::ofstream::out | std::ofstream::trunc);
  std::ofstream osTrajectory(pathSimTrajectory,
                             std::ofstream::out | std::ofstream::trunc);

  if (!osHits || !osTrajectory) {
    throw std::ios_base::failure("Could not open '" + pathSimHit + "' or '" +
                                 pathSimTrajectory + "' to write");
  }

  // Only hit plotting
  if (!m_cfg.drawConnections) {
    // Write data from internal immplementation
    for (const auto& simHit : simHits) {
      // local simhit information in global coord.
      const Acts::Vector4& globalPos4 = simHit.fourPosition();

      osHits << "v " << globalPos4[Acts::ePos0] / Acts::UnitConstants::mm << " "
             << globalPos4[Acts::ePos1] / Acts::UnitConstants::mm << " "
             << globalPos4[Acts::ePos2] / Acts::UnitConstants::mm << std::endl;

    }  // end simHit loop
  } else {
    // We need to associate first
    std::unordered_map<std::size_t, std::vector<Acts::Vector4>> particleHits;
    // Pre-loop over hits ... write those below threshold
    for (const auto& simHit : simHits) {
      double momentum = simHit.momentum4Before().head<3>().norm();
      if (momentum < m_cfg.momentumThreshold) {
        ACTS_VERBOSE("Skipping: Hit below threshold: " << momentum);
        continue;
      } else if (momentum < m_cfg.momentumThresholdTraj) {
        ACTS_VERBOSE(
            "Skipping (trajectory): Hit below threshold: " << momentum);
        osHits << "v "
               << simHit.fourPosition()[Acts::ePos0] / Acts::UnitConstants::mm
               << " "
               << simHit.fourPosition()[Acts::ePos1] / Acts::UnitConstants::mm
               << " "
               << simHit.fourPosition()[Acts::ePos2] / Acts::UnitConstants::mm
               << std::endl;
        continue;
      }
      ACTS_VERBOSE("Accepting: Hit above threshold: " << momentum);

      if (particleHits.find(simHit.particleId().value()) ==
          particleHits.end()) {
        particleHits[simHit.particleId().value()] = {};
      }
      particleHits[simHit.particleId().value()].push_back(
          simHit.fourPosition());
    }
    // Draw loop
    std::size_t lOffset = 1;
    for (auto& [pId, pHits] : particleHits) {
      // Draw the particle hits
      std::sort(pHits.begin(), pHits.end(),
                [](const Acts::Vector4& a, const Acts::Vector4& b) {
                  return a[Acts::eTime] < b[Acts::eTime];
                });

      osHits << "o particle_" << pId << std::endl;
      for (const auto& hit : pHits) {
        osHits << "v " << hit[Acts::ePos0] / Acts::UnitConstants::mm << " "
               << hit[Acts::ePos1] / Acts::UnitConstants::mm << " "
               << hit[Acts::ePos2] / Acts::UnitConstants::mm << std::endl;
      }
      osHits << '\n';

      // Interpolate the points, a minimum number of 3 hits is necessary for
      // that
      std::vector<Acts::Vector4> trajectory;
      if (pHits.size() < 3 || m_cfg.nInterpolatedPoints == 0) {
        trajectory = pHits;
      } else {
        // The total number of points is the number of hits times the number of
        // interpolated points plus the number of hits
        trajectory = Acts::Interpolation3D::spline(
            pHits, pHits.size() * (m_cfg.nInterpolatedPoints + 1) - 1,
            m_cfg.keepOriginalHits);
      }

      osTrajectory << "o particle_trajectory_" << pId << std::endl;
      for (const auto& hit : trajectory) {
        osTrajectory << "v " << hit[Acts::ePos0] / Acts::UnitConstants::mm
                     << " " << hit[Acts::ePos1] / Acts::UnitConstants::mm << " "
                     << hit[Acts::ePos2] / Acts::UnitConstants::mm << std::endl;
      }
      // Draw the line
      for (std::size_t iv = lOffset + 1; iv < lOffset + trajectory.size();
           ++iv) {
        osTrajectory << "l " << iv - 1 << " " << iv << '\n';
      }
      osTrajectory << '\n';
      // Increase the offset count
      lOffset += trajectory.size();
    }
  }
  osHits.close();
  osTrajectory.close();

  return ActsExamples::ProcessCode::SUCCESS;
}

ActsExamples::ProcessCode ActsExamples::ObjSimHitWriter::finalize() {
  return ActsExamples::ProcessCode::SUCCESS;
}
