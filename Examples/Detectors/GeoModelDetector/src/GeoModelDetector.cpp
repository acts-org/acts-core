// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/GeoModelDetector/GeoModelDetector.hpp"

#include "Acts/Plugins/GeoModel/GeoModelReader.hpp"
#include "Acts/Plugins/GeoModel/GeoModelTree.hpp"

#include <G4GDMLParser.hh>

namespace ActsExamples {

GeoModelDetector::GeoModelDetector(const Config& cfg)
    : Detector(Acts::getDefaultLogger("GeoModelDetector", cfg.logLevel)),
      m_cfg(cfg) {
  m_geoModel = Acts::GeoModelReader::readFromDb(m_cfg.path);
}

}  // namespace ActsExamples
