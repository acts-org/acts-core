// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Seeding/SeedConfirmationRangeConfig.hpp"
#include "Acts/Utilities/Delegate.hpp"

#include <memory>

namespace Acts {
// forward declaration to avoid cyclic dependence
template <typename T>
class SeedFilter;

/// @brief Structure that holds configuration parameters for the orthogonal seed finder algorithm
template <typename SpacePoint>
struct SeedFinderOrthogonalConfig {
  std::shared_ptr<Acts::SeedFilter<SpacePoint>> seedFilter;

  /// Seeding parameters for geometry settings and detector ROI

  // Limiting location of all measurements
  float phiMin = -M_PI;
  float phiMax = M_PI;
  /// limiting location of measurements
  float zMin = -2800 * Acts::UnitConstants::mm;
  float zMax = 2800 * Acts::UnitConstants::mm;
  float rMax = 600 * Acts::UnitConstants::mm;
  /// @warning If rMin is smaller than impactMax, the bin size will be 2*pi,
  /// which will make seeding very slow!
  float rMin = 33 * Acts::UnitConstants::mm;

  /// Seeding parameters used to define the region of interest for middle
  /// space-point

  /// Radial range for middle space-point
  /// The range can be defined manually with (rMinMiddle, rMaxMiddle). If
  /// useVariableMiddleSPRange is set to false and the vector rRangeMiddleSP is
  /// empty, we use (rMinMiddle, rMaxMiddle) to cut the middle space-points
  float rMinMiddle = 60.f * Acts::UnitConstants::mm;
  float rMaxMiddle = 120.f * Acts::UnitConstants::mm;
  /// If useVariableMiddleSPRange is set to false, the vector rRangeMiddleSP can
  /// be used to define a fixed r range for each z bin: {{rMin, rMax}, ...}
  bool useVariableMiddleSPRange = true;
  /// Range defined in vector for each z bin
  std::vector<std::vector<float>> rRangeMiddleSP;
  /// If useVariableMiddleSPRange is true, the radial range will be calculated
  /// based on the maximum and minimum r values of the space-points in the event
  /// and a deltaR (deltaRMiddleMinSPRange, deltaRMiddleMaxSPRange)
  float deltaRMiddleMinSPRange = 10. * Acts::UnitConstants::mm;
  float deltaRMiddleMaxSPRange = 10. * Acts::UnitConstants::mm;

  /// Vector containing minimum and maximum z boundaries for cutting middle
  /// space-points
  std::pair<float, float> zOutermostLayers{-2700 * Acts::UnitConstants::mm,
                                           2700 * Acts::UnitConstants::mm};

  /// Seeding parameters used to define the cuts on space-point doublets

  /// Minimum radial distance between middle-outer doublet components
  float deltaRMinTopSP = std::numeric_limits<float>::quiet_NaN();
  /// Maximum radial distance between middle-outer doublet components
  float deltaRMaxTopSP = std::numeric_limits<float>::quiet_NaN();
  /// Minimum radial distance between inner-middle doublet components
  float deltaRMinBottomSP = std::numeric_limits<float>::quiet_NaN();
  /// Maximum radial distance between inner-middle doublet components
  float deltaRMaxBottomSP = std::numeric_limits<float>::quiet_NaN();

  /// Shrink the phi range of middle space-point (analogous to phi bin size in
  /// grid from default seeding + number of phi bins used in search)
  float deltaPhiMax = 0.085;

  /// Maximum value of z-distance between space-points in doublet
  float deltaZMax =
      std::numeric_limits<float>::infinity() * Acts::UnitConstants::mm;

  /// Maximum allowed cotTheta between two space-points in doublet, used to
  /// check if forward angle is within bounds
  float cotThetaMax = 7.40627;  // equivalent to 2.7 eta (pseudorapidity)

  /// Limiting location of collision region in z-axis used to check if doublet
  /// origin is within reasonable bounds
  float collisionRegionMin = -150 * Acts::UnitConstants::mm;
  float collisionRegionMax = +150 * Acts::UnitConstants::mm;

  /// Enable cut on the compatibility between interaction point and doublet,
  /// this is an useful approximation to speed up the seeding
  bool interactionPointCut = false;

