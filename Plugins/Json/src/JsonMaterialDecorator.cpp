// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Json/JsonMaterialDecorator.hpp"

namespace Acts {

JsonMaterialDecorator::JsonMaterialDecorator(
    const MaterialMapJsonConverter::Config& rConfig,
    const std::string& jFileName, Acts::Logging::Level level,
    bool clearSurfaceMaterial, bool clearVolumeMaterial)
    : m_readerConfig(rConfig),
      m_clearSurfaceMaterial(clearSurfaceMaterial),
      m_clearVolumeMaterial(clearVolumeMaterial),
      m_logger{getDefaultLogger("JsonMaterialDecorator", level)} {
  // the material reader
  Acts::MaterialMapJsonConverter jmConverter(rConfig, level);

  ACTS_VERBOSE("Reading JSON material description from: " << jFileName);
  std::ifstream ifj(jFileName.c_str());
  if (!ifj.good()) {
    throw std::runtime_error{"Unable to open input JSON material file: " +
                             jFileName};
  }
  nlohmann::json jin;

  if (jFileName.find(".cbor") != std::string::npos) {
    std::vector<std::uint8_t> iCbor((std::istreambuf_iterator<char>(ifj)),
                                    std::istreambuf_iterator<char>());
    jin = nlohmann::json::from_cbor(iCbor);
  } else {
    ifj >> jin;
  }

  auto maps = jmConverter.jsonToMaterialMaps(jin);
  m_surfaceMaterialMap = maps.first;
  m_volumeMaterialMap = maps.second;
  ACTS_VERBOSE("JSON material description read complete");
}

void JsonMaterialDecorator::decorate(Surface& surface) const {
  ACTS_VERBOSE("Processing surface: " << surface.geometryId());
  // Clear the material if registered to do so
  if (m_clearSurfaceMaterial) {
    ACTS_VERBOSE("-> Clearing surface material");
    surface.assignSurfaceMaterial(nullptr);
  }
  // Try to find the surface in the map
  auto sMaterial = m_surfaceMaterialMap.find(surface.geometryId());
  if (sMaterial != m_surfaceMaterialMap.end()) {
    ACTS_VERBOSE("-> Found material for surface, assigning");
    surface.assignSurfaceMaterial(sMaterial->second);
  }
}

/// Decorate a TrackingVolume
///
/// @param volume the non-cost volume that is decorated
void JsonMaterialDecorator::decorate(TrackingVolume& volume) const {
  ACTS_VERBOSE("Processing volume: " << volume.geometryId());
  // Clear the material if registered to do so
  if (m_clearVolumeMaterial) {
    ACTS_VERBOSE("-> Clearing volume material");
    volume.assignVolumeMaterial(nullptr);
  }
  // Try to find the volume in the map
  auto vMaterial = m_volumeMaterialMap.find(volume.geometryId());
  if (vMaterial != m_volumeMaterialMap.end()) {
    ACTS_VERBOSE("-> Found material for volume, assigning");
    volume.assignVolumeMaterial(vMaterial->second);
  }
}
}  // namespace Acts
