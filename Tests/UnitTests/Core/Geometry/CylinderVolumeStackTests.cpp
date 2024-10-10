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
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/CylinderVolumeStack.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/Zip.hpp"

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

static const std::vector<CylinderVolumeStack::AttachmentStrategy> strategies = {
    CylinderVolumeStack::AttachmentStrategy::Gap,
    CylinderVolumeStack::AttachmentStrategy::First,
    CylinderVolumeStack::AttachmentStrategy::Second,
    CylinderVolumeStack::AttachmentStrategy::Midpoint,
};

static const std::vector<CylinderVolumeStack::ResizeStrategy> resizeStrategies =
    {
        CylinderVolumeStack::ResizeStrategy::Expand,
        CylinderVolumeStack::ResizeStrategy::Gap,
};

BOOST_AUTO_TEST_SUITE(CylinderVolumeStackTest)
BOOST_AUTO_TEST_SUITE(ZDirection)

BOOST_DATA_TEST_CASE(Baseline,
                     (boost::unit_test::data::xrange(-135, 180, 45) *
                      boost::unit_test::data::xrange(0, 2, 1) *
                      boost::unit_test::data::make(0.8, 1.0, 1.2) *
                      boost::unit_test::data::make(Vector3{0_mm, 0_mm, 0_mm},
                                                   Vector3{20_mm, 0_mm, 0_mm},
                                                   Vector3{0_mm, 20_mm, 0_mm},
                                                   Vector3{20_mm, 20_mm, 0_mm},
                                                   Vector3{0_mm, 0_mm, 20_mm}) *
                      boost::unit_test::data::make(strategies)),
                     angle, rotate, shift, offset, strategy) {
  ActsScalar hlZ = 400_mm;

  // Cylinder volumes which already line up, but have different1 radii
  auto bounds1 = std::make_shared<CylinderVolumeBounds>(100_mm, 400_mm, hlZ);
  auto bounds2 = std::make_shared<CylinderVolumeBounds>(200_mm, 600_mm, hlZ);
  auto bounds3 = std::make_shared<CylinderVolumeBounds>(300_mm, 500_mm, hlZ);

  Transform3 base =
      AngleAxis3(angle * 1_degree, Vector3::UnitX()) * Translation3(offset);

  Transform3 transform1 = base;
  transform1.translate(Vector3{0_mm, 0_mm, -2 * hlZ * shift});
  auto vol1 = std::make_shared<Volume>(transform1, bounds1);

  Transform3 transform2 = base;
  transform2.translate(Vector3{0_mm, 0_mm, 0_mm});
  auto vol2 = std::make_shared<Volume>(transform2, bounds2);

  Transform3 transform3 = base;
  transform3.translate(Vector3{0_mm, 0_mm, 2 * hlZ * shift});
  auto vol3 = std::make_shared<Volume>(transform3, bounds3);

  std::vector<Volume*> volumes = {vol1.get(), vol2.get(), vol3.get()};
  // Rotate to simulate unsorted volumes: all results should be the same!
  std::rotate(volumes.begin(), volumes.begin() + rotate, volumes.end());

  auto origVolumes = volumes;

  std::vector<CylinderVolumeBounds> originalBounds;
  std::transform(
      volumes.begin(), volumes.end(), std::back_inserter(originalBounds),
      [](const auto& vol) {
        return *dynamic_cast<const CylinderVolumeBounds*>(&vol->volumeBounds());
      });

  if (shift < 1.0) {
    BOOST_CHECK_THROW(
        CylinderVolumeStack(volumes, BinningValue::binZ, strategy,
                            CylinderVolumeStack::ResizeStrategy::Gap, *logger),
        std::invalid_argument);
    return;
  }
  CylinderVolumeStack cylStack(volumes, BinningValue::binZ, strategy,
                               CylinderVolumeStack::ResizeStrategy::Gap,
                               *logger);

  auto stackBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&cylStack.volumeBounds());
  BOOST_REQUIRE(stackBounds != nullptr);

  BOOST_CHECK_EQUAL(stackBounds->get(CylinderVolumeBounds::eMinR), 100_mm);
  BOOST_CHECK_EQUAL(stackBounds->get(CylinderVolumeBounds::eMaxR), 600_mm);
  BOOST_CHECK_EQUAL(stackBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                    hlZ + 2 * hlZ * shift);
  CHECK_CLOSE_OR_SMALL(cylStack.transform().matrix(), base.matrix(), 1e-10,
                       1e-14);

  // All volumes (including gaps) are cylinders and have the same radial bounds
  for (const auto& volume : volumes) {
    const auto* cylinderBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&volume->volumeBounds());
    BOOST_REQUIRE(cylinderBounds != nullptr);
    BOOST_CHECK_EQUAL(cylinderBounds->get(CylinderVolumeBounds::eMinR), 100_mm);
    BOOST_CHECK_EQUAL(cylinderBounds->get(CylinderVolumeBounds::eMaxR), 600_mm);
  }

  // Volumes are sorted in (local) z
  for (std::size_t i = 0; i < volumes.size() - 1; ++i) {
    const auto& a = volumes.at(i);
    const auto& b = volumes.at(i + 1);

    BOOST_CHECK_LT((base.inverse() * a->center())[eZ],
                   (base.inverse() * b->center())[eZ]);
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
          dynamic_cast<const CylinderVolumeBounds*>(&volume->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                        bounds.get(CylinderVolumeBounds::eHalfLengthZ));
    }
  } else {
    if (strategy == CylinderVolumeStack::AttachmentStrategy::Gap) {
      // Gap volumes were added
      BOOST_CHECK_EQUAL(volumes.size(), 5);
      auto gap1 = volumes.at(1);
      auto gap2 = volumes.at(3);

      BOOST_TEST_MESSAGE("Gap 1: " << gap1->transform().matrix());
      BOOST_TEST_MESSAGE("Gap 2: " << gap2->transform().matrix());

      const auto* gapBounds1 =
          dynamic_cast<const CylinderVolumeBounds*>(&gap1->volumeBounds());
      const auto* gapBounds2 =
          dynamic_cast<const CylinderVolumeBounds*>(&gap2->volumeBounds());

      ActsScalar gapHlZ = (shift - 1.0) * hlZ;

      BOOST_CHECK(std::abs(gapBounds1->get(CylinderVolumeBounds::eHalfLengthZ) -
                           gapHlZ) < 1e-10);
      BOOST_CHECK(std::abs(gapBounds2->get(CylinderVolumeBounds::eHalfLengthZ) -
                           gapHlZ) < 1e-10);

      ActsScalar gap1Z = (-2 * hlZ * shift) + hlZ + gapHlZ;
      ActsScalar gap2Z = (2 * hlZ * shift) - hlZ - gapHlZ;

      Transform3 gap1Transform = base * Translation3{0_mm, 0_mm, gap1Z};
      Transform3 gap2Transform = base * Translation3{0_mm, 0_mm, gap2Z};

      CHECK_CLOSE_OR_SMALL(gap1->transform().matrix(), gap1Transform.matrix(),
                           1e-10, 1e-14);
      CHECK_CLOSE_OR_SMALL(gap2->transform().matrix(), gap2Transform.matrix(),
                           1e-10, 1e-14);

      // Original volumes did not changes bounds
      for (const auto& [volume, bounds] : zip(origVolumes, originalBounds)) {
        const auto* newBounds =
            dynamic_cast<const CylinderVolumeBounds*>(&volume->volumeBounds());
        BOOST_CHECK_EQUAL(newBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                          bounds.get(CylinderVolumeBounds::eHalfLengthZ));
      }

      // No expansion, original volumes did not move
      BOOST_CHECK_EQUAL(vol1->transform().matrix(), transform1.matrix());
      BOOST_CHECK_EQUAL(vol2->transform().matrix(), transform2.matrix());
      BOOST_CHECK_EQUAL(vol3->transform().matrix(), transform3.matrix());

    } else if (strategy == CylinderVolumeStack::AttachmentStrategy::First) {
      // No gap volumes were added
      BOOST_CHECK_EQUAL(volumes.size(), 3);

      ActsScalar wGap = (shift - 1.0) * hlZ * 2;

      // Volume 1 got bigger and shifted right
      auto newBounds1 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol1->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds1->get(CylinderVolumeBounds::eHalfLengthZ),
                        hlZ + wGap / 2.0);
      ActsScalar pZ1 = -2 * hlZ * shift + wGap / 2.0;
      Transform3 expectedTransform1 = base * Translation3{0_mm, 0_mm, pZ1};
      CHECK_CLOSE_OR_SMALL(vol1->transform().matrix(),
                           expectedTransform1.matrix(), 1e-10, 1e-14);

      // Volume 2 got bigger and shifted left
      auto newBounds2 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol2->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds2->get(CylinderVolumeBounds::eHalfLengthZ),
                        hlZ + wGap / 2.0);
      ActsScalar pZ2 = wGap / 2.0;
      Transform3 expectedTransform2 = base * Translation3{0_mm, 0_mm, pZ2};
      CHECK_CLOSE_OR_SMALL(vol2->transform().matrix(),
                           expectedTransform2.matrix(), 1e-10, 1e-14);

      // Volume 3 stayed the same
      auto newBounds3 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol3->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds3->get(CylinderVolumeBounds::eHalfLengthZ),
                        hlZ);
      ActsScalar pZ3 = 2 * hlZ * shift;
      Transform3 expectedTransform3 = base * Translation3{0_mm, 0_mm, pZ3};
      CHECK_CLOSE_OR_SMALL(vol3->transform().matrix(),
                           expectedTransform3.matrix(), 1e-10, 1e-14);
    } else if (strategy == CylinderVolumeStack::AttachmentStrategy::Second) {
      // No gap volumes were added
      BOOST_CHECK_EQUAL(volumes.size(), 3);

      ActsScalar wGap = (shift - 1.0) * hlZ * 2;

      // Volume 1 stayed the same
      auto newBounds1 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol1->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds1->get(CylinderVolumeBounds::eHalfLengthZ),
                        hlZ);
      ActsScalar pZ1 = -2 * hlZ * shift;
      Transform3 expectedTransform1 = base * Translation3{0_mm, 0_mm, pZ1};
      CHECK_CLOSE_OR_SMALL(vol1->transform().matrix(),
                           expectedTransform1.matrix(), 1e-10, 1e-14);

      // Volume 2 got bigger and shifted left
      auto newBounds2 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol2->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds2->get(CylinderVolumeBounds::eHalfLengthZ),
                        hlZ + wGap / 2.0);
      ActsScalar pZ2 = -wGap / 2.0;
      Transform3 expectedTransform2 = base * Translation3{0_mm, 0_mm, pZ2};
      CHECK_CLOSE_OR_SMALL(vol2->transform().matrix(),
                           expectedTransform2.matrix(), 1e-10, 1e-14);

      // Volume 3 got bigger and shifted left
      auto newBounds3 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol3->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds3->get(CylinderVolumeBounds::eHalfLengthZ),
                        hlZ + wGap / 2.0);
      ActsScalar pZ3 = 2 * hlZ * shift - wGap / 2.0;
      Transform3 expectedTransform3 = base * Translation3{0_mm, 0_mm, pZ3};
      CHECK_CLOSE_OR_SMALL(vol3->transform().matrix(),
                           expectedTransform3.matrix(), 1e-10, 1e-14);
    } else if (strategy == CylinderVolumeStack::AttachmentStrategy::Midpoint) {
      // No gap volumes were added
      BOOST_CHECK_EQUAL(volumes.size(), 3);

      ActsScalar wGap = (shift - 1.0) * hlZ * 2;

      // Volume 1 got bigger and shifted right
      auto newBounds1 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol1->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds1->get(CylinderVolumeBounds::eHalfLengthZ),
                        hlZ + wGap / 4.0);
      ActsScalar pZ1 = -2 * hlZ * shift + wGap / 4.0;
      Transform3 expectedTransform1 = base * Translation3{0_mm, 0_mm, pZ1};
      CHECK_CLOSE_OR_SMALL(vol1->transform().matrix(),
                           expectedTransform1.matrix(), 1e-10, 1e-14);

      // Volume 2 got bigger but didn't move
      auto newBounds2 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol2->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds2->get(CylinderVolumeBounds::eHalfLengthZ),
                        hlZ + wGap / 2.0);
      CHECK_CLOSE_OR_SMALL(vol2->transform().matrix(), base.matrix(), 1e-10,
                           1e-14);

      // Volume 3 got bigger and shifted left
      auto newBounds3 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol3->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds3->get(CylinderVolumeBounds::eHalfLengthZ),
                        hlZ + wGap / 4.0);
      ActsScalar pZ3 = 2 * hlZ * shift - wGap / 4.0;
      Transform3 expectedTransform3 = base * Translation3{0_mm, 0_mm, pZ3};
      CHECK_CLOSE_OR_SMALL(vol3->transform().matrix(),
                           expectedTransform3.matrix(), 1e-10, 1e-14);
    }
  }
}

