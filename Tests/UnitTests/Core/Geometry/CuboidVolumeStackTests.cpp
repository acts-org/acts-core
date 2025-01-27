// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/data/test_case.hpp>
#include <boost/test/detail/log_level.hpp>
#include <boost/test/tools/context.hpp>
#include <boost/test/tools/old/interface.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>
#include <boost/test/unit_test_parameters.hpp>
#include <boost/test/unit_test_suite.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/CuboidVolumeBounds.hpp"
#include "Acts/Geometry/CuboidVolumeStack.hpp"
#include "Acts/Geometry/VolumeAttachmentStrategy.hpp"
#include "Acts/Geometry/VolumeResizeStrategy.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/AxisDefinitions.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/ThrowAssert.hpp"
#include "Acts/Utilities/Zip.hpp"

#include <cassert>
#include <initializer_list>
#include <stdexcept>
#include <utility>

using namespace Acts::UnitLiterals;

namespace Acts::Test {

auto logger = Acts::getDefaultLogger("UnitTests", Acts::Logging::VERBOSE);

struct Fixture {
  Logging::Level m_level;
  Fixture() {
    m_level = Acts::Logging::getFailureThreshold();
    Acts::Logging::setFailureThreshold(Acts::Logging::FATAL);
  }

  ~Fixture() { Acts::Logging::setFailureThreshold(m_level); }
};

BOOST_FIXTURE_TEST_SUITE(Geometry, Fixture)

static const std::vector<VolumeAttachmentStrategy> strategies = {
    VolumeAttachmentStrategy::Gap,
    VolumeAttachmentStrategy::First,
    VolumeAttachmentStrategy::Second,
    VolumeAttachmentStrategy::Midpoint,
};

static const std::vector<VolumeResizeStrategy> resizeStrategies = {
    VolumeResizeStrategy::Expand,
    VolumeResizeStrategy::Gap,
};

BOOST_AUTO_TEST_SUITE(CuboidVolumeStackTest)

BOOST_DATA_TEST_CASE(BaselineLocal,
                     (boost::unit_test::data::xrange(-135, 180, 45) *
                      boost::unit_test::data::xrange(0, 2, 1) *
                      boost::unit_test::data::make(0.8, 1.0, 1.2) *
                      boost::unit_test::data::make(Vector3{0_mm, 0_mm, 0_mm},
                                                   Vector3{20_mm, 0_mm, 0_mm},
                                                   Vector3{0_mm, 20_mm, 0_mm},
                                                   Vector3{20_mm, 20_mm, 0_mm},
                                                   Vector3{0_mm, 0_mm, 20_mm}) *
                      boost::unit_test::data::make(strategies) *
                      boost::unit_test::data::make(Acts::AxisDirection::AxisX,
                                                   Acts::AxisDirection::AxisY,
                                                   Acts::AxisDirection::AxisZ)),
                     angle, rotate, shift, offset, strategy, dir) {
  double halfDir = 400_mm;

  auto [dirOrth1, dirOrth2] = CuboidVolumeStack::getOrthogonalAxes(dir);

  auto dirIdx = CuboidVolumeStack::axisToIndex(dir);
  auto dirOrth1Idx = CuboidVolumeStack::axisToIndex(dirOrth1);

  auto boundDir = CuboidVolumeBounds::fromAxisDirection(dir);
  auto boundDirOrth1 = CuboidVolumeBounds::fromAxisDirection(dirOrth1);
  auto boundDirOrth2 = CuboidVolumeBounds::fromAxisDirection(dirOrth2);

  auto bounds1 = std::make_shared<CuboidVolumeBounds>(
      std::initializer_list<std::pair<CuboidVolumeBounds::BoundValues, double>>{
          {boundDir, halfDir},
          {boundDirOrth1, 100_mm},
          {boundDirOrth2, 400_mm}});

  auto bounds2 = std::make_shared<CuboidVolumeBounds>(
      std::initializer_list<std::pair<CuboidVolumeBounds::BoundValues, double>>{
          {boundDir, halfDir},
          {boundDirOrth1, 200_mm},
          {boundDirOrth2, 600_mm}});

  auto bounds3 = std::make_shared<CuboidVolumeBounds>(
      std::initializer_list<std::pair<CuboidVolumeBounds::BoundValues, double>>{
          {boundDir, halfDir},
          {boundDirOrth1, 300_mm},
          {boundDirOrth2, 500_mm}});

  Transform3 base = AngleAxis3(angle * 1_degree, Vector3::Unit(dirOrth1Idx)) *
                    Translation3(offset);

  Translation3 translation1(Vector3::Unit(dirIdx) * (-2 * halfDir * shift));
  Transform3 transform1 = base * translation1;
  auto vol1 = std::make_shared<Volume>(transform1, bounds1);

  Transform3 transform2 = base;
  auto vol2 = std::make_shared<Volume>(transform2, bounds2);

  Translation3 translation3(Vector3::Unit(dirIdx) * (2 * halfDir * shift));
  Transform3 transform3 = base * translation3;
  auto vol3 = std::make_shared<Volume>(transform3, bounds3);

  std::vector<Volume*> volumes = {vol1.get(), vol2.get(), vol3.get()};
  // Rotate to simulate unsorted volumes: all results should be the same!
  std::rotate(volumes.begin(), volumes.begin() + rotate, volumes.end());

  auto origVolumes = volumes;

  std::vector<CuboidVolumeBounds> originalBounds;
  std::transform(volumes.begin(), volumes.end(),
                 std::back_inserter(originalBounds), [](const auto& vol) {
                   const auto* res =
                       dynamic_cast<CuboidVolumeBounds*>(&vol->volumeBounds());
                   throw_assert(res != nullptr, "");
                   return *res;
                 });

  if (shift < 1.0) {
    BOOST_CHECK_THROW(CuboidVolumeStack(volumes, dir, strategy,
                                        VolumeResizeStrategy::Gap, *logger),
                      std::invalid_argument);
    return;
  }
  CuboidVolumeStack stack(volumes, dir, strategy, VolumeResizeStrategy::Gap,
                          *logger);

  auto stackBounds =
      dynamic_cast<const CuboidVolumeBounds*>(&stack.volumeBounds());
  BOOST_REQUIRE(stackBounds != nullptr);

  BOOST_CHECK_CLOSE(stackBounds->get(boundDirOrth1), 300_mm, 1e-6);
  BOOST_CHECK_CLOSE(stackBounds->get(boundDirOrth2), 600_mm, 1e-6);
  BOOST_CHECK_CLOSE(stackBounds->get(boundDir), halfDir + 2 * halfDir * shift,
                    1e-6);
  CHECK_CLOSE_OR_SMALL(stack.transform().matrix(), base.matrix(), 1e-10, 1e-12);

  // All volumes (including gaps) are cuboids and have the same orthogonal
  // bounds
  for (const auto& volume : volumes) {
    const auto* cuboidBounds =
        dynamic_cast<const CuboidVolumeBounds*>(&volume->volumeBounds());
    BOOST_REQUIRE(cuboidBounds != nullptr);
    BOOST_CHECK_CLOSE(cuboidBounds->get(boundDirOrth1), 300_mm, 1e-6);
    BOOST_CHECK_CLOSE(cuboidBounds->get(boundDirOrth2), 600_mm, 1e-6);
  }

  // Volumes are sorted in (local) stacking direction
  for (std::size_t i = 0; i < volumes.size() - 1; ++i) {
    const auto& a = volumes.at(i);
    const auto& b = volumes.at(i + 1);

    BOOST_CHECK_LT((base.inverse() * a->center())[dirIdx],
                   (base.inverse() * b->center())[dirIdx]);
  }

  if (shift <= 1.0) {
    // No gap volumes were added
    BOOST_CHECK_EQUAL(volumes.size(), 3);

    // No expansion, original volumes did not move
    BOOST_CHECK_EQUAL(vol1->transform().matrix(), transform1.matrix());
    BOOST_CHECK_EQUAL(vol2->transform().matrix(), transform2.matrix());
    BOOST_CHECK_EQUAL(vol3->transform().matrix(), transform3.matrix());

    for (const auto& [volume, bounds] : zip(origVolumes, originalBounds)) {
      const auto* newBounds =
          dynamic_cast<const CuboidVolumeBounds*>(&volume->volumeBounds());
      BOOST_CHECK_CLOSE(newBounds->get(boundDir), bounds.get(boundDir), 1e-6);
    }
  } else {
    if (strategy == VolumeAttachmentStrategy::Gap) {
      // Gap volumes were added
      BOOST_CHECK_EQUAL(volumes.size(), 5);
      auto gap1 = volumes.at(1);
      auto gap2 = volumes.at(3);

      BOOST_TEST_MESSAGE("Gap 1: " << gap1->transform().matrix());
      BOOST_TEST_MESSAGE("Gap 2: " << gap2->transform().matrix());

      const auto* gapBounds1 =
          dynamic_cast<const CuboidVolumeBounds*>(&gap1->volumeBounds());
      const auto* gapBounds2 =
          dynamic_cast<const CuboidVolumeBounds*>(&gap2->volumeBounds());

      double gapHlDir = (shift - 1.0) * halfDir;

      BOOST_CHECK(std::abs(gapBounds1->get(boundDir) - gapHlDir) < 1e-12);
      BOOST_CHECK(std::abs(gapBounds2->get(boundDir) - gapHlDir) < 1e-12);

      double gap1Dir = (-2 * halfDir * shift) + halfDir + gapHlDir;
      double gap2Dir = (2 * halfDir * shift) - halfDir - gapHlDir;

      Translation3 gap1Translation(Vector3::Unit(dirIdx) * gap1Dir);
      Translation3 gap2Translation(Vector3::Unit(dirIdx) * gap2Dir);

      Transform3 gap1Transform = base * gap1Translation;
      Transform3 gap2Transform = base * gap2Translation;

      CHECK_CLOSE_OR_SMALL(gap1->transform().matrix(), gap1Transform.matrix(),
                           1e-10, 1e-12);
      CHECK_CLOSE_OR_SMALL(gap2->transform().matrix(), gap2Transform.matrix(),
                           1e-10, 1e-12);

      // Original volumes did not changes bounds
      for (const auto& [volume, bounds] : zip(origVolumes, originalBounds)) {
        const auto* newBounds =
            dynamic_cast<const CuboidVolumeBounds*>(&volume->volumeBounds());
        BOOST_CHECK_CLOSE(newBounds->get(boundDir), bounds.get(boundDir), 1e-6);
      }

      // No expansion, original volumes did not move
      BOOST_CHECK_EQUAL(vol1->transform().matrix(), transform1.matrix());
      BOOST_CHECK_EQUAL(vol2->transform().matrix(), transform2.matrix());
      BOOST_CHECK_EQUAL(vol3->transform().matrix(), transform3.matrix());
    } else if (strategy == VolumeAttachmentStrategy::First) {
      // No gap volumes were added
      BOOST_CHECK_EQUAL(volumes.size(), 3);

      double wGap = (shift - 1.0) * halfDir * 2;

      // Volume 1 got bigger and shifted right
      auto newBounds1 =
          dynamic_cast<const CuboidVolumeBounds*>(&vol1->volumeBounds());
      BOOST_CHECK_CLOSE(newBounds1->get(boundDir), halfDir + wGap / 2.0, 1e-6);
      double pDir1 = -2 * halfDir * shift + wGap / 2.0;
      Translation3 expectedTranslation1(Vector3::Unit(dirIdx) * pDir1);
      Transform3 expectedTransform1 = base * expectedTranslation1;
      CHECK_CLOSE_OR_SMALL(vol1->transform().matrix(),
                           expectedTransform1.matrix(), 1e-10, 1e-12);

      // Volume 2 got bigger and shifted left
      auto newBounds2 =
          dynamic_cast<const CuboidVolumeBounds*>(&vol2->volumeBounds());
      BOOST_CHECK_CLOSE(newBounds2->get(boundDir), halfDir + wGap / 2.0, 1e-6);
      double pDir2 = wGap / 2.0;
      Translation3 expectedTranslation2(Vector3::Unit(dirIdx) * pDir2);
      Transform3 expectedTransform2 = base * expectedTranslation2;
      CHECK_CLOSE_OR_SMALL(vol2->transform().matrix(),
                           expectedTransform2.matrix(), 1e-10, 1e-12);

      // Volume 3 stayed the same
      auto newBounds3 =
          dynamic_cast<const CuboidVolumeBounds*>(&vol3->volumeBounds());
      BOOST_CHECK_CLOSE(newBounds3->get(boundDir), halfDir, 1e-6);
      double pDir3 = 2 * halfDir * shift;
      Translation3 expectedTranslation3(Vector3::Unit(dirIdx) * pDir3);
      Transform3 expectedTransform3 = base * expectedTranslation3;
      CHECK_CLOSE_OR_SMALL(vol3->transform().matrix(),
                           expectedTransform3.matrix(), 1e-10, 1e-12);
    } else if (strategy == VolumeAttachmentStrategy::Second) {
      // No gap volumes were added
      BOOST_CHECK_EQUAL(volumes.size(), 3);

      double wGap = (shift - 1.0) * halfDir * 2;

      // Volume 1 stayed the same
      auto newBounds1 =
          dynamic_cast<const CuboidVolumeBounds*>(&vol1->volumeBounds());
      BOOST_CHECK_CLOSE(newBounds1->get(boundDir), halfDir, 1e-6);
      double pDir1 = -2 * halfDir * shift;
      Translation3 expectedTranslation1(Vector3::Unit(dirIdx) * pDir1);
      Transform3 expectedTransform1 = base * expectedTranslation1;
      CHECK_CLOSE_OR_SMALL(vol1->transform().matrix(),
                           expectedTransform1.matrix(), 1e-10, 1e-12);

      // Volume 2 got bigger and shifted left
      auto newBounds2 =
          dynamic_cast<const CuboidVolumeBounds*>(&vol2->volumeBounds());
      BOOST_CHECK_CLOSE(newBounds2->get(boundDir), halfDir + wGap / 2.0, 1e-6);
      double pDir2 = -wGap / 2.0;
      Translation3 expectedTranslation2(Vector3::Unit(dirIdx) * pDir2);
      Transform3 expectedTransform2 = base * expectedTranslation2;
      CHECK_CLOSE_OR_SMALL(vol2->transform().matrix(),
                           expectedTransform2.matrix(), 1e-10, 1e-12);

      // Volume 3 got bigger and shifted left
      auto newBounds3 =
          dynamic_cast<const CuboidVolumeBounds*>(&vol3->volumeBounds());
      BOOST_CHECK_CLOSE(newBounds3->get(boundDir), halfDir + wGap / 2.0, 1e-6);
      double pDir3 = 2 * halfDir * shift - wGap / 2.0;
      Translation3 expectedTranslation3(Vector3::Unit(dirIdx) * pDir3);
      Transform3 expectedTransform3 = base * expectedTranslation3;
      CHECK_CLOSE_OR_SMALL(vol3->transform().matrix(),
                           expectedTransform3.matrix(), 1e-10, 1e-12);
    } else if (strategy == VolumeAttachmentStrategy::Midpoint) {
      // No gap volumes were added
      BOOST_CHECK_EQUAL(volumes.size(), 3);

      double wGap = (shift - 1.0) * halfDir * 2;

      // Volume 1 got bigger and shifted right
      auto newBounds1 =
          dynamic_cast<const CuboidVolumeBounds*>(&vol1->volumeBounds());
      BOOST_CHECK_CLOSE(newBounds1->get(boundDir), halfDir + wGap / 4.0, 1e-6);
      double pDir1 = -2 * halfDir * shift + wGap / 4.0;
      Translation3 expectedTranslation1(Vector3::Unit(dirIdx) * pDir1);
      Transform3 expectedTransform1 = base * expectedTranslation1;
      CHECK_CLOSE_OR_SMALL(vol1->transform().matrix(),
                           expectedTransform1.matrix(), 1e-10, 1e-12);

      // Volume 2 got bigger but didn't move
      auto newBounds2 =
          dynamic_cast<const CuboidVolumeBounds*>(&vol2->volumeBounds());
      BOOST_CHECK_CLOSE(newBounds2->get(boundDir), halfDir + wGap / 2.0, 1e-6);
      CHECK_CLOSE_OR_SMALL(vol2->transform().matrix(), base.matrix(), 1e-10,
                           1e-12);

      // Volume 3 got bigger and shifted left
      auto newBounds3 =
          dynamic_cast<const CuboidVolumeBounds*>(&vol3->volumeBounds());
      BOOST_CHECK_CLOSE(newBounds3->get(boundDir), halfDir + wGap / 4.0, 1e-6);
      double pDir3 = 2 * halfDir * shift - wGap / 4.0;
      Translation3 expectedTranslation3(Vector3::Unit(dirIdx) * pDir3);
      Transform3 expectedTransform3 = base * expectedTranslation3;
      CHECK_CLOSE_OR_SMALL(vol3->transform().matrix(),
                           expectedTransform3.matrix(), 1e-10, 1e-12);
    }
  }
}

BOOST_DATA_TEST_CASE(BaselineGlobal,
                     (boost::unit_test::data::xrange(-135, 180, 45) *
                      boost::unit_test::data::xrange(0, 2, 1) *
                      boost::unit_test::data::make(1.0, 1.2) *
                      boost::unit_test::data::make(Vector3{0_mm, 0_mm, 0_mm},
                                                   Vector3{20_mm, 0_mm, 0_mm},
                                                   Vector3{0_mm, 20_mm, 0_mm},
                                                   Vector3{20_mm, 20_mm, 0_mm},
                                                   Vector3{0_mm, 0_mm, 20_mm}) *
                      boost::unit_test::data::make(strategies)),
                     angle, rotate, shift, offset, strategy) {
  double halfDir = 400_mm;

  auto bounds1 = std::make_shared<CuboidVolumeBounds>(100_mm, 400_mm, halfDir);
  auto bounds2 = std::make_shared<CuboidVolumeBounds>(200_mm, 600_mm, halfDir);
  auto bounds3 = std::make_shared<CuboidVolumeBounds>(300_mm, 500_mm, halfDir);

  AngleAxis3 baseRotation = AngleAxis3(angle, Vector3::UnitX());
  Transform3 base = baseRotation * Translation3(offset);

  Vector3 orientation = baseRotation * Vector3::UnitZ();

  Translation3 translation1(Vector3::UnitZ() * (-2 * halfDir * shift));
  Transform3 transform1 = base * translation1;
  auto vol1Orientation = std::make_shared<Volume>(transform1, bounds1);
  auto vol1AxisDirection = std::make_shared<Volume>(transform1, bounds1);

  Transform3 transform2 = base;
  auto vol2Orientation = std::make_shared<Volume>(transform2, bounds2);
  auto vol2AxisDirection = std::make_shared<Volume>(transform2, bounds2);

  Translation3 translation3(Vector3::UnitZ() * (2 * halfDir * shift));
  Transform3 transform3 = base * translation3;
  auto vol3Orientation = std::make_shared<Volume>(transform3, bounds3);
  auto vol3AxisDirection = std::make_shared<Volume>(transform3, bounds3);

  std::vector<Volume*> volumesOrientation = {
      vol1Orientation.get(), vol2Orientation.get(), vol3Orientation.get()};
  std::vector<Volume*> volumesAxisDirection = {vol1AxisDirection.get(),
                                               vol2AxisDirection.get(),
                                               vol3AxisDirection.get()};

  // Rotate to simulate unsorted volumes: all results should be the same!
  std::rotate(volumesOrientation.begin(), volumesOrientation.begin() + rotate,
              volumesOrientation.end());
  std::rotate(volumesAxisDirection.begin(),
              volumesAxisDirection.begin() + rotate,
              volumesAxisDirection.end());

  CuboidVolumeStack stackOrientation(volumesOrientation, orientation, strategy,
                                     VolumeResizeStrategy::Gap, *logger);
  CuboidVolumeStack stackAxisDirection(volumesAxisDirection,
                                       AxisDirection::AxisZ, strategy,
                                       VolumeResizeStrategy::Gap, *logger);

  auto stackBoundsOrientation =
      dynamic_cast<const CuboidVolumeBounds*>(&stackOrientation.volumeBounds());
  auto stackBoundsAxisDirection = dynamic_cast<const CuboidVolumeBounds*>(
      &stackAxisDirection.volumeBounds());

  // Global vector direction and local axis direction should
  // produce the same results
  BOOST_CHECK_NE(stackBoundsOrientation, nullptr);
  BOOST_CHECK_NE(stackBoundsAxisDirection, nullptr);
  BOOST_CHECK_EQUAL(*stackBoundsOrientation, *stackBoundsAxisDirection);
  CHECK_CLOSE_OR_SMALL(stackOrientation.transform().matrix(),
                       stackAxisDirection.transform().matrix(), 1e-10, 1e-12);
}

BOOST_DATA_TEST_CASE(Asymmetric,
                     boost::unit_test::data::make(Acts::AxisDirection::AxisX,
                                                  Acts::AxisDirection::AxisY,
                                                  Acts::AxisDirection::AxisZ),
                     dir) {
  double halfDir1 = 200_mm;
  double pDir1 = -1100_mm;
  double halfDir2 = 600_mm;
  double pDir2 = -200_mm;
  double halfDir3 = 400_mm;
  double pDir3 = 850_mm;

  auto [dirOrth1, dirOrth2] = CuboidVolumeStack::getOrthogonalAxes(dir);

  auto dirIdx = CuboidVolumeStack::axisToIndex(dir);

  auto boundDir = CuboidVolumeBounds::fromAxisDirection(dir);
  auto boundDirOrth1 = CuboidVolumeBounds::fromAxisDirection(dirOrth1);
  auto boundDirOrth2 = CuboidVolumeBounds::fromAxisDirection(dirOrth2);

  auto bounds1 = std::make_shared<CuboidVolumeBounds>(
      std::initializer_list<std::pair<CuboidVolumeBounds::BoundValues, double>>{
          {boundDir, halfDir1},
          {boundDirOrth1, 100_mm},
          {boundDirOrth2, 400_mm}});

  auto bounds2 = std::make_shared<CuboidVolumeBounds>(
      std::initializer_list<std::pair<CuboidVolumeBounds::BoundValues, double>>{
          {boundDir, halfDir2},
          {boundDirOrth1, 200_mm},
          {boundDirOrth2, 600_mm}});

  auto bounds3 = std::make_shared<CuboidVolumeBounds>(
      std::initializer_list<std::pair<CuboidVolumeBounds::BoundValues, double>>{
          {boundDir, halfDir3},
          {boundDirOrth1, 300_mm},
          {boundDirOrth2, 500_mm}});

  Translation3 translation1(Vector3::Unit(dirIdx) * pDir1);
  Transform3 transform1(translation1);
  auto vol1 = std::make_shared<Volume>(transform1, bounds1);

  Translation3 translation2(Vector3::Unit(dirIdx) * pDir2);
  Transform3 transform2(translation2);
  auto vol2 = std::make_shared<Volume>(transform2, bounds2);

  Translation3 translation3(Vector3::Unit(dirIdx) * pDir3);
  Transform3 transform3(translation3);
  auto vol3 = std::make_shared<Volume>(transform3, bounds3);

  std::vector<Volume*> volumes = {vol2.get(), vol1.get(), vol3.get()};

  CuboidVolumeStack stack(volumes, dir, VolumeAttachmentStrategy::Gap,
                          VolumeResizeStrategy::Gap, *logger);
  BOOST_CHECK_EQUAL(volumes.size(), 5);

  auto stackBounds =
      dynamic_cast<const CuboidVolumeBounds*>(&stack.volumeBounds());
  BOOST_REQUIRE(stackBounds != nullptr);

  BOOST_CHECK_CLOSE(stackBounds->get(boundDirOrth1), 300_mm, 1e-6);
  BOOST_CHECK_CLOSE(stackBounds->get(boundDirOrth2), 600_mm, 1e-6);
  BOOST_CHECK_CLOSE(stackBounds->get(boundDir),
                    (std::abs(pDir1 - halfDir1) + pDir3 + halfDir3) / 2.0,
                    1e-6);

  double midDir = (pDir1 - halfDir1 + pDir3 + halfDir3) / 2.0;
  Translation3 expectedTranslation(Vector3::Unit(dirIdx) * midDir);
  Transform3 expectedTransform = Transform3::Identity() * expectedTranslation;
  CHECK_CLOSE_OR_SMALL(stack.transform().matrix(), expectedTransform.matrix(),
                       1e-10, 1e-12);
}

BOOST_DATA_TEST_CASE(UpdateStack,
                     (boost::unit_test::data::xrange(-135, 180, 45) *
                      boost::unit_test::data::make(Vector3{0_mm, 0_mm, 0_mm},
                                                   Vector3{20_mm, 0_mm, 0_mm},
                                                   Vector3{0_mm, 20_mm, 0_mm},
                                                   Vector3{20_mm, 20_mm, 0_mm},
                                                   Vector3{0_mm, 0_mm, 20_mm}) *
                      boost::unit_test::data::make(-100_mm, 0_mm, 100_mm) *
                      boost::unit_test::data::make(resizeStrategies) *
                      boost::unit_test::data::make(Acts::AxisDirection::AxisX,
                                                   Acts::AxisDirection::AxisY,
                                                   Acts::AxisDirection::AxisZ)),
                     angle, offset, zshift, strategy, dir) {
  double halfDir = 400_mm;

  auto [dirOrth1, dirOrth2] = CuboidVolumeStack::getOrthogonalAxes(dir);

  auto dirIdx = CuboidVolumeStack::axisToIndex(dir);
  auto dirOrth1Idx = CuboidVolumeStack::axisToIndex(dirOrth1);

  auto boundDir = CuboidVolumeBounds::fromAxisDirection(dir);
  auto boundDirOrth1 = CuboidVolumeBounds::fromAxisDirection(dirOrth1);
  auto boundDirOrth2 = CuboidVolumeBounds::fromAxisDirection(dirOrth2);

  auto bounds1 = std::make_shared<CuboidVolumeBounds>(
      std::initializer_list<std::pair<CuboidVolumeBounds::BoundValues, double>>{
          {boundDir, halfDir},
          {boundDirOrth1, 100_mm},
          {boundDirOrth2, 600_mm}});

  auto bounds2 = std::make_shared<CuboidVolumeBounds>(
      std::initializer_list<std::pair<CuboidVolumeBounds::BoundValues, double>>{
          {boundDir, halfDir},
          {boundDirOrth1, 100_mm},
          {boundDirOrth2, 600_mm}});

  auto bounds3 = std::make_shared<CuboidVolumeBounds>(
      std::initializer_list<std::pair<CuboidVolumeBounds::BoundValues, double>>{
          {boundDir, halfDir},
          {boundDirOrth1, 100_mm},
          {boundDirOrth2, 600_mm}});

  Vector3 shift = Vector3::Unit(dirIdx) * zshift;
  Transform3 base = AngleAxis3(angle * 1_degree, Vector3::Unit(dirOrth1Idx)) *
                    Translation3(offset + shift);

  Translation3 translation1(Vector3::Unit(dirIdx) * -2 * halfDir);
  Transform3 transform1 = base * translation1;
  auto vol1 = std::make_shared<Volume>(transform1, bounds1);

  Transform3 transform2 = base;
  auto vol2 = std::make_shared<Volume>(transform2, bounds2);

  Translation3 translation3(Vector3::Unit(dirIdx) * 2 * halfDir);
  Transform3 transform3 = base * translation3;
  auto vol3 = std::make_shared<Volume>(transform3, bounds3);

  std::vector<Volume*> volumes = {vol1.get(), vol2.get(), vol3.get()};
  std::vector<Volume*> originalVolumes = volumes;

  std::vector<Transform3> originalTransforms = {transform1, transform2,
                                                transform3};

  CuboidVolumeStack stack(volumes, dir,
                          VolumeAttachmentStrategy::Gap,  // should not make a
                                                          // difference
                          strategy, *logger);

  const auto* originalBounds =
      dynamic_cast<const CuboidVolumeBounds*>(&stack.volumeBounds());

  auto assertOriginalBounds = [&]() {
    const auto* bounds =
        dynamic_cast<const CuboidVolumeBounds*>(&stack.volumeBounds());
    BOOST_REQUIRE(bounds != nullptr);
    BOOST_CHECK_EQUAL(bounds, originalBounds);
    BOOST_CHECK_CLOSE(bounds->get(boundDirOrth1), 100_mm, 1e-6);
    BOOST_CHECK_CLOSE(bounds->get(boundDirOrth2), 600_mm, 1e-6);
    BOOST_CHECK_CLOSE(bounds->get(boundDir), 3 * halfDir, 1e-6);
  };

  assertOriginalBounds();

  {
    // Assign a copy of the identical bounds gives identical bounds
    auto bounds = std::make_shared<CuboidVolumeBounds>(
        dynamic_cast<const CuboidVolumeBounds&>(stack.volumeBounds()));
    stack.update(bounds, std::nullopt, *logger);
    assertOriginalBounds();
  }

  {
    // Cannot decrease half length
    auto bounds = std::make_shared<CuboidVolumeBounds>(
        dynamic_cast<const CuboidVolumeBounds&>(stack.volumeBounds()));
    bounds->set(boundDirOrth1, 20_mm);
    BOOST_CHECK_THROW(stack.update(bounds, std::nullopt, *logger),
                      std::invalid_argument);
    assertOriginalBounds();
  }

  {
    // Cannot decrease half length
    auto bounds = std::make_shared<CuboidVolumeBounds>(
        dynamic_cast<const CuboidVolumeBounds&>(stack.volumeBounds()));
    bounds->set(boundDirOrth2, 200_mm);
    BOOST_CHECK_THROW(stack.update(bounds, std::nullopt, *logger),
                      std::invalid_argument);
    assertOriginalBounds();
  }

  {
    // Cannot decrease half length
    auto bounds = std::make_shared<CuboidVolumeBounds>(
        dynamic_cast<const CuboidVolumeBounds&>(stack.volumeBounds()));
    bounds->set(boundDir, 2 * halfDir);
    BOOST_CHECK_THROW(stack.update(bounds, std::nullopt, *logger),
                      std::invalid_argument);
    assertOriginalBounds();
  }

  {
    // Increase half length
    auto bounds = std::make_shared<CuboidVolumeBounds>(
        dynamic_cast<const CuboidVolumeBounds&>(stack.volumeBounds()));
    bounds->set(boundDirOrth1, 700_mm);
    stack.update(bounds, std::nullopt, *logger);
    const auto* updatedBounds =
        dynamic_cast<const CuboidVolumeBounds*>(&stack.volumeBounds());
    BOOST_REQUIRE(updatedBounds != nullptr);
    BOOST_CHECK_CLOSE(updatedBounds->get(boundDirOrth1), 700_mm, 1e-6);
    BOOST_CHECK_CLOSE(updatedBounds->get(boundDirOrth2), 600_mm, 1e-6);
    BOOST_CHECK_CLOSE(updatedBounds->get(boundDir), 3 * halfDir, 1e-6);

    // No gap volumes were added
    BOOST_CHECK_EQUAL(volumes.size(), 3);

    // All volumes increase half x to accommodate
    for (const auto& [volume, origTransform] :
         zip(volumes, originalTransforms)) {
      const auto* newBounds =
          dynamic_cast<const CuboidVolumeBounds*>(&volume->volumeBounds());
      BOOST_CHECK_CLOSE(newBounds->get(boundDirOrth1), 700_mm, 1e-6);
      BOOST_CHECK_CLOSE(newBounds->get(boundDirOrth2), 600_mm, 1e-6);
      BOOST_CHECK_CLOSE(newBounds->get(boundDir), halfDir, 1e-6);

      // Position stayed the same
      BOOST_CHECK_EQUAL(volume->transform().matrix(), origTransform.matrix());
    }
  }
  {
    // Increase half length
    auto bounds = std::make_shared<CuboidVolumeBounds>(
        dynamic_cast<const CuboidVolumeBounds&>(stack.volumeBounds()));
    bounds->set(boundDirOrth2, 700_mm);
    stack.update(bounds, std::nullopt, *logger);
    const auto* updatedBounds =
        dynamic_cast<const CuboidVolumeBounds*>(&stack.volumeBounds());
    BOOST_REQUIRE(updatedBounds != nullptr);
    BOOST_CHECK_CLOSE(updatedBounds->get(boundDirOrth1), 700_mm, 1e-6);
    BOOST_CHECK_CLOSE(updatedBounds->get(boundDirOrth2), 700_mm, 1e-6);
    BOOST_CHECK_CLOSE(updatedBounds->get(boundDir), 3 * halfDir, 1e-6);

    // No gap volumes were added
    BOOST_CHECK_EQUAL(volumes.size(), 3);

    // All volumes increase half y to accommodate
    for (const auto& [volume, origTransform] :
         zip(volumes, originalTransforms)) {
      const auto* newBounds =
          dynamic_cast<const CuboidVolumeBounds*>(&volume->volumeBounds());
      BOOST_CHECK_CLOSE(newBounds->get(boundDirOrth1), 700_mm, 1e-6);
      BOOST_CHECK_CLOSE(newBounds->get(boundDirOrth2), 700_mm, 1e-6);
      BOOST_CHECK_CLOSE(newBounds->get(boundDir), halfDir, 1e-6);

      // Position stayed the same
      BOOST_CHECK_EQUAL(volume->transform().matrix(), origTransform.matrix());
    }
  }

  {
    // Increase half length
    auto bounds = std::make_shared<CuboidVolumeBounds>(
        dynamic_cast<const CuboidVolumeBounds&>(stack.volumeBounds()));
    bounds->set(boundDir, 4 * halfDir);
    stack.update(bounds, std::nullopt, *logger);
    const auto* updatedBounds =
        dynamic_cast<const CuboidVolumeBounds*>(&stack.volumeBounds());
    BOOST_REQUIRE(updatedBounds != nullptr);
    BOOST_CHECK_CLOSE(updatedBounds->get(boundDir), 4 * halfDir, 1e-6);
    BOOST_CHECK_CLOSE(updatedBounds->get(boundDirOrth1), 700_mm, 1e-6);
    BOOST_CHECK_CLOSE(updatedBounds->get(boundDirOrth2), 700_mm, 1e-6);

    if (strategy == VolumeResizeStrategy::Expand) {
      // No gap volumes were added
      BOOST_CHECK_EQUAL(volumes.size(), 3);

      // Volume 1 got bigger and shifted left
      auto newBounds1 =
          dynamic_cast<const CuboidVolumeBounds*>(&vol1->volumeBounds());
      BOOST_CHECK_CLOSE(newBounds1->get(boundDir), halfDir + halfDir / 2.0,
                        1e-6);
      auto expectedTranslation1 =
          Translation3(Vector3::Unit(dirIdx) * (-2 * halfDir - halfDir / 2.0));
      Transform3 expectedTransform1 = base * expectedTranslation1;
      CHECK_CLOSE_OR_SMALL(vol1->transform().matrix(),
                           expectedTransform1.matrix(), 1e-10, 1e-12);

      // Volume 2 stayed the same
      auto newBounds2 =
          dynamic_cast<const CuboidVolumeBounds*>(&vol2->volumeBounds());
      BOOST_CHECK_CLOSE(newBounds2->get(boundDir), halfDir, 1e-6);
      CHECK_CLOSE_OR_SMALL(vol2->transform().matrix(), transform2.matrix(),
                           1e-10, 1e-12);

      // Volume 3 got bigger and shifted right
      auto newBounds3 =
          dynamic_cast<const CuboidVolumeBounds*>(&vol3->volumeBounds());
      BOOST_CHECK_CLOSE(newBounds3->get(boundDir), halfDir + halfDir / 2.0,
                        1e-6);
      auto expectedTranslation3 =
          Translation3(Vector3::Unit(dirIdx) * (2 * halfDir + halfDir / 2.0));
      Transform3 expectedTransform3 = base * expectedTranslation3;
      CHECK_CLOSE_OR_SMALL(vol3->transform().matrix(),
                           expectedTransform3.matrix(), 1e-10, 1e-12);
    } else if (strategy == VolumeResizeStrategy::Gap) {
      // Gap volumes were added
      BOOST_CHECK_EQUAL(volumes.size(), 5);

      for (const auto& [volume, origTransform] :
           zip(originalVolumes, originalTransforms)) {
        const auto* newBounds =
            dynamic_cast<const CuboidVolumeBounds*>(&volume->volumeBounds());
        BOOST_CHECK_CLOSE(newBounds->get(boundDirOrth1), 700_mm, 1e-6);
        BOOST_CHECK_CLOSE(newBounds->get(boundDirOrth2), 700_mm, 1e-6);
        BOOST_CHECK_CLOSE(newBounds->get(boundDir), halfDir, 1e-6);
        // Position stayed the same
        CHECK_CLOSE_OR_SMALL(volume->transform().matrix(),
                             origTransform.matrix(), 1e-10, 1e-12);
      }

      auto gap1 = volumes.front();
      auto gap2 = volumes.back();

      const auto* gapBounds1 =
          dynamic_cast<const CuboidVolumeBounds*>(&gap1->volumeBounds());
      const auto* gapBounds2 =
          dynamic_cast<const CuboidVolumeBounds*>(&gap2->volumeBounds());

      BOOST_CHECK_CLOSE(gapBounds1->get(boundDir), halfDir / 2.0, 1e-6);
      BOOST_CHECK_CLOSE(gapBounds2->get(boundDir), halfDir / 2.0, 1e-6);
      auto gap1Translation =
          Translation3(Vector3::Unit(dirIdx) * (-3 * halfDir - halfDir / 2.0));
      Transform3 gap1Transform = base * gap1Translation;

      auto gap2Translation =
          Translation3(Vector3::Unit(dirIdx) * (3 * halfDir + halfDir / 2.0));
      Transform3 gap2Transform = base * gap2Translation;
      CHECK_CLOSE_OR_SMALL(gap1->transform().matrix(), gap1Transform.matrix(),
                           1e-10, 1e-12);
      CHECK_CLOSE_OR_SMALL(gap2->transform().matrix(), gap2Transform.matrix(),
                           1e-10, 1e-12);
    }
  }
}

BOOST_DATA_TEST_CASE(
    UpdateStackOneSided,
    ((boost::unit_test::data::make(-1.0, 1.0) ^
      boost::unit_test::data::make(VolumeResizeStrategy::Gap,
                                   VolumeResizeStrategy::Expand)) *
     boost::unit_test::data::make(Acts::AxisDirection::AxisX,
                                  Acts::AxisDirection::AxisY,
                                  Acts::AxisDirection::AxisZ)),
    f, strategy, dir) {
  auto [dirOrth1, dirOrth2] = CuboidVolumeStack::getOrthogonalAxes(dir);

  auto dirIdx = CuboidVolumeStack::axisToIndex(dir);
  auto dirOrth1Idx = CuboidVolumeStack::axisToIndex(dirOrth1);
  auto dirOrth2Idx = CuboidVolumeStack::axisToIndex(dirOrth2);

  auto boundDir = CuboidVolumeBounds::fromAxisDirection(dir);
  auto boundDirOrth1 = CuboidVolumeBounds::fromAxisDirection(dirOrth1);
  auto boundDirOrth2 = CuboidVolumeBounds::fromAxisDirection(dirOrth2);

  auto bounds1 = std::make_shared<CuboidVolumeBounds>(
      std::initializer_list<std::pair<CuboidVolumeBounds::BoundValues, double>>{
          {boundDir, 400_mm},
          {boundDirOrth1, 100_mm},
          {boundDirOrth2, 300_mm}});

  auto bounds2 = std::make_shared<CuboidVolumeBounds>(
      std::initializer_list<std::pair<CuboidVolumeBounds::BoundValues, double>>{
          {boundDir, 400_mm},
          {boundDirOrth1, 100_mm},
          {boundDirOrth2, 300_mm}});

  auto trf = Transform3::Identity();

  auto translation1 = Translation3(Vector3::Unit(dirIdx) * -500_mm);
  auto trf1 = trf * translation1;
  auto vol1 = std::make_shared<Volume>(trf1, bounds1);

  auto translation2 = Translation3(Vector3::Unit(dirIdx) * 500_mm);
  auto trf2 = trf * translation2;
  auto vol2 = std::make_shared<Volume>(trf2, bounds2);

  std::vector<Volume*> volumes = {vol1.get(), vol2.get()};

  CuboidVolumeStack stack{volumes, dir, VolumeAttachmentStrategy::Gap, strategy,
                          *logger};
  const auto* originalBounds =
      dynamic_cast<const CuboidVolumeBounds*>(&stack.volumeBounds());

  // Increase half length by 50mm
  auto newBounds = std::make_shared<CuboidVolumeBounds>(
      dynamic_cast<const CuboidVolumeBounds&>(stack.volumeBounds()));
  newBounds->set(boundDir, 950_mm);
  // Shift to +stacking direction by 50mm
  auto delta = Translation3(Vector3::Unit(dirIdx) * f * 50_mm);
  trf *= delta;
  // -> left edge should stay at -400mm, right edge should be at 500mm or the
  // other direction
  auto checkUnchanged = [&]() {
    const auto* bounds =
        dynamic_cast<const CuboidVolumeBounds*>(&stack.volumeBounds());
    BOOST_REQUIRE(bounds != nullptr);
    BOOST_CHECK_EQUAL(*bounds, *originalBounds);
  };

  // Invalid: shift too far in merging direction
  BOOST_CHECK_THROW(
      auto errDelta = Translation3(Vector3::Unit(dirIdx) * f * 20_mm);
      stack.update(newBounds, trf * errDelta, *logger), std::invalid_argument);
  checkUnchanged();

  // Invalid: shift in orthogonal direction
  BOOST_CHECK_THROW(
      auto errDelta = Translation3(Vector3::Unit(dirOrth1Idx) * 10_mm);
      stack.update(newBounds, trf * errDelta, *logger), std::invalid_argument);
  checkUnchanged();

  // Invalid: shift in orthogonal direction
  BOOST_CHECK_THROW(
      auto errDelta = Translation3(Vector3::Unit(dirOrth2Idx) * 10_mm);
      stack.update(newBounds, trf * errDelta, *logger), std::invalid_argument);
  checkUnchanged();

  // Invalid: rotation
  BOOST_CHECK_THROW(
      stack.update(newBounds,
                   trf * AngleAxis3{10_degree, Vector3::Unit(dirOrth1Idx)},
                   *logger),
      std::invalid_argument);
  checkUnchanged();

  stack.update(newBounds, trf, *logger);

  CHECK_CLOSE_OR_SMALL(stack.transform().matrix(), trf.matrix(), 1e-10, 1e-12);
  const auto* bounds =
      dynamic_cast<const CuboidVolumeBounds*>(&stack.volumeBounds());
  BOOST_REQUIRE(bounds != nullptr);
  BOOST_CHECK_CLOSE(bounds->get(boundDir), 950_mm, 1e-6);

  // All volumes including gaps should have same size in orthogonal plane
  for (const auto* vol : volumes) {
    const auto* volBounds =
        dynamic_cast<const CuboidVolumeBounds*>(&vol->volumeBounds());
    BOOST_REQUIRE(volBounds != nullptr);
    BOOST_CHECK_CLOSE(volBounds->get(boundDirOrth1), 100_mm, 1e-6);
    BOOST_CHECK_CLOSE(volBounds->get(boundDirOrth2), 300_mm, 1e-6);
  }

  if (strategy == VolumeResizeStrategy::Expand) {
    // No gaps were added, there was one gap initially
    BOOST_CHECK_EQUAL(volumes.size(), 3);
    const Volume* vol = nullptr;
    if (f < 0.0) {
      // first volume should have gotten bigger
      vol = volumes.front();
    } else {
      // last volume should have gotten bigger
      vol = volumes.back();
    }

    const auto* volBounds =
        dynamic_cast<const CuboidVolumeBounds*>(&vol->volumeBounds());
    BOOST_REQUIRE(volBounds != nullptr);
    BOOST_CHECK_CLOSE(volBounds->get(boundDir), 450_mm, 1e-6);
    BOOST_CHECK_EQUAL(vol->center()[dirIdx], f * 550_mm);
  } else if (strategy == VolumeResizeStrategy::Gap) {
    // One gap volume was added
    BOOST_CHECK_EQUAL(volumes.size(), 4);

    const Volume* gap = nullptr;
    if (f < 0.0) {
      gap = volumes.front();
    } else {
      gap = volumes.back();
    }
    const auto* gapBounds =
        dynamic_cast<const CuboidVolumeBounds*>(&gap->volumeBounds());
    BOOST_REQUIRE(gapBounds != nullptr);

    BOOST_CHECK_CLOSE(gapBounds->get(boundDir), 50_mm, 1e-6);
    BOOST_CHECK_EQUAL(gap->center()[dirIdx], f * 950_mm);
  }
}

//   original size
// <--------------->
// +---------------+
// |               |
// |               |
// |   Volume 1    |
// |               |
// |               |
// +---------------+
//         first resize
// <-------------------------->
// +---------------+----------+
// |               |          |
// |               |          |
// |   Volume 1    |   Gap    |
// |               |          |      Gap is
// |               |          |      reused!--+
// +---------------+----------+               |
//             second resize                  |
// <----------------------------------->      |
// +---------------+-------------------+      |
// |               |                   |      |
// |               |                   |      |
// |   Volume 1    |        Gap        |<-----+
// |               |                   |
// |               |                   |
// +---------------+-------------------+
//
BOOST_DATA_TEST_CASE(ResizeGapMultiple,
                     boost::unit_test::data::make(Acts::AxisDirection::AxisX,
                                                  Acts::AxisDirection::AxisY,
                                                  Acts::AxisDirection::AxisZ),
                     dir) {
  auto [dirOrth1, dirOrth2] = CuboidVolumeStack::getOrthogonalAxes(dir);

  auto dirIdx = CuboidVolumeStack::axisToIndex(dir);

  auto boundDir = CuboidVolumeBounds::fromAxisDirection(dir);
  auto boundDirOrth1 = CuboidVolumeBounds::fromAxisDirection(dirOrth1);
  auto boundDirOrth2 = CuboidVolumeBounds::fromAxisDirection(dirOrth2);

  auto bounds = std::make_shared<CuboidVolumeBounds>(
      std::initializer_list<std::pair<CuboidVolumeBounds::BoundValues, double>>{
          {boundDir, 100}, {boundDirOrth1, 70}, {boundDirOrth2, 100}});
  Transform3 trf = Transform3::Identity();
  Volume vol{trf, bounds};

  BOOST_TEST_CONTEXT("Positive") {
    std::vector<Volume*> volumes = {&vol};
    CuboidVolumeStack stack(volumes, dir, VolumeAttachmentStrategy::Gap,
                            VolumeResizeStrategy::Gap, *logger);

    BOOST_CHECK_EQUAL(volumes.size(), 1);
    BOOST_CHECK(stack.gaps().empty());

    auto newBounds1 = std::make_shared<CuboidVolumeBounds>(
        std::initializer_list<
            std::pair<CuboidVolumeBounds::BoundValues, double>>{
            {boundDir, 200}, {boundDirOrth1, 70}, {boundDirOrth2, 100}});
    stack.update(newBounds1, trf * Translation3{Vector3::Unit(dirIdx) * 100},
                 *logger);
    BOOST_CHECK_EQUAL(volumes.size(), 2);
    BOOST_CHECK_EQUAL(stack.gaps().size(), 1);

    BOOST_CHECK_EQUAL(stack.gaps().front()->center()[dirIdx], 200.0);
    const auto* updatedBounds = dynamic_cast<const CuboidVolumeBounds*>(
        &stack.gaps().front()->volumeBounds());
    BOOST_REQUIRE_NE(updatedBounds, nullptr);
    BOOST_CHECK_CLOSE(updatedBounds->get(boundDir), 100.0, 1e-6);

    auto newBounds2 = std::make_shared<CuboidVolumeBounds>(
        std::initializer_list<
            std::pair<CuboidVolumeBounds::BoundValues, double>>{
            {boundDir, 300}, {boundDirOrth1, 70}, {boundDirOrth2, 100}});
    stack.update(newBounds2, trf * Translation3{Vector3::Unit(dirIdx) * 200},
                 *logger);

    BOOST_CHECK_EQUAL(volumes.size(), 2);
    // No additional gap volume was added!
    BOOST_CHECK_EQUAL(stack.gaps().size(), 1);

    BOOST_CHECK_EQUAL(stack.gaps().front()->center()[dirIdx], 300.0);
    updatedBounds = dynamic_cast<const CuboidVolumeBounds*>(
        &stack.gaps().front()->volumeBounds());
    BOOST_REQUIRE_NE(updatedBounds, nullptr);
    BOOST_CHECK_CLOSE(updatedBounds->get(boundDir), 200.0, 1e-6);
  }

  BOOST_TEST_CONTEXT("Negative") {
    std::vector<Volume*> volumes = {&vol};
    CuboidVolumeStack stack(volumes, dir, VolumeAttachmentStrategy::Gap,
                            VolumeResizeStrategy::Gap, *logger);

    BOOST_CHECK_EQUAL(volumes.size(), 1);
    BOOST_CHECK(stack.gaps().empty());

    auto newBounds1 = std::make_shared<CuboidVolumeBounds>(
        std::initializer_list<
            std::pair<CuboidVolumeBounds::BoundValues, double>>{
            {boundDir, 200}, {boundDirOrth1, 70}, {boundDirOrth2, 100}});
    stack.update(newBounds1, trf * Translation3{Vector3::Unit(dirIdx) * -100},
                 *logger);
    BOOST_CHECK_EQUAL(volumes.size(), 2);
    BOOST_CHECK_EQUAL(stack.gaps().size(), 1);

    BOOST_CHECK_EQUAL(stack.gaps().front()->center()[dirIdx], -200.0);
    const auto* updatedBounds = dynamic_cast<const CuboidVolumeBounds*>(
        &stack.gaps().front()->volumeBounds());
    BOOST_REQUIRE_NE(updatedBounds, nullptr);
    BOOST_CHECK_CLOSE(updatedBounds->get(boundDir), 100.0, 1e-6);

    auto newBounds2 = std::make_shared<CuboidVolumeBounds>(
        std::initializer_list<
            std::pair<CuboidVolumeBounds::BoundValues, double>>{
            {boundDir, 300}, {boundDirOrth1, 70}, {boundDirOrth2, 100}});
    stack.update(newBounds2, trf * Translation3{Vector3::Unit(dirIdx) * -200},
                 *logger);

    BOOST_CHECK_EQUAL(volumes.size(), 2);
    // No additional gap volume was added!
    BOOST_CHECK_EQUAL(stack.gaps().size(), 1);

    BOOST_CHECK_EQUAL(stack.gaps().front()->center()[dirIdx], -300.0);
    updatedBounds = dynamic_cast<const CuboidVolumeBounds*>(
        &stack.gaps().front()->volumeBounds());
    BOOST_REQUIRE_NE(updatedBounds, nullptr);
    BOOST_CHECK_CLOSE(updatedBounds->get(boundDir), 200.0, 1e-6);
  }
}

BOOST_DATA_TEST_CASE(InvalidDirection, boost::unit_test::data::make(strategies),
                     strategy) {
  std::vector<Volume*> volumes;
  auto vol1 = std::make_shared<Volume>(
      Transform3::Identity(),
      std::make_shared<CuboidVolumeBounds>(100_mm, 400_mm, 400_mm));
  volumes.push_back(vol1.get());

  // Single volume invalid direction still gives an error
  BOOST_CHECK_THROW(CuboidVolumeStack(volumes, AxisDirection::AxisR, strategy),
                    std::invalid_argument);

  auto vol2 = std::make_shared<Volume>(
      Transform3::Identity(),
      std::make_shared<CuboidVolumeBounds>(100_mm, 400_mm, 400_mm));
  volumes.push_back(vol2.get());

  BOOST_CHECK_THROW(CuboidVolumeStack(volumes, AxisDirection::AxisR, strategy),
                    std::invalid_argument);
}

BOOST_DATA_TEST_CASE(InvalidInput,
                     (boost::unit_test::data::make(strategies) *
                      boost::unit_test::data::make(Acts::AxisDirection::AxisX,
                                                   Acts::AxisDirection::AxisY,
                                                   Acts::AxisDirection::AxisZ)),
                     strategy, direction) {
  BOOST_TEST_CONTEXT("Empty Volume") {
    std::vector<Volume*> volumes;
    BOOST_CHECK_THROW(CuboidVolumeStack(volumes, direction, strategy),
                      std::invalid_argument);
  }

  BOOST_TEST_CONTEXT("Volumes rotated relative to each other") {
    // At this time, all rotations are considered invalid, even around
    // orientation
    for (const Vector3 axis : {Vector3::UnitX(), Vector3::UnitY()}) {
      std::vector<Volume*> volumes;
      auto vol1 = std::make_shared<Volume>(
          Transform3{Translation3{Vector3{0_mm, 0_mm, -500_mm}}},
          std::make_shared<CuboidVolumeBounds>(100_mm, 400_mm, 400_mm));
      volumes.push_back(vol1.get());

      BOOST_TEST_MESSAGE("Axis: " << axis);
      auto vol2 = std::make_shared<Volume>(
          Transform3{Translation3{Vector3{0_mm, 0_mm, 500_mm}} *
                     AngleAxis3(1_degree, axis)},
          std::make_shared<CuboidVolumeBounds>(100_mm, 400_mm, 400_mm));
      volumes.push_back(vol2.get());

      BOOST_CHECK_THROW(CuboidVolumeStack(volumes, direction, strategy,
                                          VolumeResizeStrategy::Gap, *logger),
                        std::invalid_argument);
    }
  }

  BOOST_TEST_CONTEXT(
      "Volumes shifted in the orthogonal plane relative to each other") {
    for (const Vector3& shift :
         {Vector3{5_mm, 0, 0}, Vector3{0, -5_mm, 0}, Vector3{2_mm, -2_mm, 0}}) {
      std::vector<Volume*> volumes;
      auto vol1 = std::make_shared<Volume>(
          Transform3{Translation3{Vector3{0_mm, 0_mm, -500_mm}}},
          std::make_shared<CuboidVolumeBounds>(100_mm, 400_mm, 400_mm));
      volumes.push_back(vol1.get());

      auto vol2 = std::make_shared<Volume>(
          Transform3{Translation3{Vector3{0_mm, 0_mm, 500_mm} + shift}},
          std::make_shared<CuboidVolumeBounds>(100_mm, 400_mm, 400_mm));
      volumes.push_back(vol2.get());

      BOOST_CHECK_THROW(CuboidVolumeStack(volumes, direction, strategy,
                                          VolumeResizeStrategy::Gap, *logger),
                        std::invalid_argument);
    }
  }
}

BOOST_DATA_TEST_CASE(JoinCuboidVolumeSingle,
                     (boost::unit_test::data::make(Acts::AxisDirection::AxisX,
                                                   Acts::AxisDirection::AxisY,
                                                   Acts::AxisDirection::AxisZ) *
                      boost::unit_test::data::make(strategies)),
                     direction, strategy) {
  auto vol = std::make_shared<Volume>(
      Transform3::Identity() * Translation3{14_mm, 24_mm, 0_mm} *
          AngleAxis3(73_degree, Vector3::UnitX()),
      std::make_shared<CuboidVolumeBounds>(100_mm, 400_mm, 400_mm));

  std::vector<Volume*> volumes{vol.get()};

  CuboidVolumeStack stack(volumes, direction, strategy,
                          VolumeResizeStrategy::Gap, *logger);

  // Cuboid stack has the same transform as bounds as the single input
  // volume
  BOOST_CHECK_EQUAL(volumes.size(), 1);
  BOOST_CHECK_EQUAL(volumes.at(0), vol.get());
  BOOST_CHECK_EQUAL(vol->transform().matrix(), stack.transform().matrix());
  BOOST_CHECK_EQUAL(vol->volumeBounds(), stack.volumeBounds());
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
