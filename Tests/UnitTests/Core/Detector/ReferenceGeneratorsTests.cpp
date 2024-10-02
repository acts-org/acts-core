// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/detail/ReferenceGenerators.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/BinningType.hpp"

#include <memory>
#include <utility>
#include <vector>

using namespace Acts::Experimental::detail;

Acts::GeometryContext tContext;

auto rBounds = std::make_shared<Acts::RectangleBounds>(10, 20);
auto sTransform = Acts::Transform3::Identity();
auto pSurface = Acts::Surface::makeShared<Acts::PlaneSurface>(
    sTransform.pretranslate(Acts::Vector3(20., 20., 100.)), std::move(rBounds));

BOOST_AUTO_TEST_SUITE(Detector)

BOOST_AUTO_TEST_CASE(CenterReference) {
  // Simply return the cetner
  auto center = CenterReferenceGenerator{}.references(tContext, *pSurface);
  BOOST_CHECK_EQUAL(center.size(), 1u);
  BOOST_CHECK(center.front().isApprox(Acts::Vector3(20., 20., 100.)));
}

BOOST_AUTO_TEST_CASE(BinningPositionReference) {
  // Simply return binning position, we test only the behavior of the generator
  // not the output
  auto binningPosition =
      BinningValueReferenceGenerator<Acts::BinningValue::binZ>{}.references(
          tContext, *pSurface);
  BOOST_CHECK_EQUAL(binningPosition.size(), 1u);
}

BOOST_AUTO_TEST_CASE(PolyhedronReference) {
  // Simply return binning position, we test only the behavior of the generator
  // not the output
  auto referencePositions =
      PolyhedronReferenceGenerator<>{}.references(tContext, *pSurface);
  // 4 corners with center of gravity
  BOOST_CHECK_EQUAL(referencePositions.size(), 5u);
}

BOOST_AUTO_TEST_SUITE_END()