  /// Seeding parameters used to define the cuts on space-point triplets

  /// Minimum transverse momentum (pT) used to check the r-z slope compatibility
  /// of triplets with maximum multiple scattering effect (produced by the
  /// minimum allowed pT particle) + a certain uncertainty term. Check the
  /// documentation for more information
  /// https://acts.readthedocs.io/en/latest/core/reconstruction/pattern_recognition/seeding.html
  float minPt = 400. * Acts::UnitConstants::MeV;
  /// Number of sigmas of scattering angle to be considered in the minimum pT
  /// scattering term
  float sigmaScattering = 5;
  /// Term that accounts for the thickness of scattering medium in radiation
  /// lengths in the Lynch & Dahl correction to the Highland equation default is
  /// 5%
  /// TODO: necessary to make amount of material dependent on detector region?
  float radLengthPerSeed = 0.05;
  /// Maximum transverse momentum for scattering calculation
  float maxPtScattering = 10 * Acts::UnitConstants::GeV;
  /// Maximum value of impact parameter estimation of the seed candidates
  float impactMax = 20. * Acts::UnitConstants::mm;
  /// Parameter which can loosen the tolerance of the track seed to form a
  /// helix. This is useful for e.g. misaligned seeding.
  float helixCutTolerance = 1.;

  /// Seeding parameters used for quality seed confirmation

  /// Enable quality seed confirmation, this is different than default seeding
  /// confirmation because it can also be defined for different (r, z) regions
  /// of the detector (e.g. forward or central region) by SeedConfirmationRange.
  /// Seeds are classified as "high-quality" seeds and normal quality seeds.
  /// Normal quality seeds are only selected if no other "high-quality" seeds
  /// has been found for that inner-middle doublet.
  bool seedConfirmation = false;
  /// Contains parameters for central seed confirmation
  SeedConfirmationRangeConfig centralSeedConfirmationRange;
  /// Contains parameters for forward seed confirmation
  SeedConfirmationRangeConfig forwardSeedConfirmationRange;
  /// Maximum number (minus one) of accepted seeds per middle space-point
  unsigned int maxSeedsPerSpM = 5;

  /// Other parameters

  /// derived values, set on SeedFinder construction
  float highland = 0;
  float maxScatteringAngle2 = 0;

  // Delegate to apply experiment specific cuts
  Delegate<bool(float /*bottomRadius*/, float /*cotTheta*/)> experimentCuts{
      DelegateFuncTag<&noopExperimentCuts>{}};

  bool isInInternalUnits = false;

  SeedFinderOrthogonalConfig calculateDerivedQuantities() const {
    if (!isInInternalUnits) {
      throw std::runtime_error(
          "SeedFinderOrthogonalConfig not in ACTS internal units in "
          "calculateDerivedQuantities");
    }
    SeedFinderOrthogonalConfig config = *this;
    /// calculation of scattering using the highland formula
    /// convert pT to p once theta angle is known
    config.highland = 13.6 * std::sqrt(radLengthPerSeed) *
                      (1 + 0.038 * std::log(radLengthPerSeed));
    config.maxScatteringAngle2 = std::pow(config.highland / config.minPt, 2);
    return config;
  }

  SeedFinderOrthogonalConfig toInternalUnits() const {
    if (isInInternalUnits) {
      throw std::runtime_error(
          "SeedFinderOrthogonalConfig already in ACTS internal units in "
          "toInternalUnits");
    }
    using namespace Acts::UnitLiterals;
    SeedFinderOrthogonalConfig config = *this;
    config.isInInternalUnits = true;
    config.minPt /= 1_MeV;
    config.deltaRMinTopSP /= 1_mm;
    config.deltaRMaxTopSP /= 1_mm;
    config.deltaRMinBottomSP /= 1_mm;
    config.deltaRMaxBottomSP /= 1_mm;
    config.impactMax /= 1_mm;
    config.maxPtScattering /= 1_MeV;
    config.collisionRegionMin /= 1_mm;
    config.collisionRegionMax /= 1_mm;
    config.zMin /= 1_mm;
    config.zMax /= 1_mm;
    config.rMax /= 1_mm;
    config.rMin /= 1_mm;

    return config;
  }
};

}  // namespace Acts
