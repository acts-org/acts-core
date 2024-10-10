// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Surfaces/CurvilinearSurface.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Intersection.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace Acts::Test {

class Object {};

/// test of the intersection class
BOOST_AUTO_TEST_CASE(IntersectionTest) {
  // a few valid intersections
  // all positively sortered
  Intersection3D fIp(Vector3(0., 1., 0.), 1.,
                     Intersection3D::Status::reachable);
  Intersection3D sIp(Vector3(0., 2., 0.), 2.,
                     Intersection3D::Status::reachable);
  Intersection3D tIp(Vector3(0., 3., 0.), 3.,
                     Intersection3D::Status::reachable);
  BOOST_CHECK(fIp.isValid());
  BOOST_CHECK(sIp.isValid());
  BOOST_CHECK(tIp.isValid());

  // a non-valid intersection
  Intersection3D nIp(Vector3(3., 3., 0.), 3.,
                     Intersection3D::Status::unreachable);
  BOOST_CHECK(!nIp.isValid());

  std::vector<Intersection3D> fstpIntersections = {fIp, sIp, tIp};
  std::vector<Intersection3D> tsfpIntersections = {tIp, sIp, fIp};

  // let's sort the tsf intersection, it should give fst
  std::ranges::sort(tsfpIntersections, Intersection3D::pathLengthOrder);
  BOOST_CHECK_EQUAL(fstpIntersections[0].pathLength(),
                    tsfpIntersections[0].pathLength());
  BOOST_CHECK_EQUAL(fstpIntersections[1].pathLength(),
                    tsfpIntersections[1].pathLength());
  BOOST_CHECK_EQUAL(fstpIntersections[2].pathLength(),
                    tsfpIntersections[2].pathLength());

  // now let's create one with a non-valid intersection, it should be shuffled
  // last
  std::vector<Intersection3D> ntfspIntersections = {nIp, tIp, fIp, sIp};
  std::vector<Intersection3D> tfnsnpIntersections = {tIp, fIp, nIp, sIp, nIp};

  // shuffle the intersections
  std::ranges::sort(ntfspIntersections, Intersection3D::pathLengthOrder);
  BOOST_CHECK_EQUAL(fstpIntersections[0].pathLength(),
                    ntfspIntersections[0].pathLength());
  BOOST_CHECK_EQUAL(fstpIntersections[1].pathLength(),
                    ntfspIntersections[1].pathLength());
  BOOST_CHECK_EQUAL(fstpIntersections[2].pathLength(),
                    ntfspIntersections[2].pathLength());

  std::ranges::sort(tfnsnpIntersections, Intersection3D::pathLengthOrder);
  BOOST_CHECK_EQUAL(fstpIntersections[0].pathLength(),
                    tfnsnpIntersections[0].pathLength());
  BOOST_CHECK_EQUAL(fstpIntersections[1].pathLength(),
                    tfnsnpIntersections[1].pathLength());
  BOOST_CHECK_EQUAL(fstpIntersections[2].pathLength(),
                    tfnsnpIntersections[2].pathLength());

  /// let's make a bunch of negative solution
  Intersection3D fIn(Vector3(0., -1., 0.), -1.,
                     Intersection3D::Status::reachable);
  Intersection3D sIn(Vector3(0., -2., 0.), -2.,
                     Intersection3D::Status::reachable);
  Intersection3D tIn(Vector3(0., -3., 0.), -3.,
                     Intersection3D::Status::reachable);

  std::vector<Intersection3D> tsfnIntersections = {tIn, sIn, fIn};
  std::vector<Intersection3D> fstnIntersections = {fIn, sIn, tIn};

  // this time around, sort the f-s-t-n to match the t-s-f-n
  std::ranges::sort(fstnIntersections, Intersection3D::pathLengthOrder);
  BOOST_CHECK_EQUAL(fstnIntersections[0].pathLength(),
                    tsfnIntersections[0].pathLength());
  BOOST_CHECK_EQUAL(fstnIntersections[1].pathLength(),
                    tsfnIntersections[1].pathLength());
  BOOST_CHECK_EQUAL(fstnIntersections[2].pathLength(),
                    tsfnIntersections[2].pathLength());

  // shuffle negative and positive solutions
  std::vector<Intersection3D> pnsolutions = {tIp, sIn, sIp, fIn, tIn, fIp};
  std::ranges::sort(pnsolutions, Intersection3D::pathLengthOrder);

  BOOST_CHECK_EQUAL(pnsolutions[0].pathLength(), -3.);
  BOOST_CHECK_EQUAL(pnsolutions[1].pathLength(), -2.);
  BOOST_CHECK_EQUAL(pnsolutions[2].pathLength(), -1.);
  BOOST_CHECK_EQUAL(pnsolutions[3].pathLength(), 1.);
  BOOST_CHECK_EQUAL(pnsolutions[4].pathLength(), 2.);
  BOOST_CHECK_EQUAL(pnsolutions[5].pathLength(), 3.);

  // sort intersections with zero path length
  Intersection3D zI(Vector3(0., 0., 0.), 0., Intersection3D::Status::onSurface);
  std::vector<Intersection3D> tszfpIntersections = {tIp, sIp, zI, fIp};

  std::ranges::sort(tszfpIntersections, Intersection3D::pathLengthOrder);
  BOOST_CHECK_EQUAL(tszfpIntersections[0].pathLength(), 0.);
  BOOST_CHECK_EQUAL(tszfpIntersections[1].pathLength(), 1.);
  BOOST_CHECK_EQUAL(tszfpIntersections[2].pathLength(), 2.);
  BOOST_CHECK_EQUAL(tszfpIntersections[3].pathLength(), 3.);

  std::vector<Intersection3D> tfsznIntersections = {tIn, fIn, sIn, zI};
  std::vector<Intersection3D> ztfsnIntersections = {zI, tIn, fIn, sIn};

  std::ranges::sort(tfsznIntersections, Intersection3D::pathLengthOrder);
  BOOST_CHECK_EQUAL(tfsznIntersections[0].pathLength(), -3.);
  BOOST_CHECK_EQUAL(tfsznIntersections[1].pathLength(), -2.);
  BOOST_CHECK_EQUAL(tfsznIntersections[2].pathLength(), -1.);
  BOOST_CHECK_EQUAL(tfsznIntersections[3].pathLength(), 0.);
}

