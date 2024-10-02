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
#include "Acts/Geometry/DiscLayer.hpp"
#include "Acts/Geometry/GenericApproachDescriptor.hpp"
#include "Acts/Geometry/Layer.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceArray.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace Acts::Test::Layers {

BOOST_AUTO_TEST_SUITE(Layers)

/// Unit test for creating compliant/non-compliant DiscLayer object
BOOST_AUTO_TEST_CASE(DiscLayerConstruction) {
  // default constructor, copy and assignment are all deleted
  // minimally need a Transform3 and a PlanarBounds object (e.g.
  // RadialBounds) to construct
  Translation3 translation{0., 1., 2.};
  auto pTransform = Transform3(translation);
  const double minRad(5.), maxRad(10.);  // 20 x 10 disc
  auto pDisc = std::make_shared<const RadialBounds>(minRad, maxRad);
  auto pDiscLayer = DiscLayer::create(pTransform, pDisc, nullptr, 1.);
  BOOST_CHECK_EQUAL(pDiscLayer->layerType(), LayerType::passive);
  // next level: need an array of Surfaces;
  // bounds object, rectangle type
  auto rBounds = std::make_shared<const RectangleBounds>(1., 1.);
  /// Construction
  const std::vector<std::shared_ptr<const Surface>> aSurfaces{
      Surface::makeShared<PlaneSurface>(Transform3::Identity(), rBounds),
      Surface::makeShared<PlaneSurface>(Transform3::Identity(), rBounds)};
  const double thickness(1.0);
  auto pDiscLayerFromSurfaces =
      DiscLayer::create(pTransform, pDisc, nullptr, 1.);
  BOOST_CHECK_EQUAL(pDiscLayerFromSurfaces->layerType(), LayerType::passive);
  // construct with thickness:
  auto pDiscLayerWithThickness =
      DiscLayer::create(pTransform, pDisc, nullptr, thickness);
  BOOST_CHECK_EQUAL(pDiscLayerWithThickness->thickness(), thickness);
  // with an approach descriptor...
  std::unique_ptr<ApproachDescriptor> ad(
      new GenericApproachDescriptor(aSurfaces));
  auto adPtr = ad.get();
  auto pDiscLayerWithApproachDescriptor =
      DiscLayer::create(pTransform, pDisc, nullptr, thickness, std::move(ad));
  BOOST_CHECK_EQUAL(pDiscLayerWithApproachDescriptor->approachDescriptor(),
                    adPtr);
  // with the layerType specified...
  auto pDiscLayerWithLayerType = DiscLayer::create(
      pTransform, pDisc, nullptr, thickness, std::move(ad), LayerType::passive);
  BOOST_CHECK_EQUAL(pDiscLayerWithLayerType->layerType(), LayerType::passive);
}

/// Unit test for testing Layer properties
BOOST_AUTO_TEST_CASE(DiscLayerProperties) {
  Translation3 translation{0., 1., 2.};
  auto pTransform = Transform3(translation);
  const double minRad(5.), maxRad(10.);  // 20 x 10 disc
  auto pDisc = std::make_shared<const RadialBounds>(minRad, maxRad);
  auto pDiscLayer = DiscLayer::create(pTransform, pDisc, nullptr, 1.);
  // auto planeSurface = pDiscLayer->surfaceRepresentation();
  BOOST_CHECK_EQUAL(pDiscLayer->surfaceRepresentation().name(),
                    std::string("Acts::DiscSurface"));
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test::Layers
