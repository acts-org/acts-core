// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>

#include "Acts/Navigation/NavigationStream.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"

namespace {

// This creates a set of plane surfaces along the z axis
std::vector<std::shared_ptr<Acts::Surface>> createPlaneSurfaces() {
  auto rectangle = std::make_shared<Acts::RectangleBounds>(10., 10.);
  // Surface A:
  // This surface should not be reachable from (0.,0.,0.) position along z
  Acts::Transform3 aTransform = Acts::Transform3::Identity();
  aTransform.pretranslate(Acts::Vector3(0., 0., -20.));
  auto surfaceA =
      Acts::Surface::makeShared<Acts::PlaneSurface>(aTransform, rectangle);
  // Surface B:
  // This surface should not be reachable from (0.,0.,0.) position along z with
  // boundary check
  Acts::Transform3 bTransform = Acts::Transform3::Identity();
  bTransform.pretranslate(Acts::Vector3(50., 50., 100.));
  auto surfaceB =
      Acts::Surface::makeShared<Acts::PlaneSurface>(bTransform, rectangle);
  // Surface C:
  Acts::Transform3 cTransform = Acts::Transform3::Identity();
  cTransform.pretranslate(Acts::Vector3(0., 0., 200.));
  auto surfaceC =
      Acts::Surface::makeShared<Acts::PlaneSurface>(cTransform, rectangle);
  // Surface D:
  Acts::Transform3 dTransform = Acts::Transform3::Identity();
  dTransform.pretranslate(Acts::Vector3(0., 0., 400.));
  auto surfaceD =
      Acts::Surface::makeShared<Acts::PlaneSurface>(dTransform, rectangle);

  // Let's fill them shuffled
  return {surfaceC, surfaceA, surfaceD, surfaceB};
}

// This creates a set of cylinder surfaces
std::vector<std::shared_ptr<Acts::Surface>> createCylinders() {
  // Surface A:
  // A concentric cylinder with a radius of 10 and a half length of 20
  Acts::Transform3 aTransform = Acts::Transform3::Identity();
  auto surfaceA =
      Acts::Surface::makeShared<Acts::CylinderSurface>(aTransform, 10., 20);

  // Surface B:
  // A  small cylinder sitting at 20, 20
  Acts::Transform3 bTransform = Acts::Transform3::Identity();
  bTransform.pretranslate(Acts::Vector3(20., 20., 0.));
  auto surfaceB =
      Acts::Surface::makeShared<Acts::CylinderSurface>(bTransform, 2., 10);

  // Surface C:
  // A concentric cylinder with a radius of 40 and a half length of 20
  Acts::Transform3 cTransform = Acts::Transform3::Identity();
  auto surfaceC =
      Acts::Surface::makeShared<Acts::CylinderSurface>(cTransform, 40., 20);

  // Surface C:
  // A concentric, but shifted cylinder with a radius of 50 and a half length of
  // 5
  Acts::Transform3 dTransform = Acts::Transform3::Identity();
  dTransform.pretranslate(Acts::Vector3(0., 0., 10.));
  auto surfaceD =
      Acts::Surface::makeShared<Acts::CylinderSurface>(dTransform, 50., 5.);

  // Return in a shuffled order
  return {surfaceC, surfaceB, surfaceA, surfaceD};
}

}  // namespace

using namespace Acts;

auto gContext = GeometryContext();

BOOST_AUTO_TEST_SUITE(Navigation)

