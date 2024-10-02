// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Navigation/DetectorVolumeFinders.hpp"
#include "Acts/Navigation/PortalNavigation.hpp"
#include "Acts/Plugins/Json/DetectorVolumeFinderJsonConverter.hpp"
#include "Acts/Utilities/GridAxisGenerators.hpp"

#include <fstream>
#include <memory>
#include <vector>

#include <nlohmann/json.hpp>

BOOST_AUTO_TEST_SUITE(DetectorVolumeFinderJsonConverter)

BOOST_AUTO_TEST_CASE(RzVolumes) {
  std::vector<Acts::ActsScalar> zBoundaries = {-1000., -500, 150.};
  std::vector<Acts::ActsScalar> rBoundaries = {0., 10., 30., 35.};

  using AxesGeneratorType = Acts::GridAxisGenerators::VarBoundVarBound;

  AxesGeneratorType zrAxes{zBoundaries, rBoundaries};

  // Create the grid with the provided axis generator
  using GridType = typename AxesGeneratorType::template grid_type<std::size_t>;
  GridType grid(zrAxes());

  using PointType = typename GridType::point_t;

  PointType p11 = {-800., 5.};
  PointType p12 = {-800., 20.};
  PointType p13 = {-800., 32.};

  grid.atPosition(p11) = 11u;
  grid.atPosition(p12) = 12u;
  grid.atPosition(p13) = 13u;

  PointType p21 = {0., 5.};
  PointType p22 = {0., 20.};
  PointType p23 = {0., 32.};

  grid.atPosition(p21) = 21u;
  grid.atPosition(p22) = 22u;
  grid.atPosition(p23) = 23u;

  auto casts = std::array<Acts::BinningValue, 2u>{Acts::BinningValue::binZ,
                                                  Acts::BinningValue::binR};

  using IndexedDetectorVolumesImpl = Acts::Experimental::IndexedGridNavigation<
      Acts::Experimental::IExternalNavigation, GridType,
      Acts::Experimental::IndexedDetectorVolumeExtractor,
      Acts::Experimental::DetectorVolumeFiller>;

  auto indexedDetectorVolumesImpl =
      std::make_unique<const IndexedDetectorVolumesImpl>(std::move(grid),
                                                         casts);

  // Return the root volume finder
  Acts::Experimental::ExternalNavigationDelegate rootVolumeFinder;
  rootVolumeFinder.connect<&IndexedDetectorVolumesImpl::update>(
      std::move(indexedDetectorVolumesImpl));

  nlohmann::json rFinderJson =
      Acts::DetectorVolumeFinderJsonConverter::toJson(rootVolumeFinder);

  auto readInRootVolumeFinder =
      Acts::DetectorVolumeFinderJsonConverter::fromJson(rFinderJson);

  BOOST_REQUIRE(readInRootVolumeFinder.instance() != nullptr);

  auto readInIndexedDetectorVolumesImpl =
      dynamic_cast<const IndexedDetectorVolumesImpl*>(
          readInRootVolumeFinder.instance());

  BOOST_REQUIRE(readInIndexedDetectorVolumesImpl != nullptr);

  const auto& gridRead = readInIndexedDetectorVolumesImpl->grid;

  BOOST_CHECK_EQUAL(gridRead.atPosition(p11), 11u);
  BOOST_CHECK_EQUAL(gridRead.atPosition(p12), 12u);
  BOOST_CHECK_EQUAL(gridRead.atPosition(p13), 13u);
  BOOST_CHECK_EQUAL(gridRead.atPosition(p21), 21u);
  BOOST_CHECK_EQUAL(gridRead.atPosition(p22), 22u);
  BOOST_CHECK_EQUAL(gridRead.atPosition(p23), 23u);
}

BOOST_AUTO_TEST_SUITE_END()
