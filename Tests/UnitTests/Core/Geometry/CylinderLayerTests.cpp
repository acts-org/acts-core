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
#include "Acts/Geometry/CylinderLayer.hpp"
#include "Acts/Geometry/GenericApproachDescriptor.hpp"
#include "Acts/Geometry/Layer.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceArray.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace Acts::Test::Layers {

BOOST_AUTO_TEST_SUITE(Layers)

/// Unit test for creating compliant/non-compliant CylinderLayer object
BOOST_AUTO_TEST_CASE(CylinderLayerConstruction) {
  // default constructor, copy and assignment are all deleted
  // minimally need a Transform3 and a PlanarBounds object (e.g.
  // CylinderBounds) to construct
  Translation3 translation{0., 1., 2.};
  auto pTransform = Transform3(translation);
  double radius(0.5), halfz(10.);
  auto pCylinder = std::make_shared<const CylinderBounds>(radius, halfz);
  auto pCylinderLayer =
      CylinderLayer::create(pTransform, pCylinder, nullptr, 1.);
  BOOST_CHECK_EQUAL(pCylinderLayer->layerType(), LayerType::passive);
  // next level: need an array of Surfaces;
  // bounds object, rectangle type
  auto rBounds = std::make_shared<const RectangleBounds>(1., 1.);
  /// Construction
  const std::vector<std::shared_ptr<const Surface>> aSurfaces{
      Surface::makeShared<PlaneSurface>(Transform3::Identity(), rBounds),
      Surface::makeShared<PlaneSurface>(Transform3::Identity(), rBounds)};
  const double thickness(1.0);
  auto pCylinderLayerFromSurfaces =
      CylinderLayer::create(pTransform, pCylinder, nullptr, thickness);
  BOOST_CHECK_EQUAL(pCylinderLayerFromSurfaces->layerType(),
                    LayerType::passive);
  // construct with thickness:
  auto pCylinderLayerWithThickness =
      CylinderLayer::create(pTransform, pCylinder, nullptr, thickness);
  CHECK_CLOSE_REL(pCylinderLayerWithThickness->thickness(), thickness, 1e-6);
  // with an approach descriptor...
  std::unique_ptr<ApproachDescriptor> ad(
      new GenericApproachDescriptor(aSurfaces));
  auto adPtr = ad.get();
  auto pCylinderLayerWithApproachDescriptor = CylinderLayer::create(
      pTransform, pCylinder, nullptr, thickness, std::move(ad));
  BOOST_CHECK_EQUAL(pCylinderLayerWithApproachDescriptor->approachDescriptor(),
                    adPtr);
  // with the layerType specified...
  auto pCylinderLayerWithLayerType =
      CylinderLayer::create(pTransform, pCylinder, nullptr, thickness,
                            std::move(ad), LayerType::passive);
  BOOST_CHECK_EQUAL(pCylinderLayerWithLayerType->layerType(),
                    LayerType::passive);
}

/// Unit test for testing Layer properties
BOOST_AUTO_TEST_CASE(CylinderLayerProperties) {
  Translation3 translation{0., 1., 2.};
  auto pTransform = Transform3(translation);
  double radius(0.5), halfz(10.);
  auto pCylinder = std::make_shared<const CylinderBounds>(radius, halfz);
  auto pCylinderLayer =
      CylinderLayer::create(pTransform, pCylinder, nullptr, 1.);
  // auto planeSurface = pCylinderLayer->surfaceRepresentation();
  BOOST_CHECK_EQUAL(pCylinderLayer->surfaceRepresentation().name(),
                    std::string("Acts::CylinderSurface"));
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test::Layers