BOOST_AUTO_TEST_CASE(Asymmetric) {
  ActsScalar hlZ1 = 200_mm;
  ActsScalar pZ1 = -1100_mm;
  ActsScalar hlZ2 = 600_mm;
  ActsScalar pZ2 = -200_mm;
  ActsScalar hlZ3 = 400_mm;
  ActsScalar pZ3 = 850_mm;

  // Cylinder volumes which already line up, but have different1 radii
  auto bounds1 = std::make_shared<CylinderVolumeBounds>(100_mm, 400_mm, hlZ1);
  auto bounds2 = std::make_shared<CylinderVolumeBounds>(200_mm, 600_mm, hlZ2);
  auto bounds3 = std::make_shared<CylinderVolumeBounds>(300_mm, 500_mm, hlZ3);

  Transform3 transform1 = Transform3::Identity();
  transform1.translate(Vector3{0_mm, 0_mm, pZ1});
  auto vol1 = std::make_shared<Volume>(transform1, bounds1);

  Transform3 transform2 = Transform3::Identity();
  transform2.translate(Vector3{0_mm, 0_mm, pZ2});
  auto vol2 = std::make_shared<Volume>(transform2, bounds2);

  Transform3 transform3 = Transform3::Identity();
  transform3.translate(Vector3{0_mm, 0_mm, pZ3});
  auto vol3 = std::make_shared<Volume>(transform3, bounds3);

  std::vector<Volume*> volumes = {vol2.get(), vol1.get(), vol3.get()};

  CylinderVolumeStack cylStack(
      volumes, BinningValue::binZ, CylinderVolumeStack::AttachmentStrategy::Gap,
      CylinderVolumeStack::ResizeStrategy::Gap, *logger);
  BOOST_CHECK_EQUAL(volumes.size(), 5);

  auto stackBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&cylStack.volumeBounds());
  BOOST_REQUIRE(stackBounds != nullptr);

  BOOST_CHECK_EQUAL(stackBounds->get(CylinderVolumeBounds::eMinR), 100_mm);
  BOOST_CHECK_EQUAL(stackBounds->get(CylinderVolumeBounds::eMaxR), 600_mm);
  BOOST_CHECK_EQUAL(stackBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                    (std::abs(pZ1 - hlZ1) + pZ3 + hlZ3) / 2.0);

  ActsScalar midZ = (pZ1 - hlZ1 + pZ3 + hlZ3) / 2.0;
  Transform3 expectedTransform{Translation3{0_mm, 0_mm, midZ}};
  CHECK_CLOSE_OR_SMALL(cylStack.transform().matrix(),
                       expectedTransform.matrix(), 1e-10, 1e-14);
}

BOOST_DATA_TEST_CASE(RotationInZ, boost::unit_test::data::make(strategies),
                     strategy) {
  ActsScalar hlZ = 400_mm;
  ActsScalar gap = 100_mm;
  ActsScalar shift = 300_mm;

  auto bounds1 = std::make_shared<CylinderVolumeBounds>(100_mm, 400_mm, hlZ);
  auto bounds2 = std::make_shared<CylinderVolumeBounds>(200_mm, 300_mm, hlZ);

  auto vol1 = std::make_shared<Volume>(
      Transform3::Identity() *
          Translation3{0_mm, 0_mm, -hlZ - gap / 2.0 + shift},
      bounds1);

  auto vol2 = std::make_shared<Volume>(
      Transform3::Identity() *
          Translation3{0_mm, 0_mm, hlZ + gap / 2.0 + shift} *
          AngleAxis3{30_degree, Vector3::UnitZ()},
      bounds2);

  std::vector<Volume*> volumes = {vol1.get(), vol2.get()};

  CylinderVolumeStack cylStack(volumes, BinningValue::binZ, strategy,
                               CylinderVolumeStack::ResizeStrategy::Gap,
                               *logger);

  auto stackBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&cylStack.volumeBounds());
  BOOST_REQUIRE(stackBounds != nullptr);
  BOOST_CHECK_EQUAL(stackBounds->get(CylinderVolumeBounds::eMinR), 100_mm);
  BOOST_CHECK_EQUAL(stackBounds->get(CylinderVolumeBounds::eMaxR), 400_mm);
  BOOST_CHECK_EQUAL(stackBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                    2 * hlZ + gap / 2.0);

  auto newBounds1 =
      dynamic_cast<const CylinderVolumeBounds*>(&vol1->volumeBounds());
  auto newBounds2 =
      dynamic_cast<const CylinderVolumeBounds*>(&vol2->volumeBounds());

  for (const auto& bounds : {newBounds1, newBounds2}) {
    BOOST_CHECK_EQUAL(bounds->get(CylinderVolumeBounds::eMinR), 100_mm);
    BOOST_CHECK_EQUAL(bounds->get(CylinderVolumeBounds::eMaxR), 400_mm);
  }

  if (strategy == CylinderVolumeStack::AttachmentStrategy::Gap) {
    // Volumes stayed at the same position, not resized
    BOOST_CHECK_EQUAL(vol1->center()[eZ], -hlZ - gap / 2.0 + shift);
    BOOST_CHECK_EQUAL(vol2->center()[eZ], hlZ + gap / 2.0 + shift);
    BOOST_CHECK_EQUAL(newBounds1->get(CylinderVolumeBounds::eHalfLengthZ), hlZ);
    BOOST_CHECK_EQUAL(newBounds2->get(CylinderVolumeBounds::eHalfLengthZ), hlZ);
  } else if (strategy == CylinderVolumeStack::AttachmentStrategy::First) {
    // Left volume moved, got resized
    BOOST_CHECK_EQUAL(vol1->center()[eZ], -hlZ + shift);
    BOOST_CHECK_EQUAL(newBounds1->get(CylinderVolumeBounds::eHalfLengthZ),
                      hlZ + gap / 2.0);
    // Right volume stayed the same
    BOOST_CHECK_EQUAL(vol2->center()[eZ], hlZ + gap / 2.0 + shift);
    BOOST_CHECK_EQUAL(newBounds2->get(CylinderVolumeBounds::eHalfLengthZ), hlZ);
  } else if (strategy == CylinderVolumeStack::AttachmentStrategy::Second) {
    // Left volume stayed the same
    BOOST_CHECK_EQUAL(vol1->center()[eZ], -hlZ - gap / 2.0 + shift);
    BOOST_CHECK_EQUAL(newBounds1->get(CylinderVolumeBounds::eHalfLengthZ), hlZ);
    // Right volume moved, got resized
    BOOST_CHECK_EQUAL(vol2->center()[eZ], hlZ + shift);
    BOOST_CHECK_EQUAL(newBounds2->get(CylinderVolumeBounds::eHalfLengthZ),
                      hlZ + gap / 2.0);
  } else if (strategy == CylinderVolumeStack::AttachmentStrategy::Midpoint) {
    // Left volume moved, got resized
    BOOST_CHECK_EQUAL(vol1->center()[eZ], -hlZ - gap / 4.0 + shift);
    BOOST_CHECK_EQUAL(newBounds1->get(CylinderVolumeBounds::eHalfLengthZ),
                      hlZ + gap / 4.0);

    // Right volume moved, got resized
    BOOST_CHECK_EQUAL(vol2->center()[eZ], hlZ + gap / 4.0 + shift);
    BOOST_CHECK_EQUAL(newBounds2->get(CylinderVolumeBounds::eHalfLengthZ),
                      hlZ + gap / 4.0);
  }
}

