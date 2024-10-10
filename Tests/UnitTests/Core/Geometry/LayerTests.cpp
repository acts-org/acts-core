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
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceArray.hpp"

#include <cmath>
#include <memory>
#include <utility>
#include <vector>

#include "../Surfaces/SurfaceStub.hpp"
#include "LayerStub.hpp"

namespace Acts::Test {
// Create a test context
GeometryContext tgContext = GeometryContext();
}  // namespace Acts::Test

namespace Acts::Test::Layers {

BOOST_AUTO_TEST_SUITE(Layers)

/// Unit test for creating compliant/non-compliant Layer object
BOOST_AUTO_TEST_CASE(LayerConstruction) {
  // Descendant Layer objects also inherit from Surface objects, which
  // delete the default constructor
  //
  /// Minimum possible construction (default constructor is deleted)
  LayerStub minallyConstructed(nullptr);
  BOOST_CHECK(minallyConstructed.constructedOk());
  /// Need an approach descriptor for the next level of complexity:
  std::vector<std::shared_ptr<const Surface>> aSurfaces{
      Surface::makeShared<SurfaceStub>(), Surface::makeShared<SurfaceStub>()};
  std::unique_ptr<ApproachDescriptor> ad(
      new GenericApproachDescriptor(aSurfaces));
  const double thickness(1.0);
  LayerStub approachDescriptorConstructed(nullptr, thickness, std::move(ad));
  /// Construction with (minimal) approach descriptor
  BOOST_CHECK(approachDescriptorConstructed.constructedOk());
  // Copy construction is deleted
}

/// Unit test for testing Layer properties
BOOST_AUTO_TEST_CASE(LayerProperties) {
  // Make a dummy layer to play with
  // bounds object, rectangle type
  auto rBounds = std::make_shared<const RectangleBounds>(1., 1.);
  /// Constructor
  const std::vector<std::shared_ptr<const Surface>> aSurfaces{
      Surface::makeShared<PlaneSurface>(Transform3::Identity(), rBounds),
      Surface::makeShared<PlaneSurface>(Transform3::Identity(), rBounds)};
  std::unique_ptr<ApproachDescriptor> ad(
      new GenericApproachDescriptor(aSurfaces));
  auto adPtr = ad.get();
  const double thickness(1.0);
  LayerStub layerStub(nullptr, thickness, std::move(ad));
  //
  /// surfaceArray()
  BOOST_CHECK_EQUAL(layerStub.surfaceArray(), nullptr);
  /// thickness()
  BOOST_CHECK_EQUAL(layerStub.thickness(), thickness);
  // onLayer() is templated; can't find implementation!
  /// isOnLayer() (delegates to the Surface 'isOnSurface()')
  const Vector3 pos{0.0, 0.0, 0.0};
  const Vector3 pos2{100., 100., std::nan("")};
  BOOST_CHECK(layerStub.isOnLayer(tgContext, pos));
  // this should fail, but globalToLocal has hard-coded return values, so it
  // succeeds
  BOOST_CHECK(layerStub.isOnLayer(tgContext, pos2));
  /// approachDescriptor(), retrieved as a pointer.
  BOOST_CHECK_EQUAL(layerStub.approachDescriptor(), adPtr);
  const Vector3 gpos{0., 0., 1.0};
  const Vector3 direction{0., 0., -1.};
  /// nextLayer()
  BOOST_CHECK(!(layerStub.nextLayer(tgContext, gpos, direction)));
  /// trackingVolume()
  BOOST_CHECK(!layerStub.trackingVolume());
  // BOOST_TEST_CHECKPOINT("Before ending test");
  // deletion results in "memory access violation at address: 0x00000071: no
  // mapping at fault address"
  // delete abstractVolumePtr;
  /// layerType()
  BOOST_CHECK_EQUAL(layerStub.layerType(), LayerType::passive);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test::Layers
