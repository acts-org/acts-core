// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Units.hpp"
#include "Acts/Material/Material.hpp"
#include "Acts/Material/MaterialSlab.hpp"

namespace Acts::Test {

inline Material makeBeryllium() {
  using namespace UnitLiterals;
  return Material::fromMolarDensity(35.28_cm, 42.10_cm, 9.012, 4,
                                    (1.848 / 9.012) * 1_mol / 1_cm3);
}

inline Material makeSilicon() {
  using namespace UnitLiterals;
  return Material::fromMolarDensity(9.370_cm, 46.52_cm, 28.0855, 14,
                                    (2.329 / 28.0855) * 1_mol / 1_cm3);
}

/// Build material slab corresponding to 1 radiation and interaction length.
inline MaterialSlab makeUnitSlab() {
  using namespace UnitLiterals;
  // silicon-like material with higher X0 and lower L0
  return {Material::fromMolarDensity(20_cm, 20_cm, 28.0855, 14,
                                     (2.329 / 28.0855) * 1_mol / 1_cm3),
          20_cm};
}

/// Build material slab corresponding to 1% of radiation and interaction length.
inline MaterialSlab makePercentSlab() {
  auto slab = makeUnitSlab();
  slab.scaleThickness(0.01);
  return slab;
}

}  // namespace Acts::Test
