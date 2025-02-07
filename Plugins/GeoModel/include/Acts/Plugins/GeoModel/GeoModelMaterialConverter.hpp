// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Material/Material.hpp"

class GeoMaterial;

namespace Acts::GeoModel {

/// @brief Convert GeoMaterial to Acts::Material
///
/// @param gm The GeoMaterial to be converted
/// @return the Acts::Material
Material geoMaterialConverter(const GeoMaterial& gm,
                              bool useMolarDensity = true);

}  // namespace Acts::GeoModel