BOOST_DATA_TEST_CASE(UpdateStack,
                     (boost::unit_test::data::xrange(-135, 180, 45) *
                      boost::unit_test::data::make(Vector3{0_mm, 0_mm, 0_mm},
                                                   Vector3{20_mm, 0_mm, 0_mm},
                                                   Vector3{0_mm, 20_mm, 0_mm},
                                                   Vector3{20_mm, 20_mm, 0_mm},
                                                   Vector3{0_mm, 0_mm, 20_mm}) *
                      boost::unit_test::data::make(-100_mm, 0_mm, 100_mm) *
                      boost::unit_test::data::make(resizeStrategies)),
                     angle, offset, zshift, strategy) {
  ActsScalar hlZ = 400_mm;

  // Cylinder volumes which already line up, but have different1 radii
  auto bounds1 = std::make_shared<CylinderVolumeBounds>(100_mm, 600_mm, hlZ);
  auto bounds2 = std::make_shared<CylinderVolumeBounds>(100_mm, 600_mm, hlZ);
  auto bounds3 = std::make_shared<CylinderVolumeBounds>(100_mm, 600_mm, hlZ);

  Transform3 base = AngleAxis3(angle * 1_degree, Vector3::UnitX()) *
                    Translation3(offset + Vector3{0_mm, 0_mm, zshift});

  Transform3 transform1 = base;
  transform1.translate(Vector3{0_mm, 0_mm, -2 * hlZ});
  auto vol1 = std::make_shared<Volume>(transform1, bounds1);

  Transform3 transform2 = base;
  transform2.translate(Vector3{0_mm, 0_mm, 0_mm});
  auto vol2 = std::make_shared<Volume>(transform2, bounds2);

  Transform3 transform3 = base;
  transform3.translate(Vector3{0_mm, 0_mm, 2 * hlZ});
  auto vol3 = std::make_shared<Volume>(transform3, bounds3);

  std::vector<Volume*> volumes = {vol1.get(), vol2.get(), vol3.get()};
  std::vector<Volume*> originalVolumes = volumes;

  std::vector<Transform3> originalTransforms = {transform1, transform2,
                                                transform3};

  CylinderVolumeStack cylStack(
      volumes, BinningValue::binZ,
      CylinderVolumeStack::AttachmentStrategy::Gap,  // should not make a
                                                     // difference
      strategy, *logger);

  const auto* originalBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&cylStack.volumeBounds());

  auto assertOriginalBounds = [&]() {
    const auto* cylBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&cylStack.volumeBounds());
    BOOST_REQUIRE(cylBounds != nullptr);
    BOOST_CHECK_EQUAL(cylBounds, originalBounds);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMinR), 100_mm);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMaxR), 600_mm);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                      3 * hlZ);
  };

  assertOriginalBounds();

  {
    // Assign a copy of the identical bounds gives identical bounds
    auto bounds = std::make_shared<CylinderVolumeBounds>(
        dynamic_cast<const CylinderVolumeBounds&>(cylStack.volumeBounds()));
    cylStack.update(bounds, std::nullopt, *logger);
    assertOriginalBounds();
  }

  {
    // Cannot increase mininmum r
    auto bounds = std::make_shared<CylinderVolumeBounds>(
        dynamic_cast<const CylinderVolumeBounds&>(cylStack.volumeBounds()));
    bounds->set(CylinderVolumeBounds::eMinR, 200_mm);
    BOOST_CHECK_THROW(cylStack.update(bounds, std::nullopt, *logger),
                      std::invalid_argument);
    assertOriginalBounds();
  }

  {
    // Cannot decrease maximum r
    auto bounds = std::make_shared<CylinderVolumeBounds>(
        dynamic_cast<const CylinderVolumeBounds&>(cylStack.volumeBounds()));
    bounds->set(CylinderVolumeBounds::eMaxR, 500_mm);
    BOOST_CHECK_THROW(cylStack.update(bounds, std::nullopt, *logger),
                      std::invalid_argument);
    assertOriginalBounds();
  }

  {
    // Cannot decrease half length z
    auto bounds = std::make_shared<CylinderVolumeBounds>(
        dynamic_cast<const CylinderVolumeBounds&>(cylStack.volumeBounds()));
    bounds->set(CylinderVolumeBounds::eHalfLengthZ, 2 * hlZ);
    BOOST_CHECK_THROW(cylStack.update(bounds, std::nullopt, *logger),
                      std::invalid_argument);
    assertOriginalBounds();
  }

  {
    // Reduce minimum r
    auto bounds = std::make_shared<CylinderVolumeBounds>(
        dynamic_cast<const CylinderVolumeBounds&>(cylStack.volumeBounds()));
    bounds->set(CylinderVolumeBounds::eMinR, 50_mm);
    cylStack.update(bounds, std::nullopt, *logger);
    const auto* cylBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&cylStack.volumeBounds());
    BOOST_REQUIRE(cylBounds != nullptr);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMinR), 50_mm);
    // Rest unchanged
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMaxR), 600_mm);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                      3 * hlZ);

    // No gap volumes were added
    BOOST_CHECK_EQUAL(volumes.size(), 3);

    // All volumes reduces min r to accommodate
    for (const auto& [volume, origTransform] :
         zip(volumes, originalTransforms)) {
      const auto* newBounds =
          dynamic_cast<const CylinderVolumeBounds*>(&volume->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds->get(CylinderVolumeBounds::eMinR), 50_mm);
      BOOST_CHECK_EQUAL(newBounds->get(CylinderVolumeBounds::eMaxR), 600_mm);
      BOOST_CHECK_EQUAL(newBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                        hlZ);

      // Position stayed the same
      BOOST_CHECK_EQUAL(volume->transform().matrix(), origTransform.matrix());
    }
  }

  {
    // Increase maximum r
    auto bounds = std::make_shared<CylinderVolumeBounds>(
        dynamic_cast<const CylinderVolumeBounds&>(cylStack.volumeBounds()));
    bounds->set(CylinderVolumeBounds::eMaxR, 700_mm);
    cylStack.update(bounds, std::nullopt, *logger);
    const auto* cylBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&cylStack.volumeBounds());
    BOOST_REQUIRE(cylBounds != nullptr);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMaxR), 700_mm);
    // Rest as before
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMinR), 50_mm);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                      3 * hlZ);

    // No gap volumes were added
    BOOST_CHECK_EQUAL(volumes.size(), 3);

    // All volumes reduces min r to accommodate
    for (const auto& [volume, origTransform] :
         zip(volumes, originalTransforms)) {
      const auto* newBounds =
          dynamic_cast<const CylinderVolumeBounds*>(&volume->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds->get(CylinderVolumeBounds::eMinR), 50_mm);
      BOOST_CHECK_EQUAL(newBounds->get(CylinderVolumeBounds::eMaxR), 700_mm);
      BOOST_CHECK_EQUAL(newBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                        hlZ);

      // Position stayed the same
      BOOST_CHECK_EQUAL(volume->transform().matrix(), origTransform.matrix());
    }
  }

  {
    // Increase half length z
    auto bounds = std::make_shared<CylinderVolumeBounds>(
        dynamic_cast<const CylinderVolumeBounds&>(cylStack.volumeBounds()));
    bounds->set(CylinderVolumeBounds::eHalfLengthZ, 4 * hlZ);
    cylStack.update(bounds, std::nullopt, *logger);
    const auto* cylBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&cylStack.volumeBounds());
    BOOST_REQUIRE(cylBounds != nullptr);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                      4 * hlZ);

    // Rest as before
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMinR), 50_mm);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMaxR), 700_mm);

    if (strategy == CylinderVolumeStack::ResizeStrategy::Expand) {
      // No gap volumes were added
      BOOST_CHECK_EQUAL(volumes.size(), 3);

      // Volume 1 got bigger and shifted left
      auto newBounds1 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol1->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds1->get(CylinderVolumeBounds::eHalfLengthZ),
                        hlZ + hlZ / 2.0);
      Transform3 expectedTransform1 =
          base * Translation3{0_mm, 0_mm, -2 * hlZ - hlZ / 2.0};
      BOOST_CHECK_EQUAL(vol1->transform().matrix(),
                        expectedTransform1.matrix());

      // Volume 2 stayed the same
      auto newBounds2 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol2->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds2->get(CylinderVolumeBounds::eHalfLengthZ),
                        hlZ);
      BOOST_CHECK_EQUAL(vol2->transform().matrix(), transform2.matrix());

      // Volume 3 got bigger and shifted right
      auto newBounds3 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol3->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds3->get(CylinderVolumeBounds::eHalfLengthZ),
                        hlZ + hlZ / 2.0);
      Transform3 expectedTransform3 =
          base * Translation3{0_mm, 0_mm, 2 * hlZ + hlZ / 2.0};
      BOOST_CHECK_EQUAL(vol3->transform().matrix(),
                        expectedTransform3.matrix());
    } else if (strategy == CylinderVolumeStack::ResizeStrategy::Gap) {
      // Gap volumes were added
      BOOST_CHECK_EQUAL(volumes.size(), 5);

      for (const auto& [volume, origTransform] :
           zip(originalVolumes, originalTransforms)) {
        const auto* newBounds =
            dynamic_cast<const CylinderVolumeBounds*>(&volume->volumeBounds());
        BOOST_CHECK_EQUAL(newBounds->get(CylinderVolumeBounds::eMinR), 50_mm);
        BOOST_CHECK_EQUAL(newBounds->get(CylinderVolumeBounds::eMaxR), 700_mm);
        BOOST_CHECK_EQUAL(newBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                          hlZ);
        // Position stayed the same
        BOOST_CHECK_EQUAL(volume->transform().matrix(), origTransform.matrix());
      }

      auto gap1 = volumes.front();
      auto gap2 = volumes.back();

      const auto* gapBounds1 =
          dynamic_cast<const CylinderVolumeBounds*>(&gap1->volumeBounds());
      const auto* gapBounds2 =
          dynamic_cast<const CylinderVolumeBounds*>(&gap2->volumeBounds());

      BOOST_CHECK_EQUAL(gapBounds1->get(CylinderVolumeBounds::eHalfLengthZ),
                        hlZ / 2.0);
      BOOST_CHECK_EQUAL(gapBounds2->get(CylinderVolumeBounds::eHalfLengthZ),
                        hlZ / 2.0);

      Transform3 gap1Transform =
          base * Translation3{0_mm, 0_mm, -3 * hlZ - hlZ / 2.0};
      Transform3 gap2Transform =
          base * Translation3{0_mm, 0_mm, 3 * hlZ + hlZ / 2.0};

      CHECK_CLOSE_OR_SMALL(gap1->transform().matrix(), gap1Transform.matrix(),
                           1e-10, 1e-14);
      CHECK_CLOSE_OR_SMALL(gap2->transform().matrix(), gap2Transform.matrix(),
                           1e-10, 1e-14);
    }
  }
}

