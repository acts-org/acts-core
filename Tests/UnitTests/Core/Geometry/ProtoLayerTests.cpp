// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/ProtoLayer.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/RangeXD.hpp"

#include <array>
#include <cmath>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace Acts::Test::Layers {

BOOST_AUTO_TEST_SUITE(Geometry)

BOOST_AUTO_TEST_CASE(ProtoLayerTests) {
  GeometryContext tgContext = GeometryContext();

  // Create a proto layer with 4 surfaces on the x/y grid
  auto recBounds = std::make_shared<RectangleBounds>(3., 6.);

  // Planar definitions to help construct the boundary surfaces
  static const Transform3 planeYZ = AngleAxis3(0.5 * M_PI, Vector3::UnitY()) *
                                    AngleAxis3(0.5 * M_PI, Vector3::UnitZ()) *
                                    Transform3::Identity();
  static const Transform3 planeZX = AngleAxis3(-0.5 * M_PI, Vector3::UnitX()) *
                                    AngleAxis3(-0.5 * M_PI, Vector3::UnitZ()) *
                                    Transform3::Identity();

  std::vector<std::shared_ptr<const Surface>> surfaceStore;
  surfaceStore.reserve(100);

  auto createProtoLayer = [&](const Transform3& trf,
                              bool shared = false) -> ProtoLayer {
    auto atNegX = Surface::makeShared<PlaneSurface>(
        Transform3(trf * Translation3(Vector3(-3., 0., 0.)) * planeYZ),
        recBounds);

    auto atPosX = Surface::makeShared<PlaneSurface>(
        Transform3(trf * Translation3(Vector3(3., 0., 0.)) * planeYZ),
        recBounds);

    auto atNegY = Surface::makeShared<PlaneSurface>(
        Transform3(trf * Translation3(Vector3(0., -3, 0.)) * planeZX),
        recBounds);

    auto atPosY = Surface::makeShared<PlaneSurface>(
        Transform3(trf * Translation3(Vector3(0., 3., 0.)) * planeZX),
        recBounds);

    std::vector<std::shared_ptr<const Surface>> sharedSurfaces = {
        atNegX, atNegY, atPosX, atPosY};
    surfaceStore.insert(surfaceStore.begin(), sharedSurfaces.begin(),
                        sharedSurfaces.end());
    if (!shared) {
      std::vector<const Surface*> surfaces = {atNegX.get(), atNegY.get(),
                                              atPosX.get(), atPosY.get()};

      return ProtoLayer(tgContext, surfaces);
    }
    return ProtoLayer(tgContext, sharedSurfaces);
  };

  // Test 0 - check constructor with surfaces and shared surfaces
  auto pLayerSf = createProtoLayer(Transform3::Identity());
  auto pLayerSfShared = createProtoLayer(Transform3::Identity());

  BOOST_CHECK(pLayerSf.extent.range() == pLayerSfShared.extent.range());
  BOOST_CHECK(pLayerSf.envelope == pLayerSfShared.envelope);

  // CHECK That you have 4 surfaces
  BOOST_CHECK_EQUAL(pLayerSf.surfaces().size(), 4);
  // Add one surface from a detector element (to test thickness)
  auto rB = std::make_shared<RectangleBounds>(30., 60.);

  // Create the detector element
  auto addSurface =
      Surface::makeShared<PlaneSurface>(Transform3::Identity(), rB);

  pLayerSf.add(tgContext, *addSurface.get());
  // CHECK That if you now have 5 surfaces
  BOOST_CHECK_EQUAL(pLayerSf.surfaces().size(), 5);

  // That should invalidate the ranges
  BOOST_CHECK(!(pLayerSf.extent.range() == pLayerSfShared.extent.range()));

  // Test 1 - identity transform
  auto protoLayer = createProtoLayer(Transform3::Identity());

  CHECK_CLOSE_ABS(protoLayer.range(BinningValue::binX), 12., 1e-8);
  CHECK_CLOSE_ABS(protoLayer.medium(BinningValue::binX), 0., 1e-8);
  CHECK_CLOSE_ABS(protoLayer.min(BinningValue::binX), -6., 1e-8);
  CHECK_CLOSE_ABS(protoLayer.max(BinningValue::binX), 6., 1e-8);
  CHECK_CLOSE_ABS(protoLayer.range(BinningValue::binY), 6., 1e-8);
  CHECK_CLOSE_ABS(protoLayer.medium(BinningValue::binY), 0., 1e-8);
  CHECK_CLOSE_ABS(protoLayer.min(BinningValue::binY), -3., 1e-8);
  CHECK_CLOSE_ABS(protoLayer.max(BinningValue::binY), 3., 1e-8);
  CHECK_CLOSE_ABS(protoLayer.range(BinningValue::binZ), 12., 1e-8);
  CHECK_CLOSE_ABS(protoLayer.medium(BinningValue::binZ), 0., 1e-8);
  CHECK_CLOSE_ABS(protoLayer.min(BinningValue::binZ), -6., 1e-8);
  CHECK_CLOSE_ABS(protoLayer.max(BinningValue::binZ), 6., 1e-8);
  CHECK_CLOSE_ABS(protoLayer.max(BinningValue::binR), std::hypot(3, 6), 1e-8);
  CHECK_CLOSE_ABS(protoLayer.min(BinningValue::binR), 3., 1e-8);

  // Test 1a

  // Test 2 - rotate around Z-Axis, should leave R, Z untouched,
  // only preserves medium values
  auto protoLayerRot = createProtoLayer(AngleAxis3(-0.345, Vector3::UnitZ()) *
                                        Transform3::Identity());

  BOOST_CHECK_NE(protoLayer.min(BinningValue::binX), -6.);
  CHECK_CLOSE_ABS(protoLayerRot.medium(BinningValue::binX), 0., 1e-8);
  CHECK_CLOSE_ABS(protoLayerRot.medium(BinningValue::binY), 0., 1e-8);
  CHECK_CLOSE_ABS(protoLayerRot.range(BinningValue::binZ), 12., 1e-8);
  CHECK_CLOSE_ABS(protoLayerRot.medium(BinningValue::binZ), 0., 1e-8);
  CHECK_CLOSE_ABS(protoLayerRot.min(BinningValue::binZ), -6., 1e-8);
  CHECK_CLOSE_ABS(protoLayerRot.max(BinningValue::binZ), 6., 1e-8);
  CHECK_CLOSE_ABS(protoLayerRot.min(BinningValue::binR), 3., 1e-8);
  CHECK_CLOSE_ABS(protoLayerRot.max(BinningValue::binR), std::hypot(3, 6),
                  1e-8);

  std::stringstream sstream;
  protoLayerRot.toStream(sstream);
  std::string oString = R"(ProtoLayer with dimensions (min/max)
Extent in space :
  - value :      binX | range = [-6.66104, 6.66104]
  - value :      binY | range = [-4.85241, 4.85241]
  - value :      binZ | range = [-6, 6]
  - value :      binR | range = [3, 6.7082]
  - value :    binPhi | range = [-3.02295, 2.33295]
  - value :   binRPhi | range = [-20.2785, 15.6499]
  - value :      binH | range = [0.61548, 2.52611]
  - value :    binEta | range = [-1.14622, 1.14622]
  - value :    binMag | range = [7.34847, 7.34847]
)";
  BOOST_CHECK_EQUAL(sstream.str(), oString);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test::Layers
