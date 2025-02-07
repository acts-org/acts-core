// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Plugins/GeoModel/GeoModelReader.hpp"

#include <GeoModelDBManager/GMDBManager.h>
#include <GeoModelKernel/GeoFullPhysVol.h>
#include <GeoModelRead/ReadGeoModel.h>

Acts::GeoModelTree Acts::GeoModelReader::readFromDb(const std::string& dbPath) {
  // Data base manager
  GMDBManager* db = new GMDBManager(dbPath);
  if (!db->checkIsDBOpen()) {
    throw std::runtime_error("GeoModelReader: Could not open the database");
  }
  // Setup the GeoModel reader
  auto geoReader = std::make_shared<GeoModelIO::ReadGeoModel>(db);
  // Read the GeoModel
  GeoModelTree geoModel{geoReader, geoReader->buildGeoModel()};
  return geoModel;
}