BOOST_DATA_TEST_CASE(
    UpdateStackOneSided,
    (boost::unit_test::data::make(-1.0, 1.0) ^
     boost::unit_test::data::make(CylinderVolumeStack::ResizeStrategy::Gap,
                                  CylinderVolumeStack::ResizeStrategy::Expand)),
    f, strategy) {
  auto trf = Transform3::Identity();

  auto trf1 = trf * Translation3{Vector3{0_mm, 0_mm, -500_mm}};
  auto vol1 = std::make_shared<Volume>(
      trf1, std::make_shared<CylinderVolumeBounds>(100_mm, 300_mm, 400_mm));

  auto trf2 = trf * Translation3{Vector3{0_mm, 0_mm, 500_mm}};
  auto vol2 = std::make_shared<Volume>(
      trf2, std::make_shared<CylinderVolumeBounds>(100_mm, 300_mm, 400_mm));

  std::vector<Volume*> volumes = {vol1.get(), vol2.get()};

  CylinderVolumeStack cylStack{volumes, BinningValue::binZ,
                               CylinderVolumeStack::AttachmentStrategy::Gap,
                               strategy, *logger};
  const auto* originalBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&cylStack.volumeBounds());

  // Increase halflength by 50mm
  auto newBounds = std::make_shared<CylinderVolumeBounds>(
      dynamic_cast<const CylinderVolumeBounds&>(cylStack.volumeBounds()));
  newBounds->set(CylinderVolumeBounds::eHalfLengthZ, 950_mm);
  // Shift to +z by 50mm
  trf *= Translation3{Vector3{0_mm, 0_mm, f * 50_mm}};
  // -> left edge should stay at -400mm, right edge should be at 500mm or the
  // other direction

  auto checkUnchanged = [&]() {
    const auto* cylBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&cylStack.volumeBounds());
    BOOST_REQUIRE(cylBounds != nullptr);
    BOOST_CHECK_EQUAL(*cylBounds, *originalBounds);
  };

  // Invalid: shift too far in z
  BOOST_CHECK_THROW(
      cylStack.update(newBounds, trf * Translation3{Vector3{0, 0, f * 20_mm}},
                      *logger),
      std::invalid_argument);
  checkUnchanged();

  // Invalid: shift in x
  BOOST_CHECK_THROW(
      cylStack.update(newBounds, trf * Translation3{Vector3{10_mm, 0, 0}},
                      *logger),
      std::invalid_argument);
  checkUnchanged();

  // Invalid: shift in y
  BOOST_CHECK_THROW(
      cylStack.update(newBounds, trf * Translation3{Vector3{0, 10_mm, 0}},
                      *logger),
      std::invalid_argument);
  checkUnchanged();

  // Invalid: rotation
  BOOST_CHECK_THROW(
      cylStack.update(newBounds, trf * AngleAxis3{10_degree, Vector3::UnitY()},
                      *logger),
      std::invalid_argument);
  checkUnchanged();

  cylStack.update(newBounds, trf, *logger);

  BOOST_CHECK_EQUAL(cylStack.transform().matrix(), trf.matrix());
  const auto* cylBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&cylStack.volumeBounds());
  BOOST_REQUIRE(cylBounds != nullptr);
  BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eHalfLengthZ), 950_mm);

  // All volumes including gaps should have same r size
  for (const auto* vol : volumes) {
    const auto* volBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&vol->volumeBounds());
    BOOST_REQUIRE(volBounds != nullptr);
    BOOST_CHECK_EQUAL(volBounds->get(CylinderVolumeBounds::eMinR), 100_mm);
    BOOST_CHECK_EQUAL(volBounds->get(CylinderVolumeBounds::eMaxR), 300_mm);
  }

  if (strategy == CylinderVolumeStack::ResizeStrategy::Expand) {
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
        dynamic_cast<const CylinderVolumeBounds*>(&vol->volumeBounds());
    BOOST_REQUIRE(volBounds != nullptr);
    BOOST_CHECK_EQUAL(volBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                      450_mm);
    BOOST_CHECK_EQUAL(vol->center()[eZ], f * 550_mm);
  } else if (strategy == CylinderVolumeStack::ResizeStrategy::Gap) {
    // One gap volume was added
    BOOST_CHECK_EQUAL(volumes.size(), 4);

    const Volume* gap = nullptr;
    if (f < 0.0) {
      gap = volumes.front();
    } else {
      gap = volumes.back();
    }
    const auto* gapBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&gap->volumeBounds());
    BOOST_REQUIRE(gapBounds != nullptr);

    BOOST_CHECK_EQUAL(gapBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                      50_mm);
    BOOST_CHECK_EQUAL(gap->center()[eZ], f * 950_mm);
  }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(RDirection)

