// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Navigation/NavigationDelegates.hpp"
#include "Acts/Navigation/NavigationState.hpp"
#include "Acts/Navigation/PortalNavigation.hpp"
#include "Acts/Utilities/BinningType.hpp"

#include <memory>
#include <stdexcept>
#include <vector>

// A test context
Acts::GeometryContext tContext;

namespace Acts::Experimental {
class DetectorVolume {};
}  // namespace Acts::Experimental

auto volumeA = std::make_shared<Acts::Experimental::DetectorVolume>();
auto volumeB = std::make_shared<Acts::Experimental::DetectorVolume>();
auto volumeC = std::make_shared<Acts::Experimental::DetectorVolume>();
auto volumeD = std::make_shared<Acts::Experimental::DetectorVolume>();

Acts::Experimental::NavigationState nState;

BOOST_AUTO_TEST_SUITE(Experimental)

// These tests check the behavior of the volume updators, i.e. the
// helper delegates that set/reset the volume raw pointer in the
// NavigaitonState according to some given information.
//
BOOST_AUTO_TEST_CASE(UnconnectedUpdate) {
  Acts::Experimental::ExternalNavigationDelegate ucUpdater;
  BOOST_CHECK(!ucUpdater.connected());
}

// The end of world is reached
BOOST_AUTO_TEST_CASE(EndOfWorldUpdate) {
  nState.currentVolume = volumeA.get();
  BOOST_CHECK_EQUAL(nState.currentVolume, volumeA.get());

  Acts::Experimental::EndOfWorld eow;
  eow.update(tContext, nState);

  BOOST_CHECK_EQUAL(nState.currentVolume, nullptr);
}

// A single link exists and this is set
BOOST_AUTO_TEST_CASE(SingleVolumeUpdate) {
  nState.currentVolume = volumeA.get();
  BOOST_CHECK_EQUAL(nState.currentVolume, volumeA.get());

  Acts::Experimental::SingleDetectorVolumeNavigation svu(volumeB.get());
  svu.update(tContext, nState);

  BOOST_CHECK_EQUAL(nState.currentVolume, volumeB.get());

  BOOST_CHECK_THROW(Acts::Experimental::SingleDetectorVolumeNavigation(nullptr),
                    std::invalid_argument);
}

// A typlical volume array in 1 dimension (bound, not closed)
BOOST_AUTO_TEST_CASE(VolumeArrayUpdate) {
  std::vector<double> zArray = {-200, -100, 100, 400, 1000};

  std::vector<const Acts::Experimental::DetectorVolume*> volumes = {
      volumeA.get(), volumeB.get(), volumeC.get(), volumeD.get()};
  Acts::Experimental::BoundVolumesGrid1Navigation bvg(
      zArray, Acts::AxisDirection::AxisZ, volumes);
  // Reset the navigation state
  nState.currentVolume = nullptr;

  // Check the volume retrieval
  nState.position = Acts::Vector3(0., 0., -150.);
  bvg.update(tContext, nState);
  BOOST_CHECK_EQUAL(nState.currentVolume, volumeA.get());

  nState.position = Acts::Vector3(0., 0., 600.);
  bvg.update(tContext, nState);
  BOOST_CHECK_EQUAL(nState.currentVolume, volumeD.get());

  // Check a shifted one
  Acts::Transform3 shift300 = Acts::Transform3::Identity();
  shift300.pretranslate(Acts::Vector3(0, 0, 300));

  Acts::Experimental::BoundVolumesGrid1Navigation bvgs(
      zArray, Acts::AxisDirection::AxisZ, volumes, shift300.inverse());

  // 150 (-300) -> transforms to -150, hence it yields A
  nState.position = Acts::Vector3(0., 0., 150.);
  bvgs.update(tContext, nState);
  BOOST_CHECK_EQUAL(nState.currentVolume, volumeA.get());
}

BOOST_AUTO_TEST_SUITE_END()
