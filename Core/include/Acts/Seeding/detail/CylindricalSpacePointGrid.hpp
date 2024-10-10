// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Seeding/BinnedGroup.hpp"
#include "Acts/Seeding/SeedFinderConfig.hpp"
#include "Acts/Utilities/Grid.hpp"

#include <numbers>
#include <vector>

namespace Acts {

/// Cylindrical Space Point bin is a 2D grid with (phi, z) bins
/// It stores a vector of internal space points to external space points
template <typename external_spacepoint_t>
using CylindricalSpacePointGrid = Acts::Grid<
    std::vector<const external_spacepoint_t*>,
    Acts::Axis<Acts::AxisType::Equidistant, Acts::AxisBoundaryType::Closed>,
    Acts::Axis<Acts::AxisType::Variable, Acts::AxisBoundaryType::Bound>,
    Acts::Axis<Acts::AxisType::Variable, Acts::AxisBoundaryType::Bound>>;

/// Cylindrical Binned Group
template <typename external_spacepoint_t>
using CylindricalBinnedGroup =
    Acts::BinnedGroup<Acts::CylindricalSpacePointGrid<external_spacepoint_t>>;

template <typename external_spacepoint_t>
using CylindricalBinnedGroupIterator = Acts::BinnedGroupIterator<
    Acts::CylindricalSpacePointGrid<external_spacepoint_t>>;

struct CylindricalSpacePointGridConfig {
  // minimum pT to be found by seedFinder
  float minPt = 0 * Acts::UnitConstants::MeV;
  // maximum extension of sensitive detector layer relevant for seeding as
  // distance from x=y=0 (i.e. in r)
  float rMax = 320 * Acts::UnitConstants::mm;
  // maximum extension of sensitive detector layer relevant for seeding as
  // distance from x=y=0 (i.e. in r)
  float rMin = 0 * Acts::UnitConstants::mm;
  // maximum extension of sensitive detector layer relevant for seeding in
  // positive direction in z
  float zMax = 0 * Acts::UnitConstants::mm;
  // maximum extension of sensitive detector layer relevant for seeding in
  // negative direction in z
  float zMin = 0 * Acts::UnitConstants::mm;
  // maximum distance in r from middle space point to bottom or top spacepoint
  float deltaRMax = 0 * Acts::UnitConstants::mm;
  // maximum forward direction expressed as cot(theta)
  float cotThetaMax = 0;
  // maximum impact parameter in mm
  float impactMax = 0 * Acts::UnitConstants::mm;
  // minimum phi value for phiAxis construction
  float phiMin = -std::numbers::pi_v<float>;
  // maximum phi value for phiAxis construction
  float phiMax = std::numbers::pi_v<float>;
  // Multiplicator for the number of phi-bins. The minimum number of phi-bins
  // depends on min_pt, magnetic field: 2*M_PI/(minPT particle phi-deflection).
  // phiBinDeflectionCoverage is a multiplier for this number. If
  // numPhiNeighbors (in the configuration of the BinFinders) is configured to
  // return 1 neighbor on either side of the current phi-bin (and you want to
  // cover the full phi-range of minPT), leave this at 1.
  int phiBinDeflectionCoverage = 1;
  // maximum number of phi bins
  int maxPhiBins = 10000;
  // enable non equidistant binning in z
  std::vector<float> zBinEdges{};
  std::vector<float> rBinEdges{};
  bool isInInternalUnits = false;
  CylindricalSpacePointGridConfig toInternalUnits() const {
    if (isInInternalUnits) {
      throw std::runtime_error(
          "Repeated conversion to internal units for "
          "CylindricalSpacePointGridConfig");
    }

    using namespace Acts::UnitLiterals;
    CylindricalSpacePointGridConfig config = *this;
    config.isInInternalUnits = true;
    config.minPt /= 1_MeV;
    config.rMin /= 1_mm;
    config.rMax /= 1_mm;
    config.zMax /= 1_mm;
    config.zMin /= 1_mm;
    config.deltaRMax /= 1_mm;

    if (config.phiMin < -std::numbers::pi_v<float> ||
        config.phiMax > std::numbers::pi_v<float>) {
      throw std::runtime_error(
          "CylindricalSpacePointGridConfig: phiMin (" +
          std::to_string(config.phiMin) + ") and/or phiMax (" +
          std::to_string(config.phiMax) +
          ") are outside "
          "the allowed phi range, defined as "
          "[-std::numbers::pi_v<float>, std::numbers::pi_v<float>]");
    }
    if (config.phiMin > config.phiMax) {
      throw std::runtime_error(
          "CylindricalSpacePointGridConfig: phiMin is bigger then phiMax");
    }
    if (config.rMin > config.rMax) {
      throw std::runtime_error(
          "CylindricalSpacePointGridConfig: rMin is bigger then rMax");
    }
    if (config.zMin > config.zMax) {
      throw std::runtime_error(
          "CylindricalSpacePointGridConfig: zMin is bigger than zMax");
    }

    return config;
  }
};

struct CylindricalSpacePointGridOptions {
  // magnetic field
  float bFieldInZ = 0.;
  bool isInInternalUnits = false;
  CylindricalSpacePointGridOptions toInternalUnits() const {
    if (isInInternalUnits) {
      throw std::runtime_error(
          "Repeated conversion to internal units for "
          "CylindricalSpacePointGridOptions");
    }
    using namespace Acts::UnitLiterals;
    CylindricalSpacePointGridOptions options = *this;
    options.isInInternalUnits = true;
    options.bFieldInZ /= 1000_T;

    return options;
  }
};

/// Instructions on how to create and fill this grid specialization
class CylindricalSpacePointGridCreator {
 public:
  template <typename external_spacepoint_t>
  static Acts::CylindricalSpacePointGrid<external_spacepoint_t> createGrid(
      const Acts::CylindricalSpacePointGridConfig& _config,
      const Acts::CylindricalSpacePointGridOptions& _options);

  template <typename external_spacepoint_t,
            typename external_spacepoint_iterator_t>
  static void fillGrid(
      const Acts::SeedFinderConfig<external_spacepoint_t>& config,
      const Acts::SeedFinderOptions& options,
      Acts::CylindricalSpacePointGrid<external_spacepoint_t>& grid,
      external_spacepoint_iterator_t spBegin,
      external_spacepoint_iterator_t spEnd);
};

}  // namespace Acts

#include "Acts/Seeding/detail/CylindricalSpacePointGrid.ipp"
