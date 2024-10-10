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
#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/GenericCurvilinearTrackParameters.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/MagneticField/ConstantBField.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Propagator/AtlasStepper.hpp"
#include "Acts/Propagator/EigenStepper.hpp"
#include "Acts/Surfaces/CurvilinearSurface.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/PerigeeSurface.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/StrawSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

using namespace Acts::UnitLiterals;

namespace Acts::Test {

using BFieldType = ConstantBField;
using EigenStepperType = EigenStepper<>;
using AtlasStepperType = AtlasStepper;
using Covariance = BoundSquareMatrix;

// Create a test context
GeometryContext tgContext = GeometryContext();
MagneticFieldContext mfContext = MagneticFieldContext();

static auto bField = std::make_shared<BFieldType>(Vector3{0, 0, 1_T});

/// Helper method to create a transform for a plane
/// to mimic detector situations, the plane is roughly
/// perpendicular to the track
///
/// @param nnomal The nominal normal direction
/// @param angleT Rotation around the norminal normal
/// @param angleU Rotation around the original U axis
Transform3 createCylindricTransform(const Vector3& nposition, double angleX,
                                    double angleY) {
  Transform3 ctransform;
  ctransform.setIdentity();
  ctransform.pretranslate(nposition);
  ctransform.prerotate(AngleAxis3(angleX, Vector3::UnitX()));
  ctransform.prerotate(AngleAxis3(angleY, Vector3::UnitY()));
  return ctransform;
}

/// Helper method to create a transform for a plane
/// to mimic detector situations, the plane is roughly
/// perpendicular to the track
///
/// @param nnomal The nominal normal direction
/// @param angleT Rotation around the norminal normal
/// @param angleU Rotation around the original U axis
Transform3 createPlanarTransform(const Vector3& nposition,
                                 const Vector3& nnormal, double angleT,
                                 double angleU) {
  // the rotation of the destination surface
  Vector3 T = nnormal.normalized();
  Vector3 U = std::abs(T.dot(Vector3::UnitZ())) < 0.99
                  ? Vector3::UnitZ().cross(T).normalized()
                  : Vector3::UnitX().cross(T).normalized();
  Vector3 V = T.cross(U);
  // that's the plane curvilinear Rotation
  RotationMatrix3 curvilinearRotation;
  curvilinearRotation.col(0) = U;
  curvilinearRotation.col(1) = V;
  curvilinearRotation.col(2) = T;
  // curvilinear surfaces are boundless
  Transform3 ctransform{curvilinearRotation};
  ctransform.pretranslate(nposition);
  ctransform.prerotate(AngleAxis3(angleT, T));
  ctransform.prerotate(AngleAxis3(angleU, U));
  //
  return ctransform;
}

/// Helper method : convert into Acts matrix
/// It takes the double array from AtlasStepper
/// and transforms it into an ActsMatrixD
///
/// @param P is the pointer to the array
///
/// Translation is (for lookup)
///                   /dL0    /dL1    /dPhi   /dThe   /dCM   /dT
/// X  ->P[0]  dX /   P[ 8]   P[16]   P[24]   P[32]   P[40]  P[48]
/// Y  ->P[1]  dY /   P[ 9]   P[17]   P[25]   P[33]   P[41]  P[49]
/// Z  ->P[2]  dZ /   P[10]   P[18]   P[26]   P[34]   P[42]  P[50]
/// T  ->P[3]  dT/	  P[11]   P[19]   P[27]   P[35]   P[43]  P[51]
/// Ax ->P[4]  dAx/   P[12]   P[20]   P[28]   P[36]   P[44]  P[52]
/// Ay ->P[5]  dAy/   P[13]   P[21]   P[29]   P[37]   P[45]  P[53]
/// Az ->P[6]  dAz/   P[14]   P[22]   P[30]   P[38]   P[46]  P[54]
/// CM ->P[7]  dCM/   P[15]   P[23]   P[31]   P[39]   P[47]  P[55]

BoundToFreeMatrix convertToMatrix(const std::array<double, 60> P) {
  // initialize to zero
  BoundToFreeMatrix jMatrix = BoundToFreeMatrix::Zero();
  for (std::size_t j = 0; j < eBoundSize; ++j) {
    for (std::size_t i = 0; i < eFreeSize; ++i) {
      std::size_t ijc = eFreeSize + j * eFreeSize + i;
      jMatrix(i, j) = P[ijc];
    }
  }
  return jMatrix;
}

/// Helper method : tests the jacobian to Global
/// for a templated Parameters object
///
/// @tparam Parameters the parameter type
/// @param pars the parameter object
template <typename Parameters>
void testJacobianToGlobal(const Parameters& pars) {
  // Jacobian creation for Propagator/Steppers
  // a) ATLAS stepper
  AtlasStepperType::State astepState(tgContext, bField->makeCache(mfContext),
                                     pars);
  // b) Eigen stepper
  EigenStepperType::State estepState(tgContext, bField->makeCache(mfContext),
                                     pars);

  // create the matrices
  auto asMatrix = convertToMatrix(astepState.pVector);

  // cross comparison checks
  CHECK_CLOSE_OR_SMALL(asMatrix, estepState.jacToGlobal, 1e-6, 1e-9);
}

/// This tests the jacobian of local curvilinear -> global
BOOST_AUTO_TEST_CASE(JacobianCurvilinearToGlobalTest) {
  // Create curvilinear parameters
  Covariance cov;
  cov << 10_mm, 0, 0, 0, 0, 0, 0, 10_mm, 0, 0, 0, 0, 0, 0, 0.1, 0, 0, 0, 0, 0,
      0, 0.1, 0, 0, 0, 0, 0, 0, 1. / (10_GeV), 0, 0, 0, 0, 0, 0, 0;
  CurvilinearTrackParameters curvilinear(Vector4(341., 412., 93., 0.),
                                         Vector3(1.2, 8.3, 0.45), 1 / 10.0, cov,
                                         ParticleHypothesis::pion());

  // run the test
  testJacobianToGlobal(curvilinear);
}

/// This tests the jacobian of local cylinder -> global
BOOST_AUTO_TEST_CASE(JacobianCylinderToGlobalTest) {
  // the cylinder transform and surface
  auto cTransform = createCylindricTransform({10., -5., 0.}, 0.004, 0.03);
  auto cSurface = Surface::makeShared<CylinderSurface>(cTransform, 200., 1000.);

  Covariance cov;
  cov << 10_mm, 0, 0, 0, 0, 0, 0, 10_mm, 0, 0, 0, 0, 0, 0, 0.1, 0, 0, 0, 0, 0,
      0, 0.1, 0, 0, 0, 0, 0, 0, 1. / (10_GeV), 0, 0, 0, 0, 0, 0, 0;

  BoundVector pars;
  pars << 182.34, -82., 0.134, 0.85, 1. / (100_GeV), 0;

  BoundTrackParameters atCylinder(cSurface, pars, std::move(cov),
                                  ParticleHypothesis::pion());

  // run the test
  testJacobianToGlobal(atCylinder);
}

/// This tests the jacobian of local disc -> global
BOOST_AUTO_TEST_CASE(JacobianDiscToGlobalTest) {
  // the disc transform and surface
  auto dTransform = createPlanarTransform(
      {10., -5., 0.}, Vector3(0.23, 0.07, 1.).normalized(), 0.004, 0.03);
  auto dSurface = Surface::makeShared<DiscSurface>(dTransform, 200., 1000.);

  Covariance cov;
  cov << 10_mm, 0, 0, 0, 0, 0, 0, 10_mm, 0, 0, 0, 0, 0, 0, 0.1, 0, 0, 0, 0, 0,
      0, 0.1, 0, 0, 0, 0, 0, 0, 1. / (10_GeV), 0, 0, 0, 0, 0, 0, 0;

  BoundVector pars;
  pars << 192.34, 1.823, 0.734, 0.235, 1. / (100_GeV), 0;

  BoundTrackParameters atDisc(dSurface, pars, std::move(cov),
                              ParticleHypothesis::pion());

  // run the test
  testJacobianToGlobal(atDisc);
}

/// This tests the jacobian of local plane -> global
BOOST_AUTO_TEST_CASE(JacobianPlaneToGlobalTest) {
  // Let's create a surface somewhere in space
  Vector3 sPosition(3421., 112., 893.);
  Vector3 sNormal = Vector3(1.2, -0.3, 0.05).normalized();

  // Create a surface & parameters with covariance on the surface
  auto pSurface = CurvilinearSurface(sPosition, sNormal).planeSurface();

  Covariance cov;
  cov << 10_mm, 0, 0, 0, 0, 0, 0, 10_mm, 0, 0, 0, 0, 0, 0, 0.1, 0, 0, 0, 0, 0,
      0, 0.1, 0, 0, 0, 0, 0, 0, 1. / (10_GeV), 0, 0, 0, 0, 0, 0, 0;

  BoundVector pars;
  pars << 12.34, -8722., 2.134, 0.85, 1. / (100_GeV), 0;

  BoundTrackParameters atPlane(pSurface, pars, std::move(cov),
                               ParticleHypothesis::pion());

  // run the test
  testJacobianToGlobal(atPlane);
}

/// This tests the jacobian of local perigee -> global
BOOST_AUTO_TEST_CASE(JacobianPerigeeToGlobalTest) {
  // Create a surface & parameters with covariance on the surface
  auto pSurface = Surface::makeShared<PerigeeSurface>(Vector3({0., 0., 0.}));

  Covariance cov;
  cov << 10_mm, 0, 0, 0, 0, 0, 0, 10_mm, 0, 0, 0, 0, 0, 0, 0.1, 0, 0, 0, 0, 0,
      0, 0.1, 0, 0, 0, 0, 0, 0, 1. / (10_GeV), 0, 0, 0, 0, 0, 0, 0;
  BoundVector pars;
  pars << -3.34, -822., -0.734, 0.85, 1. / (100_GeV), 0;

  BoundTrackParameters perigee(pSurface, pars, std::move(cov),
                               ParticleHypothesis::pion());

  // run the test
  testJacobianToGlobal(perigee);
}

/// This tests the jacobian of local straw -> global
BOOST_AUTO_TEST_CASE(JacobianStrawToGlobalTest) {
  // Create a surface & parameters with covariance on the surface
  auto sTransform = createCylindricTransform({1019., -52., 382.}, 0.4, -0.3);
  auto sSurface = Surface::makeShared<StrawSurface>(sTransform, 10., 1000.);

  Covariance cov;
  cov << 10_mm, 0, 0, 0, 0, 0, 0, 10_mm, 0, 0, 0, 0, 0, 0, 0.1, 0, 0, 0, 0, 0,
      0, 0.1, 0, 0, 0, 0, 0, 0, 1. / (10_GeV), 0, 0, 0, 0, 0, 0, 0;

  BoundVector pars;
  pars << -8.34, 812., 0.734, 0.25, 1. / (100_GeV), 0;

  BoundTrackParameters atStraw(sSurface, pars, std::move(cov),
                               ParticleHypothesis::pion());

  // run the test
  testJacobianToGlobal(atStraw);
}

}  // namespace Acts::Test
