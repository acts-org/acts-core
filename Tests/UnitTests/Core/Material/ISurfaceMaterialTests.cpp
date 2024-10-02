// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Common.hpp"
#include "Acts/Definitions/Direction.hpp"
#include "Acts/Material/ISurfaceMaterial.hpp"
#include "Acts/Material/MaterialSlab.hpp"

#include <cstddef>
#include <ostream>

namespace Acts::Test {

class SurfaceMaterialStub : public ISurfaceMaterial {
  using ISurfaceMaterial::ISurfaceMaterial;

  ISurfaceMaterial& operator*=(double /*scale*/) override { return *this; };

  const MaterialSlab& materialSlab(const Vector2& /*lp*/) const override {
    return m_fullMaterial;
  }

  const MaterialSlab& materialSlab(const Vector3& /*gp*/) const override {
    return m_fullMaterial;
  }

  std::ostream& toStream(std::ostream& sl) const override {
    sl << "SurfaceMaterialStub";
    return sl;
  };

  MaterialSlab m_fullMaterial{};
};

/// Test the constructors
BOOST_AUTO_TEST_CASE(ISurfaceMaterial_factor_test) {
  double splitFactor = 42.0;
  SurfaceMaterialStub stub{splitFactor};

  BOOST_CHECK_EQUAL(
      stub.factor(Direction::Forward, MaterialUpdateStage::FullUpdate), 1.0);

  BOOST_CHECK_EQUAL(
      stub.factor(Direction::Backward, MaterialUpdateStage::FullUpdate), 1.0);

  BOOST_CHECK_EQUAL(
      stub.factor(Direction::Forward, MaterialUpdateStage::PostUpdate),
      splitFactor);

  BOOST_CHECK_EQUAL(
      stub.factor(Direction::Backward, MaterialUpdateStage::PreUpdate),
      splitFactor);

  BOOST_CHECK_EQUAL(
      stub.factor(Direction::Forward, MaterialUpdateStage::PreUpdate),
      1 - splitFactor);

  BOOST_CHECK_EQUAL(
      stub.factor(Direction::Backward, MaterialUpdateStage::PostUpdate),
      1 - splitFactor);
}

}  // namespace Acts::Test
