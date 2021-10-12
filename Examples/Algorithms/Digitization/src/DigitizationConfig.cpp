// This file is part of the Acts project.
//
// Copyright (C) 2020-2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ActsExamples/Digitization/DigitizationConfig.hpp"

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/Digitization/DigitizationAlgorithm.hpp"
#include "ActsExamples/Digitization/Smearers.hpp"
#include "ActsExamples/Digitization/SmearingConfig.hpp"
#include "ActsExamples/Utilities/Options.hpp"

#include <numeric>
#include <string>

#include <boost/program_options.hpp>

namespace {

enum SmearingTypes : int {
  eGauss = 0,
  eGaussTruncated = 1,
  eGaussClipped = 2,
  eUniform = 3,
  eDigital = 4,
};

constexpr size_t numConfigParametersForType(int type) {
  // Gaussian smearing requires only a width/standard deviation parameter
  // Everything else requires a width/pitch and a range
  return (static_cast<SmearingTypes>(type) == eGauss) ? 1u : 3u;
}

}  // namespace

ActsExamples::DigitizationConfig::DigitizationConfig(
    const Options::Variables& vars,
    Acts::GeometryHierarchyMap<DigiComponentsConfig>&& digiCfgs)
    : doMerge(vars["digi-merge"].as<bool>()),
      mergeNsigma(vars["digi-merge-nsigma"].as<double>()),
      mergeCommonCorner(vars["digi-merge-common-corner"].as<bool>()) {
  digitizationConfigs = std::move(digiCfgs);
}

ActsExamples::DigitizationConfig::DigitizationConfig(
    Acts::GeometryHierarchyMap<DigiComponentsConfig>&& digiCfgs)
    : doMerge(false),
      mergeNsigma(1.0),
      mergeCommonCorner(false) {
  digitizationConfigs = std::move(digiCfgs);
}

std::vector<
    std::pair<Acts::GeometryIdentifier, std::vector<Acts::BoundIndices>>>
ActsExamples::DigitizationConfig::getBoundIndices() const {
  std::vector<
      std::pair<Acts::GeometryIdentifier, std::vector<Acts::BoundIndices>>>
      bIndexInput;

  for (size_t ibi = 0; ibi < digitizationConfigs.size(); ++ibi) {
    Acts::GeometryIdentifier geoID = digitizationConfigs.idAt(ibi);
    const auto dCfg = digitizationConfigs.valueAt(ibi);
    std::vector<Acts::BoundIndices> boundIndices;
    boundIndices.insert(boundIndices.end(),
			dCfg.geometricDigiConfig.indices.begin(),
			dCfg.geometricDigiConfig.indices.end());
    bIndexInput.push_back({geoID, boundIndices});
  }
  return bIndexInput;
}