BOOST_DATA_TEST_CASE(Baseline,
                     (boost::unit_test::data::xrange(-135, 180, 45) *
                      boost::unit_test::data::xrange(0, 2, 1) *
                      boost::unit_test::data::make(-0.1, 0.0, 0.1) *
                      boost::unit_test::data::make(Vector3{0_mm, 0_mm, 0_mm},
                                                   Vector3{20_mm, 0_mm, 0_mm},
                                                   Vector3{0_mm, 20_mm, 0_mm},
                                                   Vector3{20_mm, 20_mm, 0_mm},
                                                   Vector3{0_mm, 0_mm, 20_mm}) *
                      boost::unit_test::data::make(strategies)),
                     angle, rotate, f, offset, strategy) {
  ActsScalar hlZ = 400_mm;

  ActsScalar fInner = 1.0 + f;
  ActsScalar fOuter = 1.0 - f;

  // Cylinder volumes which already line up in r but have different z and hl
  auto bounds1 = std::make_shared<CylinderVolumeBounds>(fInner * 100_mm,
                                                        fOuter * 300_mm, hlZ);
  auto bounds2 = std::make_shared<CylinderVolumeBounds>(fInner * 300_mm,
                                                        fOuter * 600_mm, hlZ);
  auto bounds3 = std::make_shared<CylinderVolumeBounds>(fInner * 600_mm,
                                                        fOuter * 900_mm, hlZ);

  Transform3 base =
      AngleAxis3(angle * 1_degree, Vector3::UnitX()) * Translation3(offset);

  // volumes are shifted in z

  Transform3 transform1 = base;
  transform1.translate(Vector3{0_mm, 0_mm, 20_mm});
  auto vol1 = std::make_shared<Volume>(transform1, bounds1);

  Transform3 transform2 = base;
  transform2.translate(Vector3{0_mm, 0_mm, -30_mm});
  auto vol2 = std::make_shared<Volume>(transform2, bounds2);

  Transform3 transform3 = base;
  transform3.translate(Vector3{0_mm, 0_mm, 40_mm});
  auto vol3 = std::make_shared<Volume>(transform3, bounds3);

  std::vector<Volume*> volumes = {vol1.get(), vol2.get(), vol3.get()};
  // Rotate to simulate unsorted volumes: all results should be the same!
  std::rotate(volumes.begin(), volumes.begin() + rotate, volumes.end());

  std::vector<Volume*> origVolumes = volumes;

  std::vector<CylinderVolumeBounds> originalBounds;
  std::transform(
      volumes.begin(), volumes.end(), std::back_inserter(originalBounds),
      [](const auto& vol) {
        return dynamic_cast<const CylinderVolumeBounds&>(vol->volumeBounds());
      });

  if (f < 0.0) {
    BOOST_CHECK_THROW(
        CylinderVolumeStack(volumes, BinningValue::binR, strategy,
                            CylinderVolumeStack::ResizeStrategy::Gap, *logger),
        std::invalid_argument);
    return;
  }

  CylinderVolumeStack cylStack(volumes, BinningValue::binR, strategy,
                               CylinderVolumeStack::ResizeStrategy::Gap,
                               *logger);

  auto stackBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&cylStack.volumeBounds());
  BOOST_REQUIRE(stackBounds != nullptr);

  BOOST_CHECK_EQUAL(stackBounds->get(CylinderVolumeBounds::eMinR),
                    fInner * 100_mm);
  BOOST_CHECK_EQUAL(stackBounds->get(CylinderVolumeBounds::eMaxR),
                    fOuter * 900_mm);
  ActsScalar expectedHalfLengthZ = (40_mm + 30_mm + 2 * hlZ) / 2.0;
  BOOST_CHECK_EQUAL(stackBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                    expectedHalfLengthZ);

  // After synchronization, all volumes should have the same z position and half
  // length
  // This includes possible gap volumes!
  Transform3 commonTransform = base * Translation3{0_mm, 0_mm, 5_mm};

  CHECK_CLOSE_OR_SMALL(cylStack.transform().matrix(), commonTransform.matrix(),
                       1e-10, 1e-14);

  for (const auto& volume : volumes) {
    const auto* cylinderBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&volume->volumeBounds());
    BOOST_REQUIRE(cylinderBounds != nullptr);
    BOOST_CHECK_EQUAL(cylinderBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                      expectedHalfLengthZ);
  }

  BOOST_CHECK_EQUAL(
      dynamic_cast<const CylinderVolumeBounds&>(vol1->volumeBounds())
          .get(CylinderVolumeBounds::eMinR),
      fInner * 100_mm);

  BOOST_CHECK_EQUAL(
      dynamic_cast<const CylinderVolumeBounds&>(vol3->volumeBounds())
          .get(CylinderVolumeBounds::eMaxR),
      fOuter * 900_mm);

  // Volumes are sorted in r
  for (std::size_t i = 0; i < volumes.size() - 1; ++i) {
    const auto& a = volumes.at(i);
    const auto& b = volumes.at(i + 1);

    const auto* aBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&a->volumeBounds());
    const auto* bBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&b->volumeBounds());

    ActsScalar aMidR = (aBounds->get(CylinderVolumeBounds::eMinR) +
                        aBounds->get(CylinderVolumeBounds::eMaxR)) /
                       2.0;

    ActsScalar bMidR = (bBounds->get(CylinderVolumeBounds::eMinR) +
                        bBounds->get(CylinderVolumeBounds::eMaxR)) /
                       2.0;

    BOOST_CHECK_LT(aMidR, bMidR);
  }

  if (f == 0.0) {
    // No gap volumes were added
    BOOST_CHECK_EQUAL(volumes.size(), 3);

    // Original volumes did not change r bounds
    for (const auto& [volume, origCylBounds] :
         zip(origVolumes, originalBounds)) {
      const auto* newBounds =
          dynamic_cast<const CylinderVolumeBounds*>(&volume->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds->get(CylinderVolumeBounds::eMinR),
                        origCylBounds.get(CylinderVolumeBounds::eMinR));
      BOOST_CHECK_EQUAL(newBounds->get(CylinderVolumeBounds::eMaxR),
                        origCylBounds.get(CylinderVolumeBounds::eMaxR));
    }
  } else {
    const auto* newBounds1 =
        dynamic_cast<const CylinderVolumeBounds*>(&vol1->volumeBounds());
    const auto* newBounds2 =
        dynamic_cast<const CylinderVolumeBounds*>(&vol2->volumeBounds());
    const auto* newBounds3 =
        dynamic_cast<const CylinderVolumeBounds*>(&vol3->volumeBounds());
    if (strategy == CylinderVolumeStack::AttachmentStrategy::Gap) {
      // Two gap volumes were added
      BOOST_CHECK_EQUAL(volumes.size(), 5);

      // Original volumes did not change r bounds
      BOOST_CHECK_EQUAL(newBounds1->get(CylinderVolumeBounds::eMinR),
                        fInner * 100_mm);
      BOOST_CHECK_EQUAL(newBounds1->get(CylinderVolumeBounds::eMaxR),
                        fOuter * 300_mm);
      BOOST_CHECK_EQUAL(newBounds2->get(CylinderVolumeBounds::eMinR),
                        fInner * 300_mm);
      BOOST_CHECK_EQUAL(newBounds2->get(CylinderVolumeBounds::eMaxR),
                        fOuter * 600_mm);
      BOOST_CHECK_EQUAL(newBounds3->get(CylinderVolumeBounds::eMinR),
                        fInner * 600_mm);
      BOOST_CHECK_EQUAL(newBounds3->get(CylinderVolumeBounds::eMaxR),
                        fOuter * 900_mm);

      auto gap1 = volumes.at(1);
      auto gap2 = volumes.at(3);

      const auto* gapBounds1 =
          dynamic_cast<const CylinderVolumeBounds*>(&gap1->volumeBounds());
      const auto* gapBounds2 =
          dynamic_cast<const CylinderVolumeBounds*>(&gap2->volumeBounds());

      BOOST_CHECK_EQUAL(gapBounds1->get(CylinderVolumeBounds::eMinR),
                        fOuter * 300_mm);
      BOOST_CHECK_EQUAL(gapBounds1->get(CylinderVolumeBounds::eMaxR),
                        fInner * 300_mm);
      BOOST_CHECK_EQUAL(gapBounds2->get(CylinderVolumeBounds::eMinR),
                        fOuter * 600_mm);
      BOOST_CHECK_EQUAL(gapBounds2->get(CylinderVolumeBounds::eMaxR),
                        fInner * 600_mm);

    } else if (strategy == CylinderVolumeStack::AttachmentStrategy::First) {
      // No gap volumes were added
      BOOST_CHECK_EQUAL(volumes.size(), 3);

      // Volume 1 got bigger and grew to meet Volume 2
      BOOST_CHECK_EQUAL(newBounds1->get(CylinderVolumeBounds::eMinR),
                        fInner * 100_mm);
      BOOST_CHECK_EQUAL(newBounds1->get(CylinderVolumeBounds::eMaxR),
                        fInner * 300_mm);

      // Volume 2 got bigger and grew to meet Volume 3
      BOOST_CHECK_EQUAL(newBounds2->get(CylinderVolumeBounds::eMinR),
                        fInner * 300_mm);
      BOOST_CHECK_EQUAL(newBounds2->get(CylinderVolumeBounds::eMaxR),
                        fInner * 600_mm);

      // Volume 3 stayed the same
      BOOST_CHECK_EQUAL(newBounds3->get(CylinderVolumeBounds::eMinR),
                        fInner * 600_mm);
      BOOST_CHECK_EQUAL(newBounds3->get(CylinderVolumeBounds::eMaxR),
                        fOuter * 900_mm);

    } else if (strategy == CylinderVolumeStack::AttachmentStrategy::Second) {
      // No gap volumes were added
      BOOST_CHECK_EQUAL(volumes.size(), 3);

      // Volume 1 stayed the same
      BOOST_CHECK_EQUAL(newBounds1->get(CylinderVolumeBounds::eMinR),
                        fInner * 100_mm);
      BOOST_CHECK_EQUAL(newBounds1->get(CylinderVolumeBounds::eMaxR),
                        fOuter * 300_mm);

      // Volume 2 got bigger and grew inward to meet Volume 1
      BOOST_CHECK_EQUAL(newBounds2->get(CylinderVolumeBounds::eMinR),
                        fOuter * 300_mm);
      BOOST_CHECK_EQUAL(newBounds2->get(CylinderVolumeBounds::eMaxR),
                        fOuter * 600_mm);

      // Volume 3 got bigger and grew inward to meet Volume 2
      BOOST_CHECK_EQUAL(newBounds3->get(CylinderVolumeBounds::eMinR),
                        fOuter * 600_mm);
      BOOST_CHECK_EQUAL(newBounds3->get(CylinderVolumeBounds::eMaxR),
                        fOuter * 900_mm);
    } else if (strategy == CylinderVolumeStack::AttachmentStrategy::Midpoint) {
      // No gap volumes were added
      BOOST_CHECK_EQUAL(volumes.size(), 3);

      // Volume 1 grew outward to meet Volume 2 half way
      BOOST_CHECK_EQUAL(newBounds1->get(CylinderVolumeBounds::eMinR),
                        fInner * 100_mm);
      BOOST_CHECK_EQUAL(newBounds1->get(CylinderVolumeBounds::eMaxR),
                        (fOuter * 300_mm + fInner * 300_mm) / 2.0);

      // Volume 2 grew inward and outward to meet Volume 1 and 3 half way
      BOOST_CHECK_EQUAL(newBounds2->get(CylinderVolumeBounds::eMinR),
                        (fOuter * 300_mm + fInner * 300_mm) / 2.0);
      BOOST_CHECK_EQUAL(newBounds2->get(CylinderVolumeBounds::eMaxR),
                        (fOuter * 600_mm + fInner * 600_mm) / 2.0);

      // Volume 3 grew inward to meet Volume 2 half way
      BOOST_CHECK_EQUAL(newBounds3->get(CylinderVolumeBounds::eMinR),
                        (fOuter * 600_mm + fInner * 600_mm) / 2.0);
      BOOST_CHECK_EQUAL(newBounds3->get(CylinderVolumeBounds::eMaxR),
                        fOuter * 900_mm);
    }
  }
}

