// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/CuboidVolumeBounds.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/Volume.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/BinningType.hpp"

#include <cmath>
#include <limits>
#include <memory>
#include <utility>

namespace Acts::Test {

BOOST_AUTO_TEST_CASE(VolumeTest) {
  using namespace Acts::UnitLiterals;
  double eps = std::numeric_limits<double>::epsilon();

  // Build a translation
  Vector3 translation{1_mm, 2_mm, 3_mm};

  // Build a translation
  ActsMatrix<3, 3> rotation = RotationMatrix3::Identity();
  double rotationAngle = 60_degree;
  Vector3 xPos(cos(rotationAngle), 0., sin(rotationAngle));
  Vector3 yPos(0., 1., 0.);
  Vector3 zPos(-sin(rotationAngle), 0., cos(rotationAngle));
  rotation.col(0) = xPos;
  rotation.col(1) = yPos;
  rotation.col(2) = zPos;

  // Build a transform
  Transform3 transform(Transform3::Identity() * rotation);
  transform.translation() = translation;
  // Build the bounds
  CuboidVolumeBounds bounds(4_mm, 5_mm, 6_mm);

  // Build and test the volume
  Volume volume(transform, std::make_shared<CuboidVolumeBounds>(bounds));
  BOOST_CHECK_EQUAL(volume.transform().matrix(), transform.matrix());
  CHECK_CLOSE_ABS(volume.itransform().matrix(), transform.inverse().matrix(),
                  eps);
  BOOST_CHECK_EQUAL(volume.center(), translation);
  auto vBounds = static_cast<const decltype(bounds)*>(&volume.volumeBounds());
  BOOST_CHECK_EQUAL(*vBounds, bounds);

  // Build and test a shifted volume
  Transform3 shift(Transform3::Identity());
  Vector3 shiftTranslation{-4_mm, -5_mm, -6_mm};
  shift.translation() = shiftTranslation;
  Volume volumeShift(volume, shift);
  BOOST_CHECK_EQUAL(volumeShift.center(),
                    (shift * volume.transform()).translation());
  BOOST_CHECK_EQUAL(volumeShift.transform().rotation(),
                    volume.transform().rotation());

  // Inside/Outside check
  BOOST_CHECK(volume.inside(translation));
  BOOST_CHECK(!volume.inside({10_mm, 2_mm, 3_mm}));
  BOOST_CHECK(volume.inside({10_mm, 2_mm, 3_mm}, 2_mm));

  // Binning test
  GeometryContext gctx;
  BOOST_CHECK_EQUAL(volume.binningPosition(gctx, BinningValue::binX),
                    volume.center());
}

BOOST_AUTO_TEST_CASE(VolumeUpdateTest) {
  using namespace Acts::UnitLiterals;
  auto volBounds = std::make_shared<CuboidVolumeBounds>(4_mm, 5_mm, 6_mm);
  auto volBounds2 = std::make_shared<CuboidVolumeBounds>(4_mm, 5_mm, 8_mm);

  Transform3 trf = Transform3::Identity();

  Volume volume(trf, volBounds);

  // Only update the bounds, keep the transform the same
  volume.update(volBounds2, std::nullopt);
  BOOST_CHECK_EQUAL(&volume.volumeBounds(), volBounds2.get());
  BOOST_CHECK_EQUAL(volume.transform().matrix(), trf.matrix());

  // Update the bounds and the transform
  Transform3 trf2{Translation3{1_mm, 2_mm, 3_mm}};
  volume.update(volBounds, trf2);
  BOOST_CHECK_EQUAL(&volume.volumeBounds(), volBounds.get());
  BOOST_CHECK_EQUAL(volume.transform().matrix(), trf2.matrix());
}

}  // namespace Acts::Test
