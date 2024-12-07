// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/Json/JsonDigitizationConfig.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Utilities/BinningData.hpp"
#include "ActsExamples/Digitization/Smearers.hpp"
#include "ActsExamples/Framework/RandomNumbers.hpp"
#include "ActsFatras/Digitization/UncorrelatedHitSmearer.hpp"

#include <cstddef>
#include <fstream>
#include <initializer_list>
#include <stdexcept>
#include <utility>
#include <vector>

namespace ActsExamples {
namespace {
void to_json(nlohmann::json& j, const ActsFatras::SingleParameterSmearFunction<
                                    ActsExamples::RandomEngine>& f) {
  // Gauss:
  auto gauss = f.target<const Digitization::Gauss>();
  if (gauss != nullptr) {
    j["type"] = "Gauss";
    j["mean"] = 0;
    j["stddev"] = gauss->sigma;
    return;
  }
  // Truncated gauss:
  auto gaussT = f.target<const Digitization::GaussTrunc>();
  if (gaussT != nullptr) {
    j["type"] = "GaussTrunc";
    j["mean"] = 0;
    j["stddev"] = gaussT->sigma;
    j["range"] = gaussT->range;
    return;
  }
  // Clipped gauss:
  auto gaussC = f.target<const Digitization::GaussClipped>();
  if (gaussC != nullptr) {
    j["type"] = "GaussClipped";
    j["mean"] = 0;
    j["stddev"] = gaussC->sigma;
    j["range"] = gaussC->range;
    j["max_attempts"] = gaussC->maxAttemps;
    return;
  }
  // Uniform
  auto uniform = f.target<const Digitization::Uniform>();
  if (uniform != nullptr) {
    j["type"] = "Uniform";
    j["bindata"] = nlohmann::json(uniform->binningData);
    return;
  }
  // Digital
  auto digital = f.target<const Digitization::Digital>();
  if (digital != nullptr) {
    j["type"] = "Digitial";
    j["bindata"] = nlohmann::json(digital->binningData);
    return;
  }
  // Exact
  auto exact = f.target<const Digitization::Exact>();
  if (exact != nullptr) {
    j["type"] = "Exact";
    j["stddev"] = exact->sigma;
    return;
  }

  throw std::runtime_error("Unable to serialize smearer");
}

void from_json(
    const nlohmann::json& j,
    ActsFatras::SingleParameterSmearFunction<ActsExamples::RandomEngine>& f) {
  std::string sType = j["type"];

  if (sType == "Gauss") {
    f = Digitization::Gauss(j["stddev"]);
  } else if (sType == "GaussTrunc") {
    double sigma = j["stddev"];
    std::pair<double, double> range = j["range"];
    f = Digitization::GaussTrunc(sigma, range);
  } else if (sType == "GaussClipped") {
    double sigma = j["stddev"];
    std::pair<double, double> range = j["range"];
    f = Digitization::GaussClipped(sigma, range);
  } else if (sType == "Uniform") {
    Acts::BinningData bd;
    from_json(j["bindata"], bd);
    f = Digitization::Uniform(bd);
  } else if (sType == "Digitial") {
    Acts::BinningData bd;
    from_json(j["bindata"], bd);
    f = Digitization::Digital(bd);
  } else if (sType == "Exact") {
    f = Digitization::Exact(j["stddev"]);
  } else {
    throw std::invalid_argument("Unknown smearer type '" + sType + "'");
  }
}

}  // namespace
}  // namespace ActsExamples

void ActsExamples::to_json(nlohmann::json& j,
                           const ActsExamples::ParameterSmearingConfig& psc) {
  j["index"] = psc.index;
  j["forcePositiveValues"] = psc.forcePositiveValues;
  to_json(j, psc.smearFunction);
}

void ActsExamples::from_json(const nlohmann::json& j,
                             ActsExamples::ParameterSmearingConfig& psc) {
  psc.index = static_cast<Acts::BoundIndices>(j["index"]);
  if (j.find("forcePositiveValues") != j.end()) {
    psc.forcePositiveValues = j["forcePositiveValues"];
  }
  from_json(j, psc.smearFunction);
}