BOOST_DATA_TEST_CASE(UpdateStack,
                     (boost::unit_test::data::xrange(-135, 180, 45) *
                      boost::unit_test::data::make(Vector3{0_mm, 0_mm, 0_mm},
                                                   Vector3{20_mm, 0_mm, 0_mm},
                                                   Vector3{0_mm, 20_mm, 0_mm},
                                                   Vector3{20_mm, 20_mm, 0_mm},
                                                   Vector3{0_mm, 0_mm, 20_mm}) *
                      boost::unit_test::data::make(-100_mm, 0_mm, 100_mm) *
                      boost::unit_test::data::make(resizeStrategies)),
                     angle, offset, zshift, strategy) {
  ActsScalar hlZ = 400_mm;

  // Cylinder volumes which already line up in r but have different z and hl
  auto bounds1 = std::make_shared<CylinderVolumeBounds>(100_mm, 300_mm, hlZ);
  auto bounds2 = std::make_shared<CylinderVolumeBounds>(300_mm, 600_mm, hlZ);
  auto bounds3 = std::make_shared<CylinderVolumeBounds>(600_mm, 900_mm, hlZ);

  Transform3 base = AngleAxis3(angle * 1_degree, Vector3::UnitX()) *
                    Translation3(offset + Vector3{0, 0, zshift});

  // volumes are shifted in z
  auto vol1 = std::make_shared<Volume>(base, bounds1);
  auto vol2 = std::make_shared<Volume>(base, bounds2);
  auto vol3 = std::make_shared<Volume>(base, bounds3);

  std::vector<Volume*> volumes = {vol1.get(), vol2.get(), vol3.get()};
  std::vector<Volume*> originalVolumes = volumes;

  std::vector<CylinderVolumeBounds> originalBounds;

  std::transform(
      volumes.begin(), volumes.end(), std::back_inserter(originalBounds),
      [](const auto& vol) {
        return *dynamic_cast<const CylinderVolumeBounds*>(&vol->volumeBounds());
      });

  const CylinderVolumeBounds* originalOuterBounds = nullptr;

  std::unique_ptr<CylinderVolumeStack> cylStack;

  auto resetCylStack = [&]() {
    volumes = originalVolumes;

    for (const auto& [volume, origBounds] : zip(volumes, originalBounds)) {
      volume->assignVolumeBounds(
          std::make_shared<CylinderVolumeBounds>(origBounds));
    }

    cylStack = std::make_unique<CylinderVolumeStack>(
        volumes, BinningValue::binR,
        CylinderVolumeStack::AttachmentStrategy::Gap,  // should not make a
                                                       // difference
        strategy, *logger);

    originalOuterBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&cylStack->volumeBounds());
  };

  resetCylStack();

  auto assertInitialVolumesUnchanged = [&]() {
    for (const auto& [volume, origCylBounds] :
         zip(originalVolumes, originalBounds)) {
      const auto* newBounds =
          dynamic_cast<const CylinderVolumeBounds*>(&volume->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds->get(CylinderVolumeBounds::eMinR),
                        origCylBounds.get(CylinderVolumeBounds::eMinR));
      BOOST_CHECK_EQUAL(newBounds->get(CylinderVolumeBounds::eMaxR),
                        origCylBounds.get(CylinderVolumeBounds::eMaxR));
      BOOST_CHECK_EQUAL(newBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                        origCylBounds.get(CylinderVolumeBounds::eHalfLengthZ));
      BOOST_CHECK_EQUAL(volume->transform().matrix(), base.matrix());
    }
  };

  auto assertOriginalBounds = [&]() {
    const auto* cylBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&cylStack->volumeBounds());
    BOOST_REQUIRE(cylBounds != nullptr);
    BOOST_CHECK_EQUAL(cylBounds, originalOuterBounds);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMinR), 100_mm);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMaxR), 900_mm);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eHalfLengthZ), hlZ);
  };

  assertOriginalBounds();

  {
    // Assign a copy of the identical bounds gives identical bounds
    auto bounds = std::make_shared<CylinderVolumeBounds>(
        dynamic_cast<const CylinderVolumeBounds&>(cylStack->volumeBounds()));
    cylStack->update(bounds, std::nullopt, *logger);
    assertOriginalBounds();
  }

  {
    // Cannot increase mininmum r
    auto bounds = std::make_shared<CylinderVolumeBounds>(
        dynamic_cast<const CylinderVolumeBounds&>(cylStack->volumeBounds()));
    bounds->set(CylinderVolumeBounds::eMinR, 200_mm);
    BOOST_CHECK_THROW(cylStack->update(bounds, std::nullopt, *logger),
                      std::invalid_argument);
    assertOriginalBounds();
  }

  {
    // Cannot decrease maximum r
    auto bounds = std::make_shared<CylinderVolumeBounds>(
        dynamic_cast<const CylinderVolumeBounds&>(cylStack->volumeBounds()));
    bounds->set(CylinderVolumeBounds::eMaxR, 500_mm);
    BOOST_CHECK_THROW(cylStack->update(bounds, std::nullopt, *logger),
                      std::invalid_argument);
    assertOriginalBounds();
  }

  {
    // Cannot decrease half length z
    auto bounds = std::make_shared<CylinderVolumeBounds>(
        dynamic_cast<const CylinderVolumeBounds&>(cylStack->volumeBounds()));
    bounds->set(CylinderVolumeBounds::eHalfLengthZ, 0.5 * hlZ);
    BOOST_CHECK_THROW(cylStack->update(bounds, std::nullopt, *logger),
                      std::invalid_argument);
    assertOriginalBounds();
  }

  {
    // Reduce minimum r
    auto bounds = std::make_shared<CylinderVolumeBounds>(
        dynamic_cast<const CylinderVolumeBounds&>(cylStack->volumeBounds()));
    bounds->set(CylinderVolumeBounds::eMinR, 50_mm);
    cylStack->update(bounds, std::nullopt, *logger);
    const auto* cylBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&cylStack->volumeBounds());
    BOOST_REQUIRE(cylBounds != nullptr);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMinR), 50_mm);
    // Rest unchanged
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMaxR), 900_mm);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eHalfLengthZ), hlZ);

    if (strategy == CylinderVolumeStack::ResizeStrategy::Expand) {
      // No gap volumes were added
      BOOST_CHECK_EQUAL(volumes.size(), 3);

      // Innermost volume reduced r size
      const auto* newBounds1 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol1->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds1->get(CylinderVolumeBounds::eMinR), 50_mm);
      // Position stayed the same
      BOOST_CHECK_EQUAL(vol1->transform().matrix(), base.matrix());

      // Other volumes are unchanged
      const auto* newBounds2 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol2->volumeBounds());
      BOOST_CHECK_EQUAL(*newBounds2, originalBounds[1]);
      BOOST_CHECK_EQUAL(vol2->transform().matrix(), base.matrix());

      const auto* newBounds3 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol3->volumeBounds());
      BOOST_CHECK_EQUAL(*newBounds3, originalBounds[2]);
      BOOST_CHECK_EQUAL(vol3->transform().matrix(), base.matrix());

    } else if (strategy == CylinderVolumeStack::ResizeStrategy::Gap) {
      // One gap volume was added
      BOOST_CHECK_EQUAL(volumes.size(), 4);

      auto gap = volumes.front();
      auto gapBounds =
          dynamic_cast<const CylinderVolumeBounds*>(&gap->volumeBounds());
      BOOST_REQUIRE(gapBounds != nullptr);
      BOOST_CHECK_EQUAL(gapBounds->get(CylinderVolumeBounds::eMinR), 50_mm);
      BOOST_CHECK_EQUAL(gapBounds->get(CylinderVolumeBounds::eMaxR), 100_mm);
      BOOST_CHECK_EQUAL(gapBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                        hlZ);
      BOOST_CHECK_EQUAL(gap->transform().matrix(), base.matrix());

      // Other volumes are unchanged
      assertInitialVolumesUnchanged();
    }
  }

  resetCylStack();

  {
    // Increase maximum r
    auto bounds = std::make_shared<CylinderVolumeBounds>(
        dynamic_cast<const CylinderVolumeBounds&>(cylStack->volumeBounds()));
    bounds->set(CylinderVolumeBounds::eMaxR, 1000_mm);
    cylStack->update(bounds, std::nullopt, *logger);
    const auto* cylBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&cylStack->volumeBounds());
    BOOST_REQUIRE(cylBounds != nullptr);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMaxR), 1000_mm);
    // Rest as before
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMinR), 100_mm);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eHalfLengthZ), hlZ);

    if (strategy == CylinderVolumeStack::ResizeStrategy::Expand) {
      // No gap volumes were added
      BOOST_CHECK_EQUAL(volumes.size(), 3);

      // Outermost volume increased r size
      const auto* newBounds3 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol3->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds3->get(CylinderVolumeBounds::eMaxR), 1000_mm);
      // Position stayed the same
      BOOST_CHECK_EQUAL(vol3->transform().matrix(), base.matrix());

      // Other volumes are unchanged
      const auto* newBounds1 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol1->volumeBounds());
      BOOST_CHECK_EQUAL(*newBounds1, originalBounds[0]);
      BOOST_CHECK_EQUAL(vol1->transform().matrix(), base.matrix());

      const auto* newBounds2 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol2->volumeBounds());
      BOOST_CHECK_EQUAL(*newBounds2, originalBounds[1]);
      BOOST_CHECK_EQUAL(vol2->transform().matrix(), base.matrix());

    } else if (strategy == CylinderVolumeStack::ResizeStrategy::Gap) {
      // One gap volume was added
      BOOST_CHECK_EQUAL(volumes.size(), 4);

      auto gap = volumes.back();
      auto gapBounds =
          dynamic_cast<const CylinderVolumeBounds*>(&gap->volumeBounds());
      BOOST_REQUIRE(gapBounds != nullptr);
      BOOST_CHECK_EQUAL(gapBounds->get(CylinderVolumeBounds::eMinR), 900_mm);
      BOOST_CHECK_EQUAL(gapBounds->get(CylinderVolumeBounds::eMaxR), 1000_mm);
      BOOST_CHECK_EQUAL(gapBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                        hlZ);
      BOOST_CHECK_EQUAL(gap->transform().matrix(), base.matrix());

      // Other volumes are unchanged
      assertInitialVolumesUnchanged();
    }
  }

  resetCylStack();

  {
    // Decrease r min and increase r max
    auto bounds = std::make_shared<CylinderVolumeBounds>(
        dynamic_cast<const CylinderVolumeBounds&>(cylStack->volumeBounds()));
    bounds->set({
        {CylinderVolumeBounds::eMinR, 0_mm},
        {CylinderVolumeBounds::eMaxR, 1100_mm},
    });

    cylStack->update(bounds, std::nullopt, *logger);
    const auto* cylBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&cylStack->volumeBounds());
    BOOST_REQUIRE(cylBounds != nullptr);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMaxR), 1100_mm);
    // Rest as before
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMinR), 0_mm);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eHalfLengthZ), hlZ);

    if (strategy == CylinderVolumeStack::ResizeStrategy::Expand) {
      // No gap volumes were added
      BOOST_CHECK_EQUAL(volumes.size(), 3);

      // Innermost volume reduced r size
      const auto* newBounds1 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol1->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds1->get(CylinderVolumeBounds::eMinR), 0_mm);
      // Position stayed the same
      BOOST_CHECK_EQUAL(vol1->transform().matrix(), base.matrix());

      // Middle volume is unchanged
      const auto* newBounds2 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol2->volumeBounds());
      BOOST_CHECK_EQUAL(*newBounds2, originalBounds[1]);
      BOOST_CHECK_EQUAL(vol2->transform().matrix(), base.matrix());

      // Outermost volume increased r size
      const auto* newBounds3 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol3->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds3->get(CylinderVolumeBounds::eMaxR), 1100_mm);
      // Position stayed the same
      BOOST_CHECK_EQUAL(vol3->transform().matrix(), base.matrix());

    } else if (strategy == CylinderVolumeStack::ResizeStrategy::Gap) {
      // One gap volume was added
      BOOST_CHECK_EQUAL(volumes.size(), 5);

      auto gap1 = volumes.front();
      auto gapBounds1 =
          dynamic_cast<const CylinderVolumeBounds*>(&gap1->volumeBounds());
      BOOST_REQUIRE(gapBounds1 != nullptr);
      BOOST_CHECK_EQUAL(gapBounds1->get(CylinderVolumeBounds::eMinR), 0_mm);
      BOOST_CHECK_EQUAL(gapBounds1->get(CylinderVolumeBounds::eMaxR), 100_mm);
      BOOST_CHECK_EQUAL(gapBounds1->get(CylinderVolumeBounds::eHalfLengthZ),
                        hlZ);
      BOOST_CHECK_EQUAL(gap1->transform().matrix(), base.matrix());

      auto gap2 = volumes.back();
      auto gapBounds2 =
          dynamic_cast<const CylinderVolumeBounds*>(&gap2->volumeBounds());
      BOOST_REQUIRE(gapBounds2 != nullptr);
      BOOST_CHECK_EQUAL(gapBounds2->get(CylinderVolumeBounds::eMinR), 900_mm);
      BOOST_CHECK_EQUAL(gapBounds2->get(CylinderVolumeBounds::eMaxR), 1100_mm);
      BOOST_CHECK_EQUAL(gapBounds2->get(CylinderVolumeBounds::eHalfLengthZ),
                        hlZ);

      // Other volumes are unchanged
      assertInitialVolumesUnchanged();
    }
  }

  resetCylStack();

  {
    // Increase half length z
    auto bounds = std::make_shared<CylinderVolumeBounds>(
        dynamic_cast<const CylinderVolumeBounds&>(cylStack->volumeBounds()));
    bounds->set(CylinderVolumeBounds::eHalfLengthZ, 2 * hlZ);
    cylStack->update(bounds, std::nullopt, *logger);
    const auto* cylBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&cylStack->volumeBounds());
    BOOST_REQUIRE(cylBounds != nullptr);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                      2 * hlZ);

    // Rest as before
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMinR), 100_mm);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMaxR), 900_mm);

    // No gap volumes were added
    BOOST_CHECK_EQUAL(volumes.size(), 3);

    for (const auto& [volume, origCylBounds] :
         zip(originalVolumes, originalBounds)) {
      const auto* newBounds =
          dynamic_cast<const CylinderVolumeBounds*>(&volume->volumeBounds());
      // Radii are all as before
      BOOST_CHECK_EQUAL(newBounds->get(CylinderVolumeBounds::eMinR),
                        origCylBounds.get(CylinderVolumeBounds::eMinR));
      BOOST_CHECK_EQUAL(newBounds->get(CylinderVolumeBounds::eMaxR),
                        origCylBounds.get(CylinderVolumeBounds::eMaxR));

      // Half length z is changed on all
      BOOST_CHECK_EQUAL(newBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                        2 * hlZ);

      // Position stayed the same
      BOOST_CHECK_EQUAL(volume->transform().matrix(), base.matrix());
    }
  }
}

