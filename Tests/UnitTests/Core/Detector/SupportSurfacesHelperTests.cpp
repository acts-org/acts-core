// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/detail/SupportSurfacesHelper.hpp"
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/BinningType.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>

Acts::GeometryContext tContext;

using namespace Acts::Experimental::detail::SupportSurfacesHelper;

BOOST_AUTO_TEST_SUITE(Detector)

BOOST_AUTO_TEST_CASE(CylindricalSupportCase) {
  // This tests the basic functionally to add a cylindrical support structure
  // and then split it into planar sectors

  // As a single cylinder
  // radius = 100
  // half length = 400
  // phi min = 0
  // phi max = 2pi

  Acts::Extent lExtent;
  lExtent.set(Acts::BinningValue::binR, 100., 110.);
  lExtent.set(Acts::BinningValue::binZ, -400., 400.);

  // Test creation of surface components
  CylindricalSupport csCreator{10., {1., 1.}};
  auto csComponents = csCreator(lExtent);
  auto& [csType, csValues, csTransform] = csComponents;

  BOOST_CHECK_EQUAL(csType, Acts::Surface::SurfaceType::Cylinder);
  BOOST_CHECK_EQUAL(csValues.size(), 6u);
  BOOST_CHECK_EQUAL(csValues[0u], 120.);
  BOOST_CHECK_EQUAL(csValues[1u], 399.);
  BOOST_CHECK(csTransform.isApprox(Acts::Transform3::Identity()));

  // Test a single support from Extent generation
  auto singleSupport = cylindricalSupport(csComponents);
  BOOST_CHECK_EQUAL(singleSupport.size(), 1u);
  BOOST_CHECK_EQUAL(singleSupport[0u]->type(),
                    Acts::Surface::SurfaceType::Cylinder);

  // Test a split cylinder into 32 sectors
  auto splitSupport = cylindricalSupport(csComponents, 32u);
  BOOST_CHECK_EQUAL(splitSupport.size(), 32u);
  for (const auto& ss : splitSupport) {
    BOOST_CHECK_EQUAL(ss->type(), Acts::Surface::SurfaceType::Plane);
  }

  // As a split cylinder - sectoral start cylinder
  auto splitSectoralSupport =
      Acts::Experimental::detail::SupportSurfacesHelper::cylindricalSupport(
          csComponents, 128u);
  BOOST_CHECK_EQUAL(splitSectoralSupport.size(), 128u);
  for (const auto& ss : splitSectoralSupport) {
    BOOST_CHECK_EQUAL(ss->type(), Acts::Surface::SurfaceType::Plane);
  }

  // Invalid / runtime checks
  Acts::Extent invalid;
  BOOST_CHECK_THROW(csCreator(invalid), std::invalid_argument);

  csValues = {120., 399.};
  BOOST_CHECK_THROW(cylindricalSupport(csComponents), std::invalid_argument);

  csValues = {120., 399., 0., 0., 0., 0.};
  csType = Acts::Surface::SurfaceType::Disc;
  BOOST_CHECK_THROW(cylindricalSupport(csComponents), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(DiscSupportCase) {
  // This tests the basic functionality to add a disc support structure
  // and then split it into planar sectors

  // As a single disc
  // rmin = 100
  // rmax = 400
  /// phi min = 0
  // phi max = 2pi
  Acts::Extent lExtent;
  lExtent.set(Acts::BinningValue::binR, 100., 400.);
  lExtent.set(Acts::BinningValue::binZ, -405., -395.);

  // Test creation of surface components
  DiscSupport dsCreator{0., {1., 1.}};
  auto dsComponents = dsCreator(lExtent);
  auto& [dsType, dsValues, dsTransform] = dsComponents;

  BOOST_CHECK_EQUAL(dsType, Acts::Surface::SurfaceType::Disc);
  BOOST_CHECK_EQUAL(dsValues.size(), 4u);
  BOOST_CHECK_EQUAL(dsValues[0u], 101.);
  BOOST_CHECK_EQUAL(dsValues[1u], 399.);
  BOOST_CHECK(dsTransform.translation().isApprox(Acts::Vector3(0., 0., -400.)));

  // Test as a single support surface
  auto singleSupport =
      Acts::Experimental::detail::SupportSurfacesHelper::discSupport(
          dsComponents);
  BOOST_CHECK_EQUAL(singleSupport.size(), 1u);
  BOOST_CHECK_EQUAL(singleSupport[0u]->type(),
                    Acts::Surface::SurfaceType::Disc);

  // As a split disc into 32 sectors
  auto splitSupport =
      Acts::Experimental::detail::SupportSurfacesHelper::discSupport(
          dsComponents, 32u);
  BOOST_CHECK_EQUAL(splitSupport.size(), 32u);
  for (const auto& ss : splitSupport) {
    BOOST_CHECK_EQUAL(ss->type(), Acts::Surface::SurfaceType::Plane);
  }

  // As a split disc - sectoral start disc
  auto splitSectoralSupport =
      Acts::Experimental::detail::SupportSurfacesHelper::discSupport(
          dsComponents, 16u);
  BOOST_CHECK_EQUAL(splitSectoralSupport.size(), 16u);
  for (const auto& ss : splitSectoralSupport) {
    BOOST_CHECK_EQUAL(ss->type(), Acts::Surface::SurfaceType::Plane);
  }

  // Invalid / runtime checks
  Acts::Extent invalid;
  BOOST_CHECK_THROW(dsCreator(invalid), std::invalid_argument);

  dsValues = {120., 399.};
  BOOST_CHECK_THROW(cylindricalSupport(dsComponents), std::invalid_argument);

  dsValues = {120., 399., M_PI, 0.};
  dsType = Acts::Surface::SurfaceType::Cylinder;
  BOOST_CHECK_THROW(cylindricalSupport(dsComponents), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(RectangularSupportCase) {
  // This tests the basic functionality to add a planar, rectangular support
  // structure

  // As a plane extent in z
  // dx = [-100,100]
  // dy = [-200,200]
  // dz = [-50, -60]
  Acts::Extent lExtent;
  lExtent.set(Acts::BinningValue::binX, -100., 100.);
  lExtent.set(Acts::BinningValue::binY, -200., 200.);
  lExtent.set(Acts::BinningValue::binZ, -60., -50.);

  // place in Z with offset 2_mm
  // Asymmetric clearances in x an y for testing
  RectangularSupport rsCreator{
      Acts::BinningValue::binZ, 2., {1., 2.}, {3., 4.}};
  auto rsComponents = rsCreator(lExtent);
  auto& [rsType, rsValues, rsTransform] = rsComponents;

  BOOST_CHECK_EQUAL(rsType, Acts::Surface::SurfaceType::Plane);
  BOOST_CHECK_EQUAL(rsValues.size(), 4u);
  BOOST_CHECK_EQUAL(rsValues[0u], -99.);
  BOOST_CHECK_EQUAL(rsValues[1u], -197.);
  BOOST_CHECK_EQUAL(rsValues[2u], 98.);
  BOOST_CHECK_EQUAL(rsValues[3u], 196.);

  BOOST_CHECK(rsTransform.translation().isApprox(Acts::Vector3(0., 0., -53.)));

  // Test the support surface creation
  auto singleSupport =
      Acts::Experimental::detail::SupportSurfacesHelper::rectangularSupport(
          rsComponents);
  BOOST_CHECK_EQUAL(singleSupport.size(), 1u);
  BOOST_CHECK_EQUAL(singleSupport[0u]->type(),
                    Acts::Surface::SurfaceType::Plane);

  // Invalid / runtime checks
  Acts::Extent invalid;
  invalid.set(Acts::BinningValue::binX, -100., 100.);
  invalid.set(Acts::BinningValue::binY, -200., 200.);
  BOOST_CHECK_THROW(rsCreator(invalid), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(addCylinderSupportCase) {
  // This tests the functionally to take the surfaces from a cylinder layer,
  // estimate their extend and use this to construct a support structure
  // with some given additional instructuions
  std::vector<std::shared_ptr<Acts::Surface>> lSurfaces;
  std::vector<std::size_t> assignToAll;

  // The Extent - estimated by surfaces and other constraints
  // In this example we assume that e.g. the surfaces were parsed
  // -> did yield and extend of 100 < r 110
  // The volume would give an extend of -400 < z < 400
  Acts::Extent lExtent;
  lExtent.set(Acts::BinningValue::binR, 100., 110.);
  lExtent.set(Acts::BinningValue::binZ, -400., 400.);

  // Cylinder support
  CylindricalSupport csCreator{10., {1., 1.}};

  // Add a single support cylinder
  Acts::Experimental::detail::SupportSurfacesHelper::addSupport(
      lSurfaces, assignToAll, lExtent, csCreator, 1u);

  BOOST_CHECK_EQUAL(lSurfaces.size(), 1u);
  BOOST_CHECK_EQUAL(lSurfaces[0u]->type(),
                    Acts::Surface::SurfaceType::Cylinder);
  BOOST_CHECK_EQUAL(assignToAll.size(), 1u);
  BOOST_CHECK_EQUAL(assignToAll[0u], 0u);

  // The radius of the support surface should be 10 out of the maximum
  CHECK_CLOSE_ABS(lSurfaces[0u]->bounds().values()[0u], 120, 1e-3);
  CHECK_CLOSE_ABS(lSurfaces[0u]->bounds().values()[1u], 399, 1e-3);

  // Clear up for the next test
  lSurfaces.clear();
  assignToAll.clear();

  // Add split surfaces as support to already existing surfaces
  Acts::Experimental::detail::SupportSurfacesHelper::addSupport(
      lSurfaces, assignToAll, lExtent, csCreator, 16u);
  BOOST_CHECK_EQUAL(lSurfaces.size(), 16u);
  BOOST_CHECK(assignToAll.empty());
}

BOOST_AUTO_TEST_CASE(addDiscSupportCase) {
  // This tests the functionally to take the surfaces from a disc layer,
  // estimate their extend and use this to construct a support structure
  // with some given additional instructuions
  std::vector<std::shared_ptr<Acts::Surface>> lSurfaces;
  std::vector<std::size_t> assignToAll;

  // The Extent
  Acts::Extent lExtent;
  lExtent.set(Acts::BinningValue::binR, 100., 400.);
  lExtent.set(Acts::BinningValue::binZ, -110., -100.);

  // Disc support: this would set the disc at the center
  DiscSupport dsCreator{0., {1., 1.}};

  // Add a single disc as a support surface
  Acts::Experimental::detail::SupportSurfacesHelper::addSupport(
      lSurfaces, assignToAll, lExtent, dsCreator, 1u);
  BOOST_CHECK_EQUAL(lSurfaces.size(), 1u);
  BOOST_CHECK_EQUAL(lSurfaces[0u]->type(), Acts::Surface::SurfaceType::Disc);
  BOOST_CHECK_EQUAL(assignToAll.size(), 1u);
  BOOST_CHECK_EQUAL(assignToAll[0u], 0u);

  // The position of the support surface should be at zenter z
  CHECK_CLOSE_ABS(lSurfaces[0u]->transform(tContext).translation().z(), -105,
                  1e-3);
  CHECK_CLOSE_ABS(lSurfaces[0u]->bounds().values()[0u], 101, 1e-3);
  CHECK_CLOSE_ABS(lSurfaces[0u]->bounds().values()[1u], 399, 1e-3);

  // Clear up for the next test
  lSurfaces.clear();
  assignToAll.clear();
  // Add split surfaces as support disc
  Acts::Experimental::detail::SupportSurfacesHelper::addSupport(
      lSurfaces, assignToAll, lExtent, dsCreator, 16u);
  BOOST_CHECK_EQUAL(lSurfaces.size(), 16u);
  BOOST_CHECK(assignToAll.empty());
}

BOOST_AUTO_TEST_CASE(addRectangularSupportCase) {
  // This tests the functionally to take the surfaces from a plane layer,
  // estimate their extend and use this to construct a support structure
  // with some given additional instructuions
  std::vector<std::shared_ptr<Acts::Surface>> lSurfaces;
  std::vector<std::size_t> assignToAll;

  // As a plane extent in z
  // dx = [-100,100]
  // dy = [-200,200]
  // dz = [-50, -60]
  Acts::Extent lExtent;
  lExtent.set(Acts::BinningValue::binX, -100., 100.);
  lExtent.set(Acts::BinningValue::binY, -200., 200.);
  lExtent.set(Acts::BinningValue::binZ, -60., -50.);

  // place in Z with offset 2_mm
  // Asymmetric clearances in x an y for testing
  RectangularSupport rsCreator{
      Acts::BinningValue::binZ, 2., {1., 2.}, {3., 4.}};

  // Add a single disc as a support surface
  Acts::Experimental::detail::SupportSurfacesHelper::addSupport(
      lSurfaces, assignToAll, lExtent, rsCreator);

  BOOST_CHECK_EQUAL(lSurfaces.size(), 1u);
  BOOST_CHECK_EQUAL(lSurfaces[0u]->type(), Acts::Surface::SurfaceType::Plane);
  BOOST_CHECK_EQUAL(assignToAll.size(), 1u);
  BOOST_CHECK_EQUAL(assignToAll[0u], 0u);

  // The position of the support surface should be z with offset
  CHECK_CLOSE_ABS(lSurfaces[0u]->transform(tContext).translation().z(), -53,
                  1e-3);
  // The bounds should be as given (including clearances)
  CHECK_CLOSE_ABS(lSurfaces[0u]->bounds().values()[0u], -99, 1e-3);
  CHECK_CLOSE_ABS(lSurfaces[0u]->bounds().values()[1u], -197, 1e-3);
  CHECK_CLOSE_ABS(lSurfaces[0u]->bounds().values()[2u], 98, 1e-3);
  CHECK_CLOSE_ABS(lSurfaces[0u]->bounds().values()[3u], 196, 1e-3);
}

BOOST_AUTO_TEST_CASE(addMisconfiguredSupportCase) {
  std::vector<std::shared_ptr<Acts::Surface>> lSurfaces;
  std::vector<std::size_t> assignToAll;

  // Unconstrainted extent
  Acts::Extent lExtent;

  // Cylinder support
  CylindricalSupport csCreator{10., {1., 1.}};

  // R - Z are not constrained
  // Add a single disc as a support cylinder
  BOOST_CHECK_THROW(
      Acts::Experimental::detail::SupportSurfacesHelper::addSupport(
          lSurfaces, assignToAll, lExtent, csCreator, 1u),
      std::invalid_argument);

  // The Extent
  lExtent.set(Acts::BinningValue::binR, 100., 400.);
  lExtent.set(Acts::BinningValue::binZ, -110., -100.);

  // Wrong surface type
  struct InvalidCreator {
    auto operator()(const Acts::Extent& /*e*/) const {
      return std::make_tuple(Acts::Surface::SurfaceType::Perigee,
                             std::vector<Acts::ActsScalar>{},
                             Acts::Transform3::Identity());
    }
  };

  // Add a single disc as a support cylinder
  BOOST_CHECK_THROW(
      Acts::Experimental::detail::SupportSurfacesHelper::addSupport(
          lSurfaces, assignToAll, lExtent, InvalidCreator{}, 1u),
      std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()