void ActsExamples::to_json(nlohmann::json& j,
                           const ActsExamples::GeometricConfig& gdc) {
  std::vector<std::size_t> indices;
  for (const auto& idx : gdc.indices) {
    indices.push_back(static_cast<std::size_t>(idx));
  }
  j["indices"] = indices;
  j["segmentation"] = nlohmann::json(gdc.segmentation);
  j["thickness"] = gdc.thickness;
  j["threshold"] = gdc.threshold;
  j["digital"] = gdc.digital;
  if (j.find("charge-smearing") != j.end()) {
    to_json(j["charge-smearing"], gdc.chargeSmearer);
  }
}

void ActsExamples::from_json(const nlohmann::json& j,
                             ActsExamples::GeometricConfig& gdc) {
  for (const auto& jidx : j["indices"]) {
    gdc.indices.push_back(static_cast<Acts::BoundIndices>(jidx));
  }
  from_json(j["segmentation"], gdc.segmentation);
  gdc.thickness = j["thickness"];
  gdc.threshold = j["threshold"];
  gdc.digital = j["digital"];
  if (j.find("variances") != j.end()) {
    /// Read the variances from the json file
    auto jvariances = j["variances"];
    for (const auto& jvar : jvariances) {
      auto idx =
          static_cast<Acts::BoundIndices>(jvar["index"].get<std::size_t>());
      auto vars = jvar["rms"].get<std::vector<double>>();
      gdc.varianceMap[idx] = vars;
    }
  }
  if (j.find("charge-smearing") != j.end()) {
    from_json(j["charge-smearing"], gdc.chargeSmearer);
  }
}

void ActsExamples::to_json(nlohmann::json& j,
                           const ActsExamples::SmearingConfig& sdc) {
  for (const auto& sc : sdc) {
    j.push_back(nlohmann::json(sc));
  }
}

void ActsExamples::from_json(const nlohmann::json& j,
                             ActsExamples::SmearingConfig& sdc) {
  for (const auto& jpsc : j) {
    ActsExamples::ParameterSmearingConfig psc;
    from_json(jpsc, psc);
    sdc.push_back(psc);
  }
}

void ActsExamples::to_json(nlohmann::json& j,
                           const ActsExamples::DigiComponentsConfig& dc) {
  if (!dc.geometricDigiConfig.indices.empty()) {
    j["geometric"] = nlohmann::json(dc.geometricDigiConfig);
  }
  if (!dc.smearingDigiConfig.empty()) {
    j["smearing"] = nlohmann::json(dc.smearingDigiConfig);
  }
}

void ActsExamples::from_json(const nlohmann::json& j,
                             ActsExamples::DigiComponentsConfig& dc) {
  if (j.find("geometric") != j.end()) {
    nlohmann::json jgdc = j["geometric"];
    from_json(jgdc, dc.geometricDigiConfig);
  }
  if (j.find("smearing") != j.end()) {
    nlohmann::json jsdc = j["smearing"];
    from_json(jsdc, dc.smearingDigiConfig);
  }
}

Acts::GeometryHierarchyMap<ActsExamples::DigiComponentsConfig>
ActsExamples::readDigiConfigFromJson(const std::string& path) {
  nlohmann::json djson;
  if (path.empty()) {
    return Acts::GeometryHierarchyMap<ActsExamples::DigiComponentsConfig>();
  }
  std::ifstream infile(path, std::ifstream::in | std::ifstream::binary);
  // rely on exception for error handling
  infile.exceptions(std::ofstream::failbit | std::ofstream::badbit);
  infile >> djson;
  return DigiConfigConverter("digitization-configuration").fromJson(djson);
}

void ActsExamples::writeDigiConfigToJson(
    const Acts::GeometryHierarchyMap<DigiComponentsConfig>& cfg,
    const std::string& path) {
  std::ofstream outfile(path, std::ofstream::out | std::ofstream::binary);
  // rely on exception for error handling
  outfile.exceptions(std::ofstream::failbit | std::ofstream::badbit);
  outfile << DigiConfigConverter("digitization-configuration")
                 .toJson(cfg, nullptr)
                 .dump(2);
}
