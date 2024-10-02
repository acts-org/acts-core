// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/KdtSurfacesProvider.hpp"
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/LayerCreator.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Tests/CommonHelpers/CylindricalTrackingGeometry.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Enumerate.hpp"
#include "Acts/Utilities/GridAxisGenerators.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

using namespace Acts;
using namespace Acts::Test;
using namespace Acts::Experimental;

GeometryContext tContext;
CylindricalTrackingGeometry cGeometry = CylindricalTrackingGeometry(tContext);

namespace {
/// Helper method that allows to use the already existing testing
/// infrastructure with the new const-correct detector design
///
std::vector<std::shared_ptr<Acts::Surface>> unpackSurfaces(
    const std::vector<const Acts::Surface*>& surfaces) {
  std::vector<std::shared_ptr<Acts::Surface>> uSurfaces;
  uSurfaces.reserve(surfaces.size());
  for (const auto& s : surfaces) {
    Surface* ncs = const_cast<Surface*>(s);
    uSurfaces.push_back(ncs->getSharedPtr());
  }
  return uSurfaces;
}

std::vector<std::shared_ptr<Acts::Surface>> pixelSurfaces(
    CylindricalTrackingGeometry::DetectorStore& dStore) {
  // The surfaces for the KDTree structure
  std::vector<std::shared_ptr<Acts::Surface>> pixelSurfaces;
  // Fill Discs
  std::vector<Acts::ActsScalar> pixelDiscs = {-800., -700., -600.,
                                              600.,  700.,  800.};
  for (const auto& z : pixelDiscs) {
    auto rSurfaces = cGeometry.surfacesRing(dStore, 6.4, 12.4, 36., 0.125, 0.,
                                            55., z, 2., 22u);
    auto urSurfaces = unpackSurfaces(rSurfaces);
    pixelSurfaces.insert(pixelSurfaces.end(), urSurfaces.begin(),
                         urSurfaces.end());
  }
  // Fill Barrels
  std::vector<Acts::ActsScalar> pixelBarrels = {32., 72., 116., 172.};
  std::vector<std::pair<int, int>> pixelBinning = {
      {16, 14}, {32, 14}, {52, 14}, {78, 14}};
  for (auto [ir, r] : enumerate(pixelBarrels)) {
    auto cSurfaces = cGeometry.surfacesCylinder(dStore, 8.4, 36., 0.15, 0.145,
                                                r, 3., 2., pixelBinning[ir]);

    auto ucSurfaces = unpackSurfaces(cSurfaces);
    pixelSurfaces.insert(pixelSurfaces.end(), ucSurfaces.begin(),
                         ucSurfaces.end());
  }
  return pixelSurfaces;
}

}  // namespace

BOOST_AUTO_TEST_SUITE(Detector)

// Test only the KDT infrastructure
BOOST_AUTO_TEST_CASE(KdtSurfacesProvider_misconfigured) {
  Acts::Extent region;
  BOOST_CHECK_THROW(
      Acts::Experimental::KdtSurfacesProvider<> end3(nullptr, region),
      std::invalid_argument);
}

// Test only the KDT infrastructure
BOOST_AUTO_TEST_CASE(KdtSurfacesProvider) {
  // Detector store
  CylindricalTrackingGeometry::DetectorStore dStore;
  auto pSurfaces = pixelSurfaces(dStore);
  // Count the number of surfacees
  std::size_t refNumber = 6u * 22u + 14u * (16u + 32u + 52u + 78u);
  BOOST_CHECK_EQUAL(pSurfaces.size(), refNumber);

  using KDTS = Acts::Experimental::KdtSurfaces<>;
  auto skdt = std::make_shared<KDTS>(
      KDTS(tContext, pSurfaces, {BinningValue::binZ, BinningValue::binR}));

  // query: Negative disc 3, it should yield 22 surfaces
  Acts::Extent regionND3;
  regionND3.set(BinningValue::binZ, -820, -780);
  regionND3.set(BinningValue::binR, 0., 200.);
  Acts::Experimental::KdtSurfacesProvider<> end3(skdt, regionND3);

  auto nd3 = end3.surfaces(tContext);
  BOOST_CHECK_EQUAL(nd3.size(), 22u);

  // query: 2nd Pixel barrel
  Acts::Extent regionB1;
  regionB1.set(BinningValue::binZ, -580, 580);
  regionB1.set(BinningValue::binR, 60., 80.);

  Acts::Experimental::KdtSurfacesProvider<> ba1(skdt, regionB1);

  auto b1 = ba1.surfaces(tContext);
  refNumber = 32u * 14u;
  BOOST_CHECK_EQUAL(b1.size(), refNumber);
}

BOOST_AUTO_TEST_SUITE_END()