/// test of the object intersection class
BOOST_AUTO_TEST_CASE(ObjectIntersectionTest) {
  auto psf6 = CurvilinearSurface(Vector3(6., 0., 0.), Vector3(1., 0., 0.))
                  .planeSurface();
  auto psf7 = CurvilinearSurface(Vector3(7., 0., 0.), Vector3(1., 0., 0.))
                  .planeSurface();
  auto psf8 = CurvilinearSurface(Vector3(8., 0., 0.), Vector3(1., 0., 0.))
                  .planeSurface();
  auto psf9 = CurvilinearSurface(Vector3(9., 0., 0.), Vector3(1., 0., 0.))
                  .planeSurface();
  auto psf10 = CurvilinearSurface(Vector3(10., 0., 0.), Vector3(1., 0., 0.))
                   .planeSurface();

  using PlaneIntersection = ObjectIntersection<PlaneSurface>;

  PlaneIntersection int6(Intersection3D(Vector3(6., 0., 0.), 6.,
                                        Intersection3D::Status::reachable),
                         psf6.get());
  PlaneIntersection int7(Intersection3D(Vector3(7., 0., 0.), 7.,
                                        Intersection3D::Status::reachable),
                         psf7.get());
  PlaneIntersection int8(Intersection3D(Vector3(8., 0., 0.), 8.,
                                        Intersection3D::Status::reachable),
                         psf8.get());
  PlaneIntersection int9a(Intersection3D(Vector3(9., 0., 0.), 9.,
                                         Intersection3D::Status::reachable),
                          psf9.get());
  PlaneIntersection int9b(
      Intersection3D(Vector3(9., 1., 0.), std::hypot(9., 1.),
                     Intersection3D::Status::reachable),
      psf9.get());
  PlaneIntersection int10(Intersection3D(Vector3(10., 0., 0.), 10.,
                                         Intersection3D::Status::reachable),
                          psf10.get());

  std::vector<PlaneIntersection> firstSet = {int6, int7, int9b, int10};
  std::vector<PlaneIntersection> secondSet = {int8, int9a, int9b, int10};
  // result of the standard union set
  std::vector<PlaneIntersection> unionSetStd = {};
  // result of the custominzed union set
  std::vector<PlaneIntersection> unionSetCst = {};

  // This should give 6 different intersections
  std::set_union(firstSet.begin(), firstSet.end(), secondSet.begin(),
                 secondSet.end(), std::back_inserter(unionSetStd),
                 PlaneIntersection::pathLengthOrder);
  BOOST_CHECK_EQUAL(unionSetStd.size(), 6u);
}

BOOST_AUTO_TEST_CASE(IntersectionStatusPrinting) {
  std::array<IntersectionStatus, 4> status_values = {
      {IntersectionStatus::missed, IntersectionStatus::unreachable,
       IntersectionStatus::reachable, IntersectionStatus::onSurface}};
  std::array<std::string, 4> expected_messages = {
      {"missed/unreachable", "missed/unreachable", "reachable", "onSurface"}};

  for (int i = 0; i < 4; ++i) {
    std::stringstream ss;
    ss << status_values[i];
    BOOST_CHECK_EQUAL(ss.str(), expected_messages[i]);
  }
}

}  // namespace Acts::Test
