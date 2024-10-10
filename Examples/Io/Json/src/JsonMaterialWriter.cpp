// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/Json/JsonMaterialWriter.hpp"

#include "Acts/Utilities/Helpers.hpp"

#include <fstream>
#include <iomanip>
#include <ios>
#include <vector>

#include <nlohmann/json.hpp>

ActsExamples::JsonMaterialWriter::JsonMaterialWriter(
    const ActsExamples::JsonMaterialWriter::Config& config,
    Acts::Logging::Level level)
    : m_logger{Acts::getDefaultLogger("JsonMaterialWriter", level)},
      m_cfg(config),
      m_converter{std::make_unique<Acts::MaterialMapJsonConverter>(
          m_cfg.converterCfg, level)} {}

ActsExamples::JsonMaterialWriter::~JsonMaterialWriter() = default;

void ActsExamples::JsonMaterialWriter::writeMaterial(
    const Acts::DetectorMaterialMaps& detMaterial) {
  // Evoke the converter
  auto jOut = m_converter->materialMapsToJson(detMaterial);
  // And write the file(s)
  if (ACTS_CHECK_BIT(m_cfg.writeFormat, ActsExamples::JsonFormat::Json)) {
    std::string fileName = m_cfg.fileName + ".json";
    ACTS_VERBOSE("Writing to file: " << fileName);
    std::ofstream ofj(fileName);
    ofj << std::setw(4) << jOut << std::endl;
  }
  if (ACTS_CHECK_BIT(m_cfg.writeFormat, ActsExamples::JsonFormat::Cbor)) {
    std::vector<std::uint8_t> cborOut = nlohmann::json::to_cbor(jOut);
    std::string fileName = m_cfg.fileName + ".cbor";
    ACTS_VERBOSE("Writing to file: " << fileName);
    std::ofstream ofj(fileName, std::ios::out | std::ios::binary);
    ofj.write(reinterpret_cast<char*>(cborOut.data()), cborOut.size());
  }
}

void ActsExamples::JsonMaterialWriter::write(
    const Acts::TrackingGeometry& tGeometry) {
  // Evoke the converter
  auto jOut = m_converter->trackingGeometryToJson(tGeometry);
  // And write the file(s)
  if (ACTS_CHECK_BIT(m_cfg.writeFormat, ActsExamples::JsonFormat::Json)) {
    std::ofstream ofj(m_cfg.fileName + ".json");
    ofj << std::setw(4) << jOut << std::endl;
  }
  if (ACTS_CHECK_BIT(m_cfg.writeFormat, ActsExamples::JsonFormat::Cbor)) {
    std::vector<std::uint8_t> cborOut = nlohmann::json::to_cbor(jOut);
    std::ofstream ofj(m_cfg.fileName + ".cbor",
                      std::ios::out | std::ios::binary);
    ofj.write(reinterpret_cast<char*>(cborOut.data()), cborOut.size());
  }
}
