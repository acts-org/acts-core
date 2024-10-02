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
#include "Acts/Geometry/ConeLayer.hpp"
#include "Acts/Geometry/GenericApproachDescriptor.hpp"
#include "Acts/Geometry/Layer.hpp"
#include "Acts/Surfaces/ConeBounds.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceArray.hpp"

#include <cmath>
#include <memory>
#include <utility>
#include <vector>

namespace Acts::Test::Layers {

BOOST_AUTO_TEST_SUITE(Layers)

/// Unit test for creating compliant/non-compliant ConeLayer object
BOOST_AUTO_TEST_CASE(ConeLayerConstruction) {
  // default constructor, copy and assignment are all deleted
  // minimally need a Transform3 and a PlanarBounds object (e.g.
  // ConeBounds) to construct
  Translation3 translation{0., 1., 2.};
  auto pTransform = Transform3(translation);
  double alpha(M_PI / 8.0);
  const bool symmetric(false);
  auto pCone = std::make_shared<const ConeBounds>(alpha, symmetric);
  // for some reason, this one doesn't exist
  // auto         pConeLayer = ConeLayer::create(pTransform, pCone);
  // BOOST_CHECK_EQUAL(pConeLayer->layerType(), LayerType::passive);
  // next level: need an array of Surfaces;
  // bounds object, rectangle type
  auto rBounds = std::make_shared<const RectangleBounds>(1., 1.);
  /// Constructor with transform pointer
  const std::vector<std::shared_ptr<const Surface>> aSurfaces{
      Surface::makeShared<PlaneSurface>(Transform3::Identity(), rBounds),
      Surface::makeShared<PlaneSurface>(Transform3::Identity(), rBounds)};
  const double thickness(1.0);
  auto pConeLayerFromSurfaces = ConeLayer::create(pTransform, pCone, nullptr);
  BOOST_CHECK_EQUAL(pConeLayerFromSurfaces->layerType(), LayerType::active);
  // construct with thickness:
  auto pConeLayerWithThickness =
      ConeLayer::create(pTransform, pCone, nullptr, thickness);
  BOOST_CHECK_EQUAL(pConeLayerWithThickness->thickness(), thickness);
  // with an approach descriptor...
  std::unique_ptr<ApproachDescriptor> ad(
      new GenericApproachDescriptor(aSurfaces));
  auto adPtr = ad.get();
  auto pConeLayerWithApproachDescriptor =
      ConeLayer::create(pTransform, pCone, nullptr, thickness, std::move(ad));
  BOOST_CHECK_EQUAL(pConeLayerWithApproachDescriptor->approachDescriptor(),
                    adPtr);
  // with the layerType specified...
  auto pConeLayerWithLayerType = ConeLayer::create(
      pTransform, pCone, nullptr, thickness, std::move(ad), LayerType::passive);
  BOOST_CHECK_EQUAL(pConeLayerWithLayerType->layerType(), LayerType::passive);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test::Layers