BOOST_AUTO_TEST_CASE(NavigationStream_InitializePlanes) {
  // Create the punch of surfaces
  auto surfaces = createPlaneSurfaces();

  NavigationStream nStreamTemplate;
  for (const auto& surface : surfaces) {
    nStreamTemplate.addSurfaceCandidate(*surface,
                                        Acts::BoundaryTolerance::None());
  }
  BOOST_CHECK_EQUAL(nStreamTemplate.remainingCandidates(), 4u);

  // (1) Run an initial update
  // - from a position where all are reachable and valid
  // - with infinite boundary tolerance
  NavigationStream nStream = nStreamTemplate;
  BOOST_CHECK(nStream.initialize(gContext,
                                 {Vector3(0., 0., -30.), Vector3(0., 0., 1.)},
                                 BoundaryTolerance::Infinite()));

  BOOST_CHECK_EQUAL(nStream.remainingCandidates(), 4u);
  BOOST_CHECK_EQUAL(&nStream.currentCandidate().surface(), surfaces[1u].get());

  // (2) Run an initial update
  // - from a position where all but one are reachable
  // - with infinite boundary tolerance
  nStream = nStreamTemplate;
  BOOST_CHECK(nStream.initialize(gContext,
                                 {Vector3(0., 0., 0.), Vector3(0., 0., 1.)},
                                 BoundaryTolerance::Infinite()));
  BOOST_CHECK_EQUAL(nStream.remainingCandidates(), 3u);
  BOOST_CHECK_EQUAL(&nStream.currentCandidate().surface(), surfaces[3u].get());

  // (3) Run an initial update
  // - from a position where all would be reachable, but
  // - with no boundary tolerance
  nStream = nStreamTemplate;
  BOOST_CHECK(nStream.initialize(gContext,
                                 {Vector3(0., 0., -100.), Vector3(0., 0., 1.)},
                                 BoundaryTolerance::None()));
  BOOST_CHECK_EQUAL(nStream.remainingCandidates(), 3u);

  // (4) Run an initial update
  // - none of the surfaces should be reachable
  nStream = nStreamTemplate;
  BOOST_CHECK(!nStream.initialize(gContext,
                                  {Vector3(0., 0., 0.), Vector3(1., 0., 0.)},
                                  BoundaryTolerance::Infinite()));
  BOOST_CHECK_EQUAL(nStream.remainingCandidates(), 0u);
  BOOST_CHECK_THROW(nStream.currentCandidate(), std::out_of_range);

  // (5) Test de-duplication
  nStream = nStreamTemplate;
  nStreamTemplate.addSurfaceCandidate(*surfaces.at(0),
                                      Acts::BoundaryTolerance::None());
  // One surface is duplicated in the stream
  BOOST_CHECK_EQUAL(nStreamTemplate.remainingCandidates(), 5u);
  // Initialize stream reaches all surfaces, but also de-duplicates
  BOOST_CHECK(nStream.initialize(gContext,
                                 {Vector3(0., 0., -100.), Vector3(0., 0., 1.)},
                                 BoundaryTolerance::Infinite()));
  BOOST_CHECK_EQUAL(nStream.remainingCandidates(), 4u);
}

BOOST_AUTO_TEST_CASE(NavigationStream_UpdatePlanes) {
  // Create the punch of surfaces
  auto surfaces = createPlaneSurfaces();

  // Surfaces are filled with no boundary tolerance, we require them to be
  // reachable and intersections inside bounds
  NavigationStream nStreamTemplate;
  for (const auto& surface : surfaces) {
    nStreamTemplate.addSurfaceCandidate(*surface,
                                        Acts::BoundaryTolerance::None());
  }
  BOOST_CHECK_EQUAL(nStreamTemplate.remainingCandidates(), 4u);

  // Run an initial update
  // - from a position where all are reachable and valid
  // - with infinite boundary tolerance
  NavigationStream::QueryPoint qPoint = {Vector3(0., 0., -30.),
                                         Vector3(0., 0., 1.)};

  NavigationStream nStream = nStreamTemplate;
  BOOST_CHECK(
      nStream.initialize(gContext, qPoint, BoundaryTolerance::Infinite()));
  BOOST_CHECK_EQUAL(nStream.remainingCandidates(), 4u);
  BOOST_CHECK_EQUAL(&nStream.currentCandidate().surface(), surfaces[1u].get());
  CHECK_CLOSE_ABS(nStream.currentCandidate().pathLength(), 10.,
                  std::numeric_limits<double>::epsilon());

  // Let's push a bit closer to the surface
  qPoint.position = Vector3(0., 0., -22.);
  BOOST_CHECK(nStream.update(gContext, qPoint));
  // Surface unchanged, but the intersection should be closer
  BOOST_CHECK_EQUAL(&nStream.currentCandidate().surface(), surfaces[1u].get());
  CHECK_CLOSE_ABS(nStream.currentCandidate().pathLength(), 2.,
                  std::numeric_limits<double>::epsilon());

  // Uuuups, an overstep
  qPoint.position = Vector3(0., 0., -19.5);
  BOOST_CHECK(nStream.update(gContext, qPoint));
  // Surface still unchanged, but pathLength is now negative
  BOOST_CHECK_EQUAL(&nStream.currentCandidate().surface(), surfaces[1u].get());
  CHECK_CLOSE_ABS(nStream.currentCandidate().pathLength(), -0.5,
                  std::numeric_limits<double>::epsilon());

  // Finally hit it
  qPoint.position = Vector3(0., 0., -20.);
  BOOST_CHECK(nStream.update(gContext, qPoint));
  // Surface still unchanged, however, now withL
  // - pathlength smaller on surface tolerance, intersection status onSurface
  BOOST_CHECK_EQUAL(&nStream.currentCandidate().surface(), surfaces[1u].get());
  CHECK_CLOSE_ABS(
      nStream.currentCandidate().pathLength(), s_onSurfaceTolerance,
      std::numeric_limits<double>::epsilon() + s_onSurfaceTolerance);
  BOOST_CHECK_EQUAL(nStream.currentCandidate().intersection.status(),
                    IntersectionStatus::onSurface);
  // Let's say the stepper confirms this
  BOOST_CHECK(nStream.switchToNextCandidate());
  // Surface is now surfaceB
  BOOST_CHECK_EQUAL(&nStream.currentCandidate().surface(), surfaces[3u].get());
  // Distance should be the initial estimate from the intialializeStream() call
  CHECK_CLOSE_ABS(nStream.currentCandidate().pathLength(), 130.,
                  std::numeric_limits<double>::epsilon());
  // Query update will re-evaluate this one: however, we will miss the surface
  // due to outside bounds - and will switch to the next candidate: which sits
  // at 200 and then will yield 220
  BOOST_CHECK(nStream.update(gContext, qPoint));
  CHECK_CLOSE_ABS(nStream.currentCandidate().pathLength(), 220.,
                  std::numeric_limits<double>::epsilon());
  // Oh noooo, an actor just kicked in and changed the direction
  qPoint.direction = Vector3(0., 1., 1.).normalized();
  // All is lost, no surface is reachable anymore
  BOOST_CHECK(!nStream.update(gContext, qPoint));
}

