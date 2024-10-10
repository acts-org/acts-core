// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/GeometryHierarchyMap.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/Result.hpp"
#include "ActsExamples/Digitization/DigitizationConfig.hpp"
#include "ActsExamples/Digitization/Smearers.hpp"
#include "ActsExamples/Digitization/SmearingConfig.hpp"
#include "ActsExamples/Framework/RandomNumbers.hpp"
#include "ActsFatras/Digitization/UncorrelatedHitSmearer.hpp"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Acts {
class GeometryIdentifier;
}  // namespace Acts

namespace ActsExamples {

/// Configuration struct for geometric digitization
///
/// If this is defined, then the geometric digitization
/// will create clusters with cells.
/// The BinUtility defines the segmentation and which parameters
/// are defined by this.
///
struct GeometricConfig {
  // The dimensions of the measurement
  std::vector<Acts::BoundIndices> indices = {};

  // The (multidimensional) binning definition for the segmentation of the
  // sensor
  Acts::BinUtility segmentation;

  // The thickness of the sensor
  double thickness = 0.;

  /// The charge smearer
  ActsFatras::SingleParameterSmearFunction<ActsExamples::RandomEngine>
      chargeSmearer = Digitization::Exact(0);

  // The threshold below a cell activation is ignored
  double threshold = 0.;

  // Whether to assume digital readout (activation is either 0 or 1)
  bool digital = false;

  // Flag as strip
  bool strip = false;

  /// The variances for this digitization
  std::map<Acts::BoundIndices, std::vector<Acts::ActsScalar>> varianceMap = {};

  /// Charge generation (configurable via the chargeSmearer)
  Acts::ActsScalar charge(Acts::ActsScalar path, RandomEngine &rng) const {
    if (!chargeSmearer) {
      return path;
    }
    auto res = chargeSmearer(path, rng);
    if (res.ok()) {
      return std::max(0.0, res->first);
    } else {
      throw std::runtime_error(res.error().message());
    }
  }

  /// This generates the variances for a given cluster
  ///
  /// @note either the variances are directly taken from a pre-read
  /// variance map, or they are generated from the pitch size
  ///
  /// @param csizes is the cluster size in the different dimensions
  /// @param cmins is the cluster minimum in the different dimensions
  ///
  /// @return a vector of variances for the cluster
  std::vector<Acts::ActsScalar> variances(
      const std::array<std::size_t, 2u> &csizes,
      const std::array<std::size_t, 2u> &cmins) const;

  /// Drift generation (currently not implemented)
  /// Takes as an argument the position, and a random engine
  ///  @return drift direction in local 3D coordinates
  Acts::Vector3 drift(const Acts::Vector3 & /*position*/,
                      RandomEngine & /*rng*/) const {
    return Acts::Vector3(0., 0., 0.);
  };
};

/// Configuration struct for the Digitization algorithm
///
/// It contains:
/// - optional GeometricConfig
/// - optional SmearingConfig
struct DigiComponentsConfig {
  GeometricConfig geometricDigiConfig;
  SmearingConfig smearingDigiConfig = {};
};

class DigitizationConfig {
 public:
  DigitizationConfig(bool merge, double sigma, bool commonCorner)
      : DigitizationConfig(merge, sigma, commonCorner,
                           Acts::GeometryHierarchyMap<DigiComponentsConfig>()) {
  }

  DigitizationConfig(
      bool doMerge, double mergeNsigma, bool mergeCommonCorner,
      Acts::GeometryHierarchyMap<DigiComponentsConfig> &&digiCfgs);

  DigitizationConfig(
      Acts::GeometryHierarchyMap<DigiComponentsConfig> &&digiCfgs);

  /// Input collection of simulated hits.
  std::string inputSimHits = "simhits";
  /// Output source links collection.
  std::string outputSourceLinks = "sourcelinks";
  /// Output measurements collection.
  std::string outputMeasurements = "measurements";
  /// Output cells map (geoID -> collection of cells).
  std::string outputCells = "cells";
  /// Output cluster collection.
  std::string outputClusters = "clusters";
  /// Output collection to map measured hits to contributing particles.
  std::string outputMeasurementParticlesMap = "measurement_particles_map";
  /// Output collection to map measured hits to simulated hits.
  std::string outputMeasurementSimHitsMap = "measurement_simhits_map";
  /// Map of surface by identifier to allow local - to global
  std::unordered_map<Acts::GeometryIdentifier, const Acts::Surface *>
      surfaceByIdentifier;
  /// Random numbers tool.
  std::shared_ptr<const RandomNumbers> randomNumbers = nullptr;
  /// Flag to determine whether cell data should be written to the
  /// `outputCells` collection; if true, writes (rather voluminous) cell data.
  bool doOutputCells = false;
  /// Flag to determine whether or not to run the clusterization; if true,
  /// clusters, measurements, and sim-hit-maps are output.
  bool doClusterization = true;
  /// Do we merge hits or not
  bool doMerge;
  /// How close do parameters have to be to consider merged
  const double mergeNsigma;
  /// Consider clusters that share a corner as merged (8-cell connectivity)
  const bool mergeCommonCorner;
  /// Energy deposit threshold for accepting a hit
  /// For a generic readout frontend we assume 1000 e/h pairs, in Si each
  /// e/h-pair requiers on average an energy of 3.65 eV (PDG  review 2023,
  /// Table 35.10)
  /// @NOTE The default is set to 0 because this works only well with Geant4
  double minEnergyDeposit = 0.0;  // 1000 * 3.65 * Acts::UnitConstants::eV;
  /// The digitizers per GeometryIdentifiers
  Acts::GeometryHierarchyMap<DigiComponentsConfig> digitizationConfigs;

  std::vector<
      std::pair<Acts::GeometryIdentifier, std::vector<Acts::BoundIndices>>>
  getBoundIndices() const;
};
}  // namespace ActsExamples
