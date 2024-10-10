// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/ApproachDescriptor.hpp"
#include "Acts/Geometry/GenericApproachDescriptor.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/Layer.hpp"
#include "Acts/Geometry/PlaneLayer.hpp"
#include "Acts/Geometry/SurfaceArrayCreator.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceArray.hpp"
#include "Acts/Utilities/BinningType.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace Acts::Test {
// Create a test context
GeometryContext tgContext = GeometryContext();
}  // namespace Acts::Test

namespace Acts::Test::Layers {

BOOST_AUTO_TEST_SUITE(Layers)

/// Unit test for creating compliant/non-compliant PlaneLayer object
BOOST_AUTO_TEST_CASE(PlaneLayerConstruction) {
  // default constructor, copy and assignment are all deleted
  // minimally need a Transform3 and a PlanarBounds object (e.g.
  // RectangleBounds) to construct
  Translation3 translation{0., 1., 2.};
  auto pTransform = Transform3(translation);
  const double halfX(10.), halfY(5.);  // 20 x 10 rectangle
  auto pRectangle = std::make_shared<const RectangleBounds>(halfX, halfY);
  auto pPlaneLayer = PlaneLayer::create(pTransform, pRectangle);
  BOOST_CHECK_EQUAL(pPlaneLayer->layerType(), LayerType::active);
  // next level: need an array of Surfaces;
  // bounds object, rectangle type
  auto rBounds = std::make_shared<const RectangleBounds>(1., 1.);
  /// Constructor with transform pointer
  const std::vector<std::shared_ptr<const Surface>> aSurfaces{
      Surface::makeShared<PlaneSurface>(Transform3::Identity(), rBounds),
      Surface::makeShared<PlaneSurface>(Transform3::Identity(), rBounds)};
  const double thickness(1.0);
  SurfaceArrayCreator sac;
  std::size_t binsX(2), binsY(4);
  auto pSurfaceArray = sac.surfaceArrayOnPlane(tgContext, aSurfaces, binsX,
                                               binsY, BinningValue::binZ);
  auto pPlaneLayerFromSurfaces =
      PlaneLayer::create(pTransform, pRectangle, std::move(pSurfaceArray));
  BOOST_CHECK_EQUAL(pPlaneLayerFromSurfaces->layerType(), LayerType::active);
  // construct with thickness:
  auto pPlaneLayerWithThickness = PlaneLayer::create(
      pTransform, pRectangle, std::move(pSurfaceArray), thickness);
  BOOST_CHECK_EQUAL(pPlaneLayerWithThickness->thickness(), thickness);
  // with an approach descriptor...
  std::unique_ptr<ApproachDescriptor> ad(
      new GenericApproachDescriptor(aSurfaces));
  auto adPtr = ad.get();
  auto pPlaneLayerWithApproachDescriptor =
      PlaneLayer::create(pTransform, pRectangle, std::move(pSurfaceArray),
                         thickness, std::move(ad));
  BOOST_CHECK_EQUAL(pPlaneLayerWithApproachDescriptor->approachDescriptor(),
                    adPtr);
  // with the layerType specified...
  auto pPlaneLayerWithLayerType =
      PlaneLayer::create(pTransform, pRectangle, std::move(pSurfaceArray),
                         thickness, std::move(ad), LayerType::passive);
  BOOST_CHECK_EQUAL(pPlaneLayerWithLayerType->layerType(), LayerType::passive);
}

/// Unit test for testing Layer properties
BOOST_AUTO_TEST_CASE(PlaneLayerProperties) {
  Translation3 translation{0., 1., 2.};
  auto pTransform = Transform3(translation);
  const double halfX(10.), halfY(5.);  // 20 x 10 rectangle
  auto pRectangle = std::make_shared<const RectangleBounds>(halfX, halfY);
  auto pPlaneLayer = PlaneLayer::create(pTransform, pRectangle);
  // auto planeSurface = pPlaneLayer->surfaceRepresentation();
  BOOST_CHECK_EQUAL(pPlaneLayer->surfaceRepresentation().name(),
                    std::string("Acts::PlaneSurface"));
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test::Layers