BOOST_AUTO_TEST_CASE(NavigationStream_InitializeCylinders) {
  // Create the cylinder setup
  auto surfaces = createCylinders();

  // Let us fill the surfaces into the navigation stream
  NavigationStream nStreamTemplate;
  for (const auto& surface : surfaces) {
    const Surface* pointer = surface.get();
    nStreamTemplate.addSurfaceCandidates({&pointer, 1},
                                         Acts::BoundaryTolerance::None());
  }
  BOOST_CHECK_EQUAL(nStreamTemplate.remainingCandidates(), 4u);

  // (1) Run an initial update - from a position/direction where all are
  // reachable
  // - with infinite boundary tolerance
  NavigationStream nStream = nStreamTemplate;
  BOOST_CHECK(nStream.initialize(
      gContext, {Vector3(0., 0., 0.), Vector3(1., 1., 0.).normalized()},
      BoundaryTolerance::Infinite()));
  // We should have 5 candidates, as one cylinder is reachable twice
  BOOST_CHECK_EQUAL(nStream.remainingCandidates(), 5u);
  // First one is inner candidate
  BOOST_CHECK_EQUAL(&nStream.currentCandidate().surface(), surfaces[2].get());
  // Surface of 2nd and 3rd candidate should be the same
  BOOST_CHECK_EQUAL(&nStream.candidates()[1u].surface(), surfaces[1].get());
  BOOST_CHECK_EQUAL(&nStream.candidates()[2u].surface(), surfaces[1].get());

  // (2) Run an initial update - from a position/direction where only
  // the concentric ones are reachable
  // - with infinite boundary tolerance
  nStream = nStreamTemplate;
  BOOST_CHECK(nStream.initialize(gContext,
                                 {Vector3(0., 0., 0.), Vector3(1., 0., 0.)},
                                 BoundaryTolerance::Infinite()));
  // We should have 3 candidates
  BOOST_CHECK_EQUAL(nStream.remainingCandidates(), 3u);

  // (3) Run an initial update - from a position/direction where only the
  // concentric ones within bounds are reachable
  nStream = nStreamTemplate;
  BOOST_CHECK(nStream.initialize(gContext,
                                 {Vector3(0., 0., 0.), Vector3(1., 0., 0.)},
                                 BoundaryTolerance::None()));
  // We should have 2 candidates
  BOOST_CHECK_EQUAL(nStream.remainingCandidates(), 2u);

  // (4) Run an initial update - from a position/direction where none are
  // reachable
  // - (even) with infinite boundary tolerance
  nStream = nStreamTemplate;
  BOOST_CHECK(!nStream.initialize(gContext,
                                  {Vector3(0., 0., 0.), Vector3(0., 0., 1.)},
                                  BoundaryTolerance::None()));
  // We should have 0 candidates
  BOOST_CHECK_EQUAL(nStream.remainingCandidates(), 0u);
  BOOST_CHECK_THROW(nStream.currentCandidate(), std::out_of_range);
}

BOOST_AUTO_TEST_SUITE_END()
