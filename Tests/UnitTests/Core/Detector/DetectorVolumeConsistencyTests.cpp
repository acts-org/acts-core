// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Detector/PortalGenerators.hpp"
#include "Acts/Detector/detail/DetectorVolumeConsistency.hpp"
#include "Acts/Geometry/CuboidVolumeBounds.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Navigation/InternalNavigation.hpp"

auto portalGenerator = Acts::Experimental::defaultPortalGenerator();
auto tContext = Acts::GeometryContext();

BOOST_AUTO_TEST_SUITE(Detector)

BOOST_AUTO_TEST_CASE(DetectorVolumeConsistencyFail) {
  // A perfect box shape
  auto box = std::make_shared<Acts::CuboidVolumeBounds>(10, 10, 10);

  // Create volume A
  auto volumeA = Acts::Experimental::DetectorVolumeFactory::construct(
      portalGenerator, tContext, "VolumeA", Acts::Transform3::Identity(), box,
      Acts::Experimental::tryAllPortals());

  // Move it into the bval direction
  auto transformB = Acts::Transform3::Identity();
  Acts::Vector3 translationB = Acts::Vector3::Zero();
  translationB[toUnderlying(Acts::BinningValue::binX)] = 20;
  translationB[toUnderlying(Acts::BinningValue::binY)] = 5;
  transformB.pretranslate(translationB);
  // Create volume B
  auto volumeB = Acts::Experimental::DetectorVolumeFactory::construct(
      portalGenerator, tContext, "VolumeB", transformB, box,
      Acts::Experimental::tryAllPortals());
  // Build the container
  std::vector<std::shared_ptr<Acts::Experimental::DetectorVolume>> volumes = {
      volumeA, volumeB};

  BOOST_CHECK_THROW(Acts::Experimental::detail::DetectorVolumeConsistency::
                        checkCenterAlignment(tContext, {volumeA, volumeB},
                                             Acts::BinningValue::binX),
                    std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(DetectorVolumeConsistencyPass) {
  // A perfect box shape
  auto box = std::make_shared<Acts::CuboidVolumeBounds>(10, 10, 10);

  // Create volume A
  auto volumeA = Acts::Experimental::DetectorVolumeFactory::construct(
      portalGenerator, tContext, "VolumeA", Acts::Transform3::Identity(), box,
      Acts::Experimental::tryAllPortals());

  // Move it into the bval direction
  auto transformB = Acts::Transform3::Identity();
  Acts::Vector3 translationB = Acts::Vector3::Zero();
  translationB[toUnderlying(Acts::BinningValue::binX)] = 20;
  transformB.pretranslate(translationB);
  // Create volume B
  auto volumeB = Acts::Experimental::DetectorVolumeFactory::construct(
      portalGenerator, tContext, "VolumeB", transformB, box,
      Acts::Experimental::tryAllPortals());
  // Build the container
  std::vector<std::shared_ptr<Acts::Experimental::DetectorVolume>> volumes = {
      volumeA, volumeB};

  BOOST_CHECK_NO_THROW(Acts::Experimental::detail::DetectorVolumeConsistency::
                           checkCenterAlignment(tContext, {volumeA, volumeB},
                                                Acts::BinningValue::binX));
}

BOOST_AUTO_TEST_SUITE_END()