BOOST_DATA_TEST_CASE(
    UpdateStackOneSided,
    (boost::unit_test::data::make(-1.0, 1.0) ^
     boost::unit_test::data::make(CylinderVolumeStack::ResizeStrategy::Gap,
                                  CylinderVolumeStack::ResizeStrategy::Expand)),
    f, strategy) {
  // Strategy should not affect the sizing here at all

  auto trf = Transform3::Identity();

  auto vol1 = std::make_shared<Volume>(
      trf, std::make_shared<CylinderVolumeBounds>(100_mm, 300_mm, 400_mm));

  auto vol2 = std::make_shared<Volume>(
      trf, std::make_shared<CylinderVolumeBounds>(400_mm, 600_mm, 400_mm));

  std::vector<Volume*> volumes = {vol1.get(), vol2.get()};

  CylinderVolumeStack cylStack{volumes, BinningValue::binR,
                               CylinderVolumeStack::AttachmentStrategy::Gap,
                               strategy, *logger};
  const auto* originalBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&cylStack.volumeBounds());

  // Increase halflength by 50mm
  auto newBounds = std::make_shared<CylinderVolumeBounds>(
      dynamic_cast<const CylinderVolumeBounds&>(cylStack.volumeBounds()));
  newBounds->set(CylinderVolumeBounds::eHalfLengthZ, 450_mm);
  // Shift to +z by 50mm
  trf *= Translation3{Vector3{0_mm, 0_mm, f * 50_mm}};
  // -> left edge should stay at -400mm, right edge should be at 500mm

  auto checkUnchanged = [&]() {
    const auto* cylBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&cylStack.volumeBounds());
    BOOST_REQUIRE(cylBounds != nullptr);
    BOOST_CHECK_EQUAL(*cylBounds, *originalBounds);
  };

  // Invalid: shift too far in z
  BOOST_CHECK_THROW(
      cylStack.update(newBounds, trf * Translation3{Vector3{0, 0, f * 20_mm}},
                      *logger),
      std::invalid_argument);
  checkUnchanged();

  // Invalid: shift in x
  BOOST_CHECK_THROW(
      cylStack.update(newBounds, trf * Translation3{Vector3{10_mm, 0, 0}},
                      *logger),
      std::invalid_argument);
  checkUnchanged();

  // Invalid: shift in y
  BOOST_CHECK_THROW(
      cylStack.update(newBounds, trf * Translation3{Vector3{0, 10_mm, 0}},
                      *logger),
      std::invalid_argument);
  checkUnchanged();

  // Invalid: rotation
  BOOST_CHECK_THROW(
      cylStack.update(newBounds, trf * AngleAxis3{10_degree, Vector3::UnitY()},
                      *logger),
      std::invalid_argument);
  checkUnchanged();

  cylStack.update(newBounds, trf, *logger);

  BOOST_CHECK_EQUAL(cylStack.transform().matrix(), trf.matrix());
  const auto* cylBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&cylStack.volumeBounds());
  BOOST_REQUIRE(cylBounds != nullptr);
  BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMinR), 100_mm);
  BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMaxR), 600_mm);

  // All volumes including gaps should have same new position and halflength
  for (const auto* vol : volumes) {
    const auto* volBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&vol->volumeBounds());
    BOOST_REQUIRE(volBounds != nullptr);
    BOOST_CHECK_EQUAL(vol->transform().matrix(), trf.matrix());
    BOOST_CHECK_EQUAL(volBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                      450_mm);
  }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Common)

