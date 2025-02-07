// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <memory>

#include <GeoModelRead/ReadGeoModel.h>

class GeoVPhysVol;

namespace Acts {

struct GeoModelTree {
  std::shared_ptr<GeoModelIO::ReadGeoModel> geoReader = nullptr;
  PVConstLink worldVolume = nullptr;
  std::string worldVolumeName = "World";
};

}  // namespace Acts
