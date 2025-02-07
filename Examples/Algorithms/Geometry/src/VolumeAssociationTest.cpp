// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/Geometry/VolumeAssociationTest.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/Detector.hpp"
#include "Acts/Detector/DetectorVolume.hpp"

#include <exception>
#include <memory>
#include <numbers>
#include <string>
#include <vector>

ActsExamples::VolumeAssociationTest::VolumeAssociationTest(
    const Config& cfg, Acts::Logging::Level level)
    : IAlgorithm(cfg.name, level), m_cfg(cfg) {
  if (m_cfg.detector == nullptr) {
    throw std::invalid_argument("Missing detector object");
  }
  if (m_cfg.randomNumbers == nullptr) {
    throw std::invalid_argument("Missing random numbers tool");
  }
  if (m_cfg.randomRange.size() < 2) {
    throw std::invalid_argument(
        "Random range needs to be at least 2-dimensional");
  }
}

ActsExamples::ProcessCode ActsExamples::VolumeAssociationTest::execute(
    const AlgorithmContext& ctx) const {
  auto rng = m_cfg.randomNumbers->spawnGenerator(ctx);

  // Setup random number distributions for some quantities
  std::uniform_real_distribution<double> phiDist(-std::numbers::pi,
                                                 std::numbers::pi);
  std::uniform_real_distribution<double> rDist(0., m_cfg.randomRange[0u]);
  std::uniform_real_distribution<double> zDist(-m_cfg.randomRange[1u],
                                               m_cfg.randomRange[1u]);

  // Lemma for vector creation
  auto testPosition = [&]() -> Acts::Vector3 {
    double r = rDist(rng);
    double phi = phiDist(rng);
    double z = zDist(rng);
    return Acts::Vector3(r * cos(phi), r * sin(phi), z);
  };

  std::size_t failedSearch = 0;
  std::size_t failedAssignment = 0;
  for (std::size_t it = 0; it < m_cfg.ntests; ++it) {
    auto pos = testPosition();
    auto dv = m_cfg.detector->findDetectorVolume(ctx.geoContext, pos);
    if (dv == nullptr) {
      ++failedSearch;
    }
    if (!dv->inside(ctx.geoContext, pos)) {
      ++failedAssignment;
    }
  }
  if (failedSearch > 0) {
    ACTS_ERROR("Failed to find detector volume " << failedSearch << " times");
  }
  if (failedAssignment > 0) {
    ACTS_ERROR("Failed to assign detector volume " << failedAssignment
                                                   << " times");
  }

  return ProcessCode::SUCCESS;
}