BOOST_DATA_TEST_CASE(JoinCylinderVolumesInvalidDirection,
                     boost::unit_test::data::make(strategies), strategy) {
  std::vector<Volume*> volumes;
  auto vol1 = std::make_shared<Volume>(
      Transform3::Identity(),
      std::make_shared<CylinderVolumeBounds>(100_mm, 400_mm, 400_mm));
  volumes.push_back(vol1.get());

  // Single volume invalid direction still gives an error
  BOOST_CHECK_THROW(CylinderVolumeStack(volumes, BinningValue::binY, strategy),
                    std::invalid_argument);

  auto vol2 = std::make_shared<Volume>(
      Transform3::Identity(),
      std::make_shared<CylinderVolumeBounds>(100_mm, 400_mm, 400_mm));
  volumes.push_back(vol2.get());

  BOOST_CHECK_THROW(CylinderVolumeStack(volumes, BinningValue::binY, strategy),
                    std::invalid_argument);
}

BOOST_DATA_TEST_CASE(JoinCylinderVolumesInvalidInput,
                     (boost::unit_test::data::make(strategies) *
                      boost::unit_test::data::make(Acts::BinningValue::binZ,
                                                   Acts::BinningValue::binR)),
                     strategy, direction) {
  BOOST_TEST_CONTEXT("Empty Volume") {
    std::vector<Volume*> volumes;
    BOOST_CHECK_THROW(CylinderVolumeStack(volumes, direction, strategy),
                      std::invalid_argument);
  }

  BOOST_TEST_CONTEXT("Volumes rotated relative to each other") {
    // At this time, all rotations are considered invalid, even around z
    for (const Vector3 axis : {Vector3::UnitX(), Vector3::UnitY()}) {
      std::vector<Volume*> volumes;
      auto vol1 = std::make_shared<Volume>(
          Transform3{Translation3{Vector3{0_mm, 0_mm, -500_mm}}},
          std::make_shared<CylinderVolumeBounds>(100_mm, 400_mm, 400_mm));
      volumes.push_back(vol1.get());

      BOOST_TEST_MESSAGE("Axis: " << axis);
      auto vol2 = std::make_shared<Volume>(
          Transform3{Translation3{Vector3{0_mm, 0_mm, 500_mm}} *
                     AngleAxis3(1_degree, axis)},
          std::make_shared<CylinderVolumeBounds>(100_mm, 400_mm, 400_mm));
      volumes.push_back(vol2.get());

      BOOST_CHECK_THROW(CylinderVolumeStack(
                            volumes, direction, strategy,
                            CylinderVolumeStack::ResizeStrategy::Gap, *logger),
                        std::invalid_argument);
    }
  }

  BOOST_TEST_CONTEXT("Volumes shifted in the xy plane relative to each other") {
    for (const Vector3& shift :
         {Vector3{5_mm, 0, 0}, Vector3{0, -5_mm, 0}, Vector3{2_mm, -2_mm, 0}}) {
      std::vector<Volume*> volumes;
      auto vol1 = std::make_shared<Volume>(
          Transform3{Translation3{Vector3{0_mm, 0_mm, -500_mm}}},
          std::make_shared<CylinderVolumeBounds>(100_mm, 400_mm, 400_mm));
      volumes.push_back(vol1.get());

      auto vol2 = std::make_shared<Volume>(
          Transform3{Translation3{Vector3{0_mm, 0_mm, 500_mm} + shift}},
          std::make_shared<CylinderVolumeBounds>(100_mm, 400_mm, 400_mm));
      volumes.push_back(vol2.get());

      BOOST_CHECK_THROW(CylinderVolumeStack(
                            volumes, direction, strategy,
                            CylinderVolumeStack::ResizeStrategy::Gap, *logger),
                        std::invalid_argument);
    }
  }

  BOOST_TEST_CONTEXT("Volume has phi values or bevel values") {
    std::vector<std::shared_ptr<CylinderVolumeBounds>> invalidVolumeBounds = {
        std::make_shared<CylinderVolumeBounds>(100_mm, 400_mm, 400_mm,
                                               0.2 * M_PI),

        std::make_shared<CylinderVolumeBounds>(100_mm, 400_mm, 400_mm, M_PI,
                                               0.3 * M_PI),

        std::make_shared<CylinderVolumeBounds>(100_mm, 400_mm, 400_mm, M_PI,
                                               0.0, 0.3 * M_PI),
        std::make_shared<CylinderVolumeBounds>(100_mm, 400_mm, 400_mm, M_PI,
                                               0.0, 0.0, 0.3 * M_PI),
    };

    for (const auto& invalid : invalidVolumeBounds) {
      std::stringstream ss;
      ss << "Invalid bounds: " << *invalid;
      BOOST_TEST_CONTEXT(ss.str()) {
        std::vector<Volume*> volumes;
        auto vol1 = std::make_shared<Volume>(
            Transform3{Translation3{Vector3{0_mm, 0_mm, -500_mm}}},
            std::make_shared<CylinderVolumeBounds>(100_mm, 400_mm, 400_mm));
        volumes.push_back(vol1.get());

        {
          // have valid stack, try to assign extra
          CylinderVolumeStack cylStack(volumes, direction, strategy,
                                       CylinderVolumeStack::ResizeStrategy::Gap,
                                       *logger);
          BOOST_CHECK_THROW(cylStack.update(invalid, std::nullopt, *logger),
                            std::invalid_argument);
        }

        {
          std::shared_ptr<Volume> vol;
          if (direction == BinningValue::binZ) {
            vol = std::make_shared<Volume>(
                Transform3{Translation3{Vector3{0_mm, 0_mm, 500_mm}}}, invalid);
          } else {
            invalid->set({
                {CylinderVolumeBounds::eMinR, 400_mm},
                {CylinderVolumeBounds::eMaxR, 600_mm},
            });
            vol = std::make_shared<Volume>(
                Transform3{Translation3{Vector3{0_mm, 0_mm, 0_mm}}}, invalid);
          }
          volumes.push_back(vol.get());
          BOOST_CHECK_THROW(
              CylinderVolumeStack(volumes, direction, strategy,
                                  CylinderVolumeStack::ResizeStrategy::Gap,
                                  *logger),
              std::invalid_argument);
        }
      }
    }
  }
}

BOOST_DATA_TEST_CASE(JoinCylinderVolumeSingle,
                     (boost::unit_test::data::make(Acts::BinningValue::binZ,
                                                   Acts::BinningValue::binR) *
                      boost::unit_test::data::make(strategies)),
                     direction, strategy) {
  auto vol = std::make_shared<Volume>(
      Transform3::Identity() * Translation3{14_mm, 24_mm, 0_mm} *
          AngleAxis3(73_degree, Vector3::UnitX()),
      std::make_shared<CylinderVolumeBounds>(100_mm, 400_mm, 400_mm));

  std::vector<Volume*> volumes{vol.get()};

  CylinderVolumeStack cylStack(volumes, direction, strategy,
                               CylinderVolumeStack::ResizeStrategy::Gap,
                               *logger);

  // Cylinder stack has the same transform as bounds as the single input
  // volume
  BOOST_CHECK_EQUAL(volumes.size(), 1);
  BOOST_CHECK_EQUAL(volumes.at(0), vol.get());
  BOOST_CHECK_EQUAL(vol->transform().matrix(), cylStack.transform().matrix());
  BOOST_CHECK_EQUAL(vol->volumeBounds(), cylStack.volumeBounds());
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
