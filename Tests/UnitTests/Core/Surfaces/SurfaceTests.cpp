// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/PlaneLayer.hpp"
#include "Acts/Material/HomogeneousSurfaceMaterial.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"  //to get s_noBounds
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Tests/CommonHelpers/DetectorElementStub.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Tests/CommonHelpers/PredefinedMaterials.hpp"

#include <memory>

#include "SurfaceStub.hpp"

namespace Acts {
/// Mock track object with minimal methods implemented for compilation
class MockTrack {
 public:
  MockTrack(const Vector3& mom, const Vector3& pos) : m_mom(mom), m_pos(pos) {
    // nop
  }

  Vector3 momentum() const { return m_mom; }

  Vector3 position() const { return m_pos; }

 private:
  Vector3 m_mom;
  Vector3 m_pos;
};
}  // namespace Acts

namespace Acts::Test {

// Create a test context
GeometryContext tgContext = GeometryContext();

BOOST_AUTO_TEST_SUITE(Surfaces)

/// todo: make test fixture; separate out different cases

/// Unit test for creating compliant/non-compliant Surface object
BOOST_AUTO_TEST_CASE(SurfaceConstruction) {
  // SurfaceStub s;
  BOOST_CHECK_EQUAL(Surface::Other, SurfaceStub().type());
  SurfaceStub original;
  BOOST_CHECK_EQUAL(Surface::Other, SurfaceStub(original).type());
  Translation3 translation{0., 1., 2.};
  Transform3 transform(translation);
  BOOST_CHECK_EQUAL(Surface::Other,
                    SurfaceStub(tgContext, original, transform).type());
  // need some cruft to make the next one work
  auto pTransform = Transform3(translation);
  std::shared_ptr<const Acts::PlanarBounds> p =
      std::make_shared<const RectangleBounds>(5., 10.);
  DetectorElementStub detElement{pTransform, p, 0.2, nullptr};
  BOOST_CHECK_EQUAL(Surface::Other, SurfaceStub(detElement).type());
}

/// Unit test for testing Surface properties
BOOST_AUTO_TEST_CASE(SurfaceProperties) {
  // build a test object , 'surface'
  std::shared_ptr<const Acts::PlanarBounds> pPlanarBound =
      std::make_shared<const RectangleBounds>(5., 10.);
  Vector3 reference{0., 1., 2.};
  Translation3 translation{0., 1., 2.};
  auto pTransform = Transform3(translation);
  auto pLayer = PlaneLayer::create(pTransform, pPlanarBound);
  auto pMaterial =
      std::make_shared<const HomogeneousSurfaceMaterial>(makePercentSlab());
  DetectorElementStub detElement{pTransform, pPlanarBound, 0.2, pMaterial};
  SurfaceStub surface(detElement);
  // associatedDetectorElement
  BOOST_CHECK_EQUAL(surface.associatedDetectorElement(), &detElement);
  // test  associatelayer, associatedLayer
  surface.associateLayer(*pLayer);
  BOOST_CHECK_EQUAL(surface.associatedLayer(), pLayer.get());
  // associated Material is not set to the surface
  // it is set to the detector element surface though
  BOOST_CHECK_NE(surface.surfaceMaterial(), pMaterial.get());
  // center()
  CHECK_CLOSE_OR_SMALL(reference, surface.center(tgContext), 1e-6, 1e-9);
  // insideBounds
  Vector2 localPosition{0.1, 3.0};
  BOOST_CHECK(surface.insideBounds(localPosition));
  Vector2 outside{20., 20.};
  BOOST_CHECK(surface.insideBounds(
      outside));  // should return false, but doesn't because SurfaceStub has
                  // "no bounds" hard-coded
  Vector3 mom{100., 200., 300.};
  // isOnSurface
  BOOST_CHECK(surface.isOnSurface(tgContext, reference, mom,
                                  BoundaryTolerance::Infinite()));
  BOOST_CHECK(surface.isOnSurface(
      tgContext, reference, mom,
      BoundaryTolerance::None()));  // need to improve bounds()
  // referenceFrame()
  RotationMatrix3 unitary;
  unitary << 1, 0, 0, 0, 1, 0, 0, 0, 1;
  auto referenceFrame =
      surface.referenceFrame(tgContext, Vector3{1, 2, 3}.normalized(),
                             mom);  // need more complex case to test
  BOOST_CHECK_EQUAL(referenceFrame, unitary);
  // normal()
  auto normal = surface.normal(tgContext, Vector3{1, 2, 3}.normalized(),
                               Vector3::UnitZ());  // needs more
                                                   // complex test
  Vector3 zero{0., 0., 0.};
  BOOST_CHECK_EQUAL(zero, normal);
  // pathCorrection is pure virtual
  // surfaceMaterial()
  auto pNewMaterial =
      std::make_shared<const HomogeneousSurfaceMaterial>(makePercentSlab());
  surface.assignSurfaceMaterial(pNewMaterial);
  BOOST_CHECK_EQUAL(surface.surfaceMaterial(),
                    pNewMaterial.get());  // passes ??
  //
  CHECK_CLOSE_OR_SMALL(surface.transform(tgContext), pTransform, 1e-6, 1e-9);
  // type() is pure virtual
}

BOOST_AUTO_TEST_CASE(EqualityOperators) {
  // build some test objects
  std::shared_ptr<const Acts::PlanarBounds> pPlanarBound =
      std::make_shared<const RectangleBounds>(5., 10.);
  Vector3 reference{0., 1., 2.};
  Translation3 translation1{0., 1., 2.};
  Translation3 translation2{1., 1., 2.};
  auto pTransform1 = Transform3(translation1);
  auto pTransform2 = Transform3(translation2);
  // build a planeSurface to be compared
  auto planeSurface =
      Surface::makeShared<PlaneSurface>(pTransform1, pPlanarBound);
  auto pLayer = PlaneLayer::create(pTransform1, pPlanarBound);
  auto pMaterial =
      std::make_shared<const HomogeneousSurfaceMaterial>(makePercentSlab());
  DetectorElementStub detElement1{pTransform1, pPlanarBound, 0.2, pMaterial};
  DetectorElementStub detElement2{pTransform1, pPlanarBound, 0.3, pMaterial};
  DetectorElementStub detElement3{pTransform2, pPlanarBound, 0.3, pMaterial};
  //
  SurfaceStub surface1(detElement1);
  SurfaceStub surface2(detElement1);  // 1 and 2 are the same
  SurfaceStub surface3(detElement2);  // 3 differs in thickness
  SurfaceStub surface4(detElement3);  // 4 has a different transform and id
  SurfaceStub surface5(detElement1);
  surface5.assignSurfaceMaterial(pMaterial);  // 5 has non-null surface material
  //
  BOOST_CHECK(surface1 == surface2);
  //
  // remove test for the moment,
  // surfaces do not have a concept of thickness (only detector elements have)
  // only thickness is different here
  //
  // BOOST_CHECK_NE(surface1, surface3);  // will fail
  //
  BOOST_CHECK(surface1 != surface4);
  //
  BOOST_CHECK(surface1 != surface5);
  //
  BOOST_CHECK(surface1 != *planeSurface);
  // Test the getSharedPtr
  const auto surfacePtr = Surface::makeShared<const SurfaceStub>(detElement1);
  const auto sharedSurfacePtr = surfacePtr->getSharedPtr();
  BOOST_CHECK(*surfacePtr == *sharedSurfacePtr);
}
BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
