// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Material/Material.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"

#include <limits>
#include <vector>

static constexpr auto eps = 2 * std::numeric_limits<float>::epsilon();

BOOST_AUTO_TEST_SUITE(material_properties)

BOOST_AUTO_TEST_CASE(construct_simple) {
  /// construct from material and thickness
  Acts::MaterialSlab fromMaterial(
      Acts::Material::fromMolarDensity(1., 2., 3., 4., 5.), 6.);

  CHECK_CLOSE_REL(fromMaterial.thickness(), 6., eps);
  CHECK_CLOSE_REL(fromMaterial.thicknessInX0(), 6., eps);
  CHECK_CLOSE_REL(fromMaterial.thicknessInL0(), 3., eps);
}

BOOST_AUTO_TEST_CASE(construct_compound) {
  using Acts::Material;
  using Acts::MaterialSlab;

  MaterialSlab a(Material::fromMolarDensity(1., 2., 3., 4., 5.), 1.);
  MaterialSlab b(Material::fromMolarDensity(2., 4., 6., 8., 10.), 2.);
  MaterialSlab c(Material::fromMolarDensity(4., 8., 12., 16., 20.), 3.);
  std::vector<MaterialSlab> components = {a, b, c};
  MaterialSlab abc = MaterialSlab::averageLayers(components);

  // consistency checks
  CHECK_CLOSE_REL(abc.thickness() / abc.material().X0(), abc.thicknessInX0(),
                  eps);
  CHECK_CLOSE_REL(abc.thickness() / abc.material().L0(), abc.thicknessInL0(),
                  eps);

  // absolute and relative thicknesses are additive
  CHECK_CLOSE_REL(abc.thickness(),
                  a.thickness() + b.thickness() + c.thickness(), eps);
  CHECK_CLOSE_REL(abc.thicknessInX0(),
                  a.thicknessInX0() + b.thicknessInX0() + c.thicknessInX0(),
                  eps);
  CHECK_CLOSE_REL(abc.thicknessInL0(),
                  a.thicknessInL0() + b.thicknessInL0() + c.thicknessInL0(),
                  eps);
  // The density scales with the thickness
  CHECK_CLOSE_REL(abc.material().massDensity(),
                  (a.thickness() * a.material().massDensity() +
                   b.thickness() * b.material().massDensity() +
                   c.thickness() * c.material().massDensity()) /
                      (a.thickness() + b.thickness() + c.thickness()),
                  eps);
}

BOOST_AUTO_TEST_CASE(scale_thickness) {
  const auto material = Acts::Material::fromMassDensity(1., 2., 3., 4., 5.);
  const Acts::MaterialSlab mat(material, 0.1);
  const Acts::MaterialSlab halfMat(material, 0.05);
  Acts::MaterialSlab halfScaled = mat;
  halfScaled.scaleThickness(0.5);

  BOOST_CHECK_NE(mat, halfMat);
  BOOST_CHECK_EQUAL(halfMat, halfScaled);
  CHECK_CLOSE_REL(mat.thicknessInX0(), 2 * halfMat.thicknessInX0(), eps);
  CHECK_CLOSE_REL(mat.thicknessInL0(), 2 * halfMat.thicknessInL0(), eps);
  CHECK_CLOSE_REL(mat.thickness() * mat.material().massDensity(),
                  2. * halfMat.thickness() * halfMat.material().massDensity(),
                  eps);
}

BOOST_AUTO_TEST_SUITE_END()
