// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"

using namespace Acts::UnitLiterals;

namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(Geometry)
BOOST_AUTO_TEST_SUITE(TrackingVolumeTests)

std::size_t countVolumes(const TrackingVolume& tv) {
  std::size_t count = 0;
  tv.visitVolumes([&count](const auto&) { ++count; });
  return count;
}

BOOST_AUTO_TEST_CASE(TrackigVolumeChildren) {
  auto cylBounds =
      std::make_shared<Acts::CylinderVolumeBounds>(10_mm, 20_mm, 100_mm);

  TrackingVolume tv{Transform3::Identity(), cylBounds};

  BOOST_CHECK(tv.volumes().empty());
  BOOST_CHECK_EQUAL(countVolumes(tv), 1);

  auto& child1 = tv.addVolume(
      std::make_unique<TrackingVolume>(Transform3::Identity(), cylBounds));

  BOOST_CHECK_EQUAL(tv.volumes().size(), 1);

  auto it = tv.volumes().begin();
  static_assert(std::is_same_v<decltype(*it), TrackingVolume&>);

  const auto& tvConst = tv;
  auto cit = tvConst.volumes().begin();
  static_assert(std::is_same_v<decltype(*cit), const TrackingVolume&>);

  BOOST_CHECK_EQUAL(&*it, &child1);

  BOOST_CHECK_EQUAL(countVolumes(tv), 2);

  tv.addVolume(
      std::make_unique<TrackingVolume>(Transform3::Identity(), cylBounds));

  BOOST_CHECK_EQUAL(countVolumes(tv), 3);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
