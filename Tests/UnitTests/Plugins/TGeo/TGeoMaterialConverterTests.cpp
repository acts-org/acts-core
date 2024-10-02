// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Units.hpp"
#include "Acts/Plugins/TGeo/TGeoMaterialConverter.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"

#include <string>
#include <vector>

#include "TGeoManager.h"
#include "TGeoMaterial.h"

using namespace Acts::UnitLiterals;

namespace Acts::Test {

BOOST_AUTO_TEST_CASE(TGeoMaterialConverter_materialSlab) {
  new TGeoManager("gm", "garbage collector");

  ActsScalar A = 26.98;
  ActsScalar Z = 13.;
  TGeoMaterial *mat = new TGeoMaterial("Al", A, Z, 2.7);

  // ROOT calculates the radiation/int length in cm
  // That's actually depending on the ROOT version
  CHECK_CLOSE_ABS(mat->GetRadLen(), 8.85, 0.1_mm);
  CHECK_CLOSE_ABS(mat->GetIntLen(), 38.8, 0.1_mm);

  Acts::TGeoMaterialConverter::Options options;
  options.unitLengthScalor = 1_cm;
  options.unitMassScalor = 1.;

  // Assume we describe a 10 mm thick box as a 10 mm thick slab
  ActsScalar tInX0 = 10_mm / (mat->GetRadLen() * options.unitLengthScalor);
  ActsScalar tInL0 = 10_mm / (mat->GetIntLen() * options.unitLengthScalor);
  ActsScalar rho =
      2.7 * options.unitMassScalor / pow(options.unitLengthScalor, 3);

  Acts::MaterialSlab slab_10_10 =
      Acts::TGeoMaterialConverter::materialSlab(*mat, 10_mm, 10_mm, options);
  CHECK_CLOSE_ABS(88.7_mm, slab_10_10.material().X0(), 0.1_mm);
  CHECK_CLOSE_ABS(388_mm, slab_10_10.material().L0(), 1_mm);
  CHECK_CLOSE_ABS(A, slab_10_10.material().Ar(), 1e-5);
  CHECK_CLOSE_ABS(Z, slab_10_10.material().Z(), 1e-5);
  CHECK_CLOSE_ABS(tInX0, slab_10_10.thicknessInX0(), 1e-5);
  CHECK_CLOSE_ABS(tInL0, slab_10_10.thicknessInL0(), 1e-5);
  CHECK_CLOSE_ABS(rho, slab_10_10.material().massDensity(), 1e-5);

  // Assume we describe a 10 mm thick box as a 1 mm thick slab
  Acts::MaterialSlab slab_10_1 =
      Acts::TGeoMaterialConverter::materialSlab(*mat, 10_mm, 1_mm, options);
  // Radiation/interaction lengths are divided by 10
  CHECK_CLOSE_ABS(8.87_mm, slab_10_1.material().X0(), 0.1_mm);
  CHECK_CLOSE_ABS(38.8_mm, slab_10_1.material().L0(), 1_mm);
  // Density is scaled up by 10
  CHECK_CLOSE_ABS(10 * rho, slab_10_1.material().massDensity(), 1e-5);
  // A and Z remain unchanged
  CHECK_CLOSE_ABS(A, slab_10_1.material().Ar(), 1e-5);
  CHECK_CLOSE_ABS(Z, slab_10_1.material().Z(), 1e-5);

  // Thickness in X0/L0 is unchanged -> same scattering
  CHECK_CLOSE_ABS(tInX0, slab_10_1.thicknessInX0(), 1e-5);
  CHECK_CLOSE_ABS(tInL0, slab_10_1.thicknessInL0(), 1e-5);
  // Thickness * rho is unchanged -> same energy loss
  CHECK_CLOSE_ABS(slab_10_10.material().massDensity() * slab_10_10.thickness(),
                  slab_10_1.material().massDensity() * slab_10_1.thickness(),
                  1e-5);
}

}  // namespace Acts::Test
