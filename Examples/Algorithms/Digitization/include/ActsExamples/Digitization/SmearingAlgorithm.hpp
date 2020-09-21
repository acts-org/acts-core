// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/EventData/Measurement.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Utilities/ParameterDefinitions.hpp"
#include "ActsExamples/EventData/DigitizedHit.hpp"
#include "ActsExamples/EventData/GeometryContainers.hpp"
#include "ActsExamples/Framework/BareAlgorithm.hpp"
#include "ActsExamples/Framework/RandomNumbers.hpp"
#include "ActsFatras/Digitization/UncorrelatedHitSmearer.hpp"

#include <array>
#include <memory>
#include <string>
#include <unordered_map>

namespace ActsExamples {

/// @brief Digitization Algorithm that turns simulated
/// hits into measuremetns for Fitting
class SmearingAlgorithm final : public BareAlgorithm {
 public:
  template <Acts::BoundIndices... kParameters>
  using Smearer = std::pair<ActsFatras::BoundParametersSmearer<kParameters...>,
                            std::array<ActsFatras::SmearFunction<RandomEngine>,
                                       sizeof...(kParameters)>>;

  /// Supported smears for this example are
  /// - strip type (either in loc0 or loc1)
  /// - pixel type (in loc0 and loc1)
  /// - pixel type with direction
  /// - all above in a timed flavor
  using SupportedSmearer = std::variant<
      Smearer<Acts::eBoundLoc0>, Smearer<Acts::eBoundLoc1>,
      Smearer<Acts::eBoundLoc0, Acts::eBoundLoc1>,
      Smearer<Acts::eBoundLoc0, Acts::eBoundLoc1, Acts::eBoundPhi,
              Acts::eBoundTheta>,
      Smearer<Acts::eBoundLoc0, Acts::eBoundTime>,
      Smearer<Acts::eBoundLoc1, Acts::eBoundTime>,
      Smearer<Acts::eBoundLoc0, Acts::eBoundLoc1, Acts::eBoundTime>,
      Smearer<Acts::eBoundLoc0, Acts::eBoundLoc1, Acts::eBoundPhi,
              Acts::eBoundTheta, Acts::eBoundTime>>;

  struct Config {
    /// Input collection of simulated hits
    std::string inputSimulatedHits;
    /// Output collection of measuremetns
    std::string outputMeasurements;
    /// Random numbers tool.
    std::shared_ptr<const RandomNumbers> randomNumbers = nullptr;
    /// The smearers per GeometryIdentifier
    ActsExamples::GeometryIdMultimap<SupportedSmearer> smearers;
  };

  /// Construct the digitization algorithm.
  ///
  /// @param cfg is the algorithm configuration
  /// @param lvl is the logging level
  SmearingAlgorithm(Config cfg, Acts::Logging::Level lvl);

  /// Build measurement from simulation hits at input
  ///
  /// @param txt is the algorithm context with event information
  /// @return a process code indication success or failure
  ProcessCode execute(const AlgorithmContext& ctx) const final override;

 private:
  /// The configuration struct containing the smearers
  Config m_cfg;
  /// All digitizable surfaces in the geometry
  GeometryIdMultimap<std::shared_ptr<const Acts::Surface>>
      m_digitizableSurfaces;

  /// Create a fittable measurmement from a parameter set
  ///
  /// @param pset The ParameterSet created from the smearer.
  /// @param surface The surface of the measurement.
  ///
  /// @return A Fittable measurement with a DigitizedHit source link
  template <Acts::BoundIndices... kParameters>
  Acts::FittableMeasurement<DigitizedHit> createMeasurement(
      Acts::ParameterSet<Acts::BoundIndices, kParameters...>&& pset,
      std::shared_ptr<const Acts::Surface> surface) const {
    DigitizedHit dhit(*surface);
    return Acts::Measurement<DigitizedHit, Acts::BoundIndices, kParameters...>(
        std::move(surface), std::move(dhit), std::move(pset));
  }
};

}  // namespace ActsExamples
