// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Digitization/DigitizationConfig.hpp"

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "ActsExamples/Digitization/SmearingConfig.hpp"

namespace {

enum SmearingTypes : int {
  eGauss = 0,
  eGaussTruncated = 1,
  eGaussClipped = 2,
  eUniform = 3,
  eDigital = 4,
};

}  // namespace

ActsExamples::DigitizationConfig::DigitizationConfig(
    bool merge, double sigma, bool commonCorner,
    Acts::GeometryHierarchyMap<DigiComponentsConfig>&& digiCfgs)
    : doMerge(merge), mergeNsigma(sigma), mergeCommonCorner(commonCorner) {
  digitizationConfigs = std::move(digiCfgs);
}

ActsExamples::DigitizationConfig::DigitizationConfig(
    Acts::GeometryHierarchyMap<DigiComponentsConfig>&& digiCfgs)
    : doMerge(false), mergeNsigma(1.0), mergeCommonCorner(false) {
  digitizationConfigs = std::move(digiCfgs);
}

std::vector<
    std::pair<Acts::GeometryIdentifier, std::vector<Acts::BoundIndices>>>
ActsExamples::DigitizationConfig::getBoundIndices() const {
  std::vector<
      std::pair<Acts::GeometryIdentifier, std::vector<Acts::BoundIndices>>>
      bIndexInput;

  for (std::size_t ibi = 0; ibi < digitizationConfigs.size(); ++ibi) {
    Acts::GeometryIdentifier geoID = digitizationConfigs.idAt(ibi);
    const auto dCfg = digitizationConfigs.valueAt(ibi);
    std::vector<Acts::BoundIndices> boundIndices;
    boundIndices.insert(boundIndices.end(),
                        dCfg.geometricDigiConfig.indices.begin(),
                        dCfg.geometricDigiConfig.indices.end());
    // we assume nobody will add multiple smearers to a single bound index
    for (const auto& c : dCfg.smearingDigiConfig) {
      boundIndices.push_back(c.index);
    }
    bIndexInput.push_back({geoID, boundIndices});
  }
  return bIndexInput;
}

std::vector<Acts::ActsScalar> ActsExamples::GeometricConfig::variances(
    const std::array<std::size_t, 2u>& csizes,
    const std::array<std::size_t, 2u>& cmins) const {
  std::vector<Acts::ActsScalar> rVariances;
  for (const auto& bIndex : indices) {
    Acts::ActsScalar var = 0.;
    if (varianceMap.contains(bIndex)) {
      // Try to find the variance for this cluster size
      std::size_t lsize =
          std::min(csizes[bIndex], varianceMap.at(bIndex).size());
      var = varianceMap.at(bIndex).at(lsize - 1);
    } else {
      // Pitch size ofer / sqrt(12) as error instead
      std::size_t ictr = cmins[bIndex] + csizes[bIndex] / 2;
      var = std::pow(segmentation.binningData()[bIndex].width(ictr), 2) / 12.0;
    }
    rVariances.push_back(var);
  }
  return rVariances;
}
