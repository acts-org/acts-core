// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Common.hpp"
#include "Acts/Material/Material.hpp"
#include "Acts/Material/MaterialSlab.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

class TGeoMaterial;

namespace Acts {
struct TGeoMaterialConverter {
  /// @brief Nested options struct
  /// to steer the conversion process
  struct Options {
    /// @brief  Convert input TGeo unit to ACTS unit
    ActsScalar unitLengthScalor = 10.;
    /// @brief  Convert input TGeo unit to ACTS unit
    ActsScalar unitMassScalor = 1.;
  };

  /// @brief Helper method to convert a TGeoMaterial into Acts::MaterialSlab
  ///
  /// @param tgMaterial The TGeoMaterial to be converted
  /// @param thicknessIn The thickness of the ingoing material slab
  /// @param thicknessOut The thickness of the outgoing material slab
  /// @param options The conversion options with the unit scalors
  ///
  /// @return a material slab object
  static MaterialSlab materialSlab(const TGeoMaterial& tgMaterial,
                                   ActsScalar thicknessIn,
                                   ActsScalar thicknessOut,
                                   const Options& options);
};
}  // namespace Acts
