// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Surfaces/CurvilinearSurface.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"

#include <cmath>

namespace Acts::Test {

BOOST_AUTO_TEST_CASE(jacobian_test) {
  // (1a) Standard test with curvilinear not glazingly close to z axis
  Vector3 direction = Vector3(7., 8., 9.).normalized();
  CurvilinearSurface surface = CurvilinearSurface(direction);
  FreeToBoundMatrix f2cJacobian = surface.freeToBoundJacobian();

  ActsScalar phi = VectorHelpers::phi(direction);
  ActsScalar theta = VectorHelpers::theta(direction);
  ActsScalar sinPhi = std::sin(phi);
  ActsScalar cosPhi = std::cos(phi);
  ActsScalar sinTheta = std::sin(theta);
  ActsScalar cosTheta = std::cos(theta);

  CHECK_CLOSE_REL(f2cJacobian(eBoundLoc0, eFreePos0), -sinPhi, 1e-5);
  CHECK_CLOSE_REL(f2cJacobian(eBoundLoc0, eFreePos1), cosPhi, 1e-5);
  CHECK_CLOSE_REL(f2cJacobian(eBoundLoc1, eFreePos0), -cosPhi * cosTheta, 1e-5);
  CHECK_CLOSE_REL(f2cJacobian(eBoundLoc1, eFreePos1), -sinPhi * cosTheta, 1e-5);
  CHECK_CLOSE_REL(f2cJacobian(eBoundLoc1, eFreePos2), sinTheta, 1e-5);
  CHECK_CLOSE_REL(f2cJacobian(eBoundTime, eFreeTime), 1., 1e-5);
  CHECK_CLOSE_REL(f2cJacobian(eBoundPhi, eFreeDir0), -sinPhi / sinTheta, 1e-5);
  CHECK_CLOSE_REL(f2cJacobian(eBoundPhi, eFreeDir1), cosPhi / sinTheta, 1e-5);
  CHECK_CLOSE_REL(f2cJacobian(eBoundTheta, eFreeDir0), cosPhi * cosTheta, 1e-5);
  CHECK_CLOSE_REL(f2cJacobian(eBoundTheta, eFreeDir1), sinPhi * cosTheta, 1e-5);
  CHECK_CLOSE_REL(f2cJacobian(eBoundTheta, eFreeDir2), -sinTheta, 1e-5);
  CHECK_CLOSE_REL(f2cJacobian(eBoundQOverP, eFreeQOverP), 1., 1e-5);

  // (2a) Standard test with curvilinear not glazingly close to z axis
  direction = Vector3(7., 8., 9.).normalized();
  surface = CurvilinearSurface(direction);
  BoundToFreeMatrix c2fJacobian = surface.boundToFreeJacobian();

  phi = VectorHelpers::phi(direction);
  theta = VectorHelpers::theta(direction);
  sinPhi = std::sin(phi);
  cosPhi = std::cos(phi);
  sinTheta = std::sin(theta);
  cosTheta = std::cos(theta);

  CHECK_CLOSE_REL(c2fJacobian(eFreePos0, eBoundLoc0), -sinPhi, 1e-5);
  CHECK_CLOSE_REL(c2fJacobian(eFreePos0, eBoundLoc1), -cosPhi * cosTheta, 1e-5);
  CHECK_CLOSE_REL(c2fJacobian(eFreePos1, eBoundLoc0), cosPhi, 1e-5);
  CHECK_CLOSE_REL(c2fJacobian(eFreePos1, eBoundLoc1), -sinPhi * cosTheta, 1e-5);
  CHECK_CLOSE_REL(c2fJacobian(eFreePos2, eBoundLoc1), sinTheta, 1e-5);
  // Time parameter: stays as is
  CHECK_CLOSE_REL(c2fJacobian(eFreeTime, eBoundTime), 1, 1e-5);
  CHECK_CLOSE_REL(c2fJacobian(eFreeDir0, eBoundPhi), -sinTheta * sinPhi, 1e-5);
  CHECK_CLOSE_REL(c2fJacobian(eFreeDir0, eBoundTheta), cosTheta * cosPhi, 1e-5);
  CHECK_CLOSE_REL(c2fJacobian(eFreeDir1, eBoundPhi), sinTheta * cosPhi, 1e-5);
  CHECK_CLOSE_REL(c2fJacobian(eFreeDir1, eBoundTheta), cosTheta * sinPhi, 1e-5);
  CHECK_CLOSE_REL(c2fJacobian(eFreeDir2, eBoundTheta), -sinTheta, 1e-5);
  // Q/P parameter: stays as is
  CHECK_CLOSE_REL(c2fJacobian(eFreeQOverP, eBoundQOverP), 1, 1e-5);
}

}  // namespace Acts::Test
