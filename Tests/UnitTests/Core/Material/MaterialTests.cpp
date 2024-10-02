// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Units.hpp"
#include "Acts/Material/Material.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Tests/CommonHelpers/PredefinedMaterials.hpp"

#include <limits>

namespace bdata = boost::unit_test::data;

using namespace Acts::UnitLiterals;

static constexpr auto eps = 2 * std::numeric_limits<float>::epsilon();
// manually calculated derived values for silicon
static constexpr float SiNe = 1.160954941_mol / 1_cm3;
static constexpr float SiI = 172.042290036_eV;

BOOST_AUTO_TEST_SUITE(Material)

BOOST_AUTO_TEST_CASE(ConstructVacuum) {
  // default constructor builds invalid material a.k.a. vacuum
  Acts::Material vacuum;
  BOOST_CHECK(!vacuum.isValid());
}

BOOST_AUTO_TEST_CASE(ConstructSomething) {
  // anything with non-zero Ar is a valid material
  auto notVacuum = Acts::Material::fromMolarDensity(1, 2, 3, 4, 5);
  BOOST_CHECK(notVacuum.isValid());
}

BOOST_AUTO_TEST_CASE(Units) {
  Acts::Material silicon = Acts::Test::makeSilicon();

  // check values w/ different units if possible
  CHECK_CLOSE_REL(silicon.X0(), 93.70_mm, eps);
  CHECK_CLOSE_REL(silicon.X0(), 9.370_cm, eps);
  CHECK_CLOSE_REL(silicon.X0(), 0.09370_m, eps);
  CHECK_CLOSE_REL(silicon.L0(), 465.2_mm, eps);
  CHECK_CLOSE_REL(silicon.L0(), 46.52_cm, eps);
  CHECK_CLOSE_REL(silicon.L0(), 0.4652_m, eps);
  CHECK_CLOSE_REL(silicon.Ar(), 28.0855, eps);
  CHECK_CLOSE_REL(silicon.Z(), 14.0, eps);
  CHECK_CLOSE_REL(silicon.molarDensity(), 0.08292535294012926_mol / 1_cm3, eps);
  CHECK_CLOSE_REL(silicon.molarDensity(), 82925.35294012926_mol / 1_m3, eps);
  // check derived values
  CHECK_CLOSE_REL(silicon.massDensity(), 2.329_g / 1_cm3, eps);
  CHECK_CLOSE_REL(silicon.massDensity(), 0.002329_kg / 1_cm3, eps);
  CHECK_CLOSE_REL(silicon.massDensity(), 0.002329_g / 1_mm3, eps);
  CHECK_CLOSE_REL(silicon.molarElectronDensity(),
                  silicon.Z() * silicon.molarDensity(), eps);
  CHECK_CLOSE_REL(silicon.molarElectronDensity(), SiNe, eps);
  CHECK_CLOSE_REL(silicon.meanExcitationEnergy(), SiI, eps);
}

BOOST_DATA_TEST_CASE(EncodingDecodingRoundtrip,
                     bdata::make({
                         Acts::Material(),
                         Acts::Material::fromMolarDensity(1, 2, 3, 4, 5),
                         Acts::Test::makeBeryllium(),
                         Acts::Test::makeSilicon(),
                     }),
                     material) {
  // encode material
  Acts::Material::ParametersVector numbers0 = material.parameters();
  // construct from encoded numbers
  Acts::Material fromNumbers(numbers0);
  // encode material again
  Acts::Material::ParametersVector numbers1 = fromNumbers.parameters();

  BOOST_CHECK_EQUAL(material, fromNumbers);
  BOOST_CHECK_EQUAL(numbers0, numbers1);
}

BOOST_AUTO_TEST_SUITE_END()
