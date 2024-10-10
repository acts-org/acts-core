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
#include "Acts/Geometry/Layer.hpp"
#include "Acts/Geometry/NavigationLayer.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/BinningType.hpp"

#include <cmath>
#include <memory>
#include <utility>

#include "../Surfaces/SurfaceStub.hpp"

namespace Acts::Test {
// Create a test context
GeometryContext tgContext = GeometryContext();
}  // namespace Acts::Test

namespace Acts::Test::Layers {

BOOST_AUTO_TEST_SUITE(Layers)

/// Unit test for creating compliant/non-compliant NavigationLayer object
BOOST_AUTO_TEST_CASE(NavigationLayerConstruction) {
  // default constructor, copy and assignment are all deleted
  std::shared_ptr<const Surface> pSurface = Surface::makeShared<SurfaceStub>();
  auto pNavigationLayer = NavigationLayer::create(std::move(pSurface));
  BOOST_CHECK_EQUAL(pNavigationLayer->layerType(), LayerType::navigation);
  // next level: with thickness
  const double thickness = 0.1;
  auto pSurface2 = Surface::makeShared<SurfaceStub>();
  auto pThickNavigationLayer =
      NavigationLayer::create(std::move(pSurface2), thickness);
  BOOST_CHECK_EQUAL(pThickNavigationLayer->layerType(), LayerType::navigation);
}

/// Unit test for testing NavigationLayer properties
BOOST_AUTO_TEST_CASE(NavigationLayerProperties) {
  const double thickness = 0.1;
  std::shared_ptr<const Surface> pSurface = Surface::makeShared<SurfaceStub>();
  auto rawSurfacePtr = pSurface.get();
  auto pNavigationLayer =
      NavigationLayer::create(std::move(pSurface), thickness);
  BinningValue b{BinningValue::binZ};
  Vector3 origin{0., 0., 0.};
  // binningPosition(), needs a better test
  BOOST_CHECK_EQUAL(pNavigationLayer->binningPosition(tgContext, b), origin);
  // surfaceRepresentation() [looks dangerous]
  BOOST_CHECK_EQUAL(rawSurfacePtr,
                    &(pNavigationLayer->surfaceRepresentation()));
  // isOnLayer()
  BOOST_CHECK(pNavigationLayer->isOnLayer(tgContext, origin,
                                          BoundaryTolerance::None()));
  // isOnLayer()
  Vector3 crazyPosition{1000., 10000., std::nan("")};
  // layer stub has hard-coded globalToLocal return value
  BOOST_CHECK(pNavigationLayer->isOnLayer(tgContext, crazyPosition,
                                          BoundaryTolerance::None()));
  // resolve()
  BOOST_CHECK(!pNavigationLayer->resolve(true, true, true));
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test::Layers
