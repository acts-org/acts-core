// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Material/ISurfaceMaterial.hpp"
#include "Acts/Material/IVolumeMaterial.hpp"
#include "Acts/Plugins/Json/MaterialMapJsonConverter.hpp"
#include "Acts/Utilities/EnumBitwiseOperators.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/MaterialMapping/IMaterialWriter.hpp"

#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

namespace Acts {

class TrackingGeometry;
class ISurfaceMaterial;
class IVolumeMaterial;

using SurfaceMaterialMap =
    std::map<GeometryIdentifier, std::shared_ptr<const ISurfaceMaterial>>;

using VolumeMaterialMap =
    std::map<GeometryIdentifier, std::shared_ptr<const IVolumeMaterial>>;

using DetectorMaterialMaps = std::pair<SurfaceMaterialMap, VolumeMaterialMap>;
}  // namespace Acts

namespace ActsExamples {

enum class JsonFormat : std::uint8_t {
  NoOutput = 0,
  Json = 1,
  Cbor = 2,
  All = std::numeric_limits<std::uint8_t>::max()
};

ACTS_DEFINE_ENUM_BITWISE_OPERATORS(JsonFormat)

/// @class Json Material writer
///
/// @brief Writes out Detector material maps
/// using the Json Geometry converter
class JsonMaterialWriter : public IMaterialWriter {
 public:
  struct Config {
    /// The config class of the converter
    Acts::MaterialMapJsonConverter::Config converterCfg;
    /// Output file name
    std::string fileName = "material";
    /// Output format of the file
    JsonFormat writeFormat = JsonFormat::Json;
  };

  /// Constructor
  ///
  /// @param config The configuration struct of the writer
  /// @param level The log level
  JsonMaterialWriter(const Config& config, Acts::Logging::Level level);

  /// Virtual destructor
  ~JsonMaterialWriter() override;

  /// Write out the material map
  ///
  /// @param detMaterial is the SurfaceMaterial and VolumeMaterial maps
  void writeMaterial(const Acts::DetectorMaterialMaps& detMaterial) override;

  /// Write out the material map from Geometry
  ///
  /// @param tGeometry is the TrackingGeometry
  void write(const Acts::TrackingGeometry& tGeometry);

  /// Readonly access to the config
  const Config& config() const { return m_cfg; }

 private:
  const Acts::Logger& logger() const { return *m_logger; }

  /// The logger instance
  std::unique_ptr<const Acts::Logger> m_logger{nullptr};

  /// The config of the writer
  Config m_cfg;

  /// The material converter
  std::unique_ptr<Acts::MaterialMapJsonConverter> m_converter{nullptr};
};

}  // namespace ActsExamples
