// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Plugins/TGeo/TGeoSurfaceConverter.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Visualization/GeometryView3D.hpp"
#include "Acts/Visualization/ObjVisualization3D.hpp"
#include "Acts/Visualization/ViewConfig.hpp"

#include <cmath>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "TGeoManager.h"
#include "TGeoMaterial.h"
#include "TGeoMatrix.h"
#include "TGeoMedium.h"
#include "TGeoTube.h"
#include "TGeoVolume.h"
#include "TView.h"

namespace Acts::Test {

GeometryContext tgContext = GeometryContext();

ViewConfig red{.color = {200, 0, 0}};
ViewConfig green{.color = {0, 200, 0}};
ViewConfig blue{.color = {0, 0, 200}};

std::vector<std::string> allowedAxes = {"XY*", "Xy*", "xy*", "xY*",
                                        "YX*", "yx*", "yX*", "Yx*"};

std::vector<std::string> notAllowedAxes = {"YZ*", "ZX*", "ZY*"};

/// @brief Unit test to convert a TGeoTube into a CylinderSurface
///
/// * The TGeoTrd1 can only have (x/X)(y/Y) orientation
BOOST_AUTO_TEST_CASE(TGeoTube_to_CylinderSurface) {
  ObjVisualization3D objVis;

  double rmin = 10.;
  double rmax = 11;
  double hz = 40.;
  double phimin = 45.;
  double phimax = -45.;

  new TGeoManager("trd1", "poza9");
  TGeoMaterial *mat = new TGeoMaterial("Al", 26.98, 13, 2.7);
  TGeoMedium *med = new TGeoMedium("MED", 1, mat);
  TGeoVolume *top = gGeoManager->MakeBox("TOP", med, 100, 100, 100);
  gGeoManager->SetTopVolume(top);
  TGeoVolume *vol = gGeoManager->MakeTube("Tube", med, rmin, rmax, hz);
  TGeoVolume *vols =
      gGeoManager->MakeTubs("Tube", med, rmin, rmax, hz, phimin, phimax);
  gGeoManager->CloseGeometry();

  std::size_t icyl = 0;
  for (const auto &axes : allowedAxes) {
    auto [cylinder, thickness] = TGeoSurfaceConverter::toSurface(
        *vol->GetShape(), *gGeoIdentity, axes, 1);
    BOOST_REQUIRE_NE(cylinder, nullptr);
    BOOST_CHECK_EQUAL(cylinder->type(), Surface::Cylinder);
    CHECK_CLOSE_ABS(thickness, rmax - rmin, s_epsilon);

    auto bounds = dynamic_cast<const CylinderBounds *>(&(cylinder->bounds()));
    BOOST_REQUIRE_NE(bounds, nullptr);
    double bR = bounds->get(CylinderBounds::eR);
    double bhZ = bounds->get(CylinderBounds::eHalfLengthZ);

    CHECK_CLOSE_ABS(bR, 10.5, s_epsilon);
    CHECK_CLOSE_ABS(bhZ, hz, s_epsilon);

    auto transform = cylinder->transform(tgContext);
    auto rotation = transform.rotation();

    // Check if the surface is the (negative) identity
    GeometryView3D::drawSurface(objVis, *cylinder, tgContext);
    const Vector3 center = cylinder->center(tgContext);
    GeometryView3D::drawArrowForward(
        objVis, center, center + 1.2 * bR * rotation.col(0), 4., 2.5, red);
    GeometryView3D::drawArrowForward(
        objVis, center, center + 1.2 * bR * rotation.col(1), 4., 2.5, green);
    GeometryView3D::drawArrowForward(
        objVis, center, center + 1.2 * bhZ * rotation.col(2), 4., 2.5, blue);

    objVis.write("TGeoConversion_TGeoTube_CylinderSurface_" +
                 std::to_string(icyl));
    objVis.clear();

    if (icyl < 2) {
      auto [cylinderSegment, cThickness] = TGeoSurfaceConverter::toSurface(
          *vols->GetShape(), *gGeoIdentity, axes, 1);
      BOOST_REQUIRE_NE(cylinderSegment, nullptr);
      BOOST_CHECK_EQUAL(cylinderSegment->type(), Surface::Cylinder);
      CHECK_CLOSE_ABS(cThickness, rmax - rmin, s_epsilon);

      auto boundsSegment =
          dynamic_cast<const CylinderBounds *>(&(cylinderSegment->bounds()));
      BOOST_REQUIRE_NE(boundsSegment, nullptr);
      bR = boundsSegment->get(CylinderBounds::eR);
      bhZ = boundsSegment->get(CylinderBounds::eHalfLengthZ);
      double hphi = boundsSegment->get(CylinderBounds::eHalfPhiSector);
      double mphi = boundsSegment->get(CylinderBounds::eAveragePhi);
      CHECK_CLOSE_ABS(bR, 10.5, s_epsilon);
      CHECK_CLOSE_ABS(bhZ, hz, s_epsilon);
      CHECK_CLOSE_ABS(hphi, 0.25 * M_PI, s_epsilon);
      CHECK_CLOSE_ABS(mphi, 0., s_epsilon);
      GeometryView3D::drawSurface(objVis, *cylinderSegment, tgContext);
      GeometryView3D::drawArrowForward(
          objVis, center, center + 1.2 * bR * rotation.col(0), 4., 2.5, red);
      GeometryView3D::drawArrowForward(
          objVis, center, center + 1.2 * bR * rotation.col(1), 4., 2.5, green);
      GeometryView3D::drawArrowForward(
          objVis, center, center + 1.2 * bhZ * rotation.col(2), 4., 2.5, blue);
      objVis.write("TGeoConversion_TGeoTube_CylinderSegmentSurface_" +
                   std::to_string(icyl));
      objVis.clear();
    } else {
      BOOST_CHECK_THROW(TGeoSurfaceConverter::toSurface(*vols->GetShape(),
                                                        *gGeoIdentity, axes, 1),
                        std::invalid_argument);
    }
    ++icyl;
  }

  // Check exceptions for not allowed axis definition
  for (const auto &naxes : notAllowedAxes) {
    BOOST_CHECK_THROW(TGeoSurfaceConverter::toSurface(*vol->GetShape(),
                                                      *gGeoIdentity, naxes, 1),
                      std::invalid_argument);
  }
}

/// @brief Unit test to convert a TGeoTube into a DiscSurface
///
/// * The TGeoTrd1 can only have (x/X)(y/Y) orientation
BOOST_AUTO_TEST_CASE(TGeoTube_to_DiscSurface) {
  ObjVisualization3D objVis;

  double rmin = 5.;
  double rmax = 25;
  double hz = 2.;
  double phimin = 45.;
  double phimax = -45.;

  new TGeoManager("trd1", "poza9");
  TGeoMaterial *mat = new TGeoMaterial("Al", 26.98, 13, 2.7);
  TGeoMedium *med = new TGeoMedium("MED", 1, mat);
  TGeoVolume *top = gGeoManager->MakeBox("TOP", med, 100, 100, 100);
  gGeoManager->SetTopVolume(top);
  TGeoVolume *vol = gGeoManager->MakeTube("Tube", med, rmin, rmax, hz);
  vol->SetLineWidth(2);
  TGeoVolume *vols =
      gGeoManager->MakeTubs("Tube", med, rmin, rmax, hz, phimin, phimax);
  gGeoManager->CloseGeometry();

  std::size_t idisc = 0;
  for (const auto &axes : allowedAxes) {
    auto [disc, thickness] = TGeoSurfaceConverter::toSurface(
        *vol->GetShape(), *gGeoIdentity, axes, 1);
    BOOST_REQUIRE_NE(disc, nullptr);
    BOOST_CHECK_EQUAL(disc->type(), Surface::Disc);
    CHECK_CLOSE_ABS(thickness, 2 * hz, s_epsilon);

    auto bounds = dynamic_cast<const RadialBounds *>(&(disc->bounds()));
    BOOST_REQUIRE_NE(bounds, nullptr);
    double bminr = bounds->get(RadialBounds::eMinR);
    double bmaxr = bounds->get(RadialBounds::eMaxR);

    CHECK_CLOSE_ABS(bminr, rmin, s_epsilon);
    CHECK_CLOSE_ABS(bmaxr, rmax, s_epsilon);

    // Check if the surface is the (negative) identity
    GeometryView3D::drawSurface(objVis, *disc, tgContext);
    const Vector3 center = disc->center(tgContext);
    GeometryView3D::drawArrowForward(
        objVis, center, center + 1.2 * rmax * Vector3::UnitX(), 4., 2.5, red);
    GeometryView3D::drawArrowForward(
        objVis, center, center + 1.2 * rmax * Vector3::UnitY(), 4., 2.5, green);
    GeometryView3D::drawArrowForward(
        objVis, center, center + 1.2 * hz * Vector3::UnitZ(), 4., 2.5, blue);
    objVis.write("TGeoConversion_TGeoTube_DiscSurface_" +
                 std::to_string(idisc));
    objVis.clear();

    if (idisc < 2) {
      auto [discSegment, dThickness] = TGeoSurfaceConverter::toSurface(
          *vols->GetShape(), *gGeoIdentity, axes, 1);
      BOOST_REQUIRE_NE(discSegment, nullptr);
      BOOST_CHECK_EQUAL(discSegment->type(), Surface::Disc);
      CHECK_CLOSE_ABS(dThickness, 2 * hz, s_epsilon);

      auto boundsSegment =
          dynamic_cast<const RadialBounds *>(&(discSegment->bounds()));
      BOOST_REQUIRE_NE(boundsSegment, nullptr);
      bminr = boundsSegment->get(RadialBounds::eMinR);
      bmaxr = boundsSegment->get(RadialBounds::eMaxR);
      double hphi = boundsSegment->get(RadialBounds::eHalfPhiSector);
      double mphi = boundsSegment->get(RadialBounds::eAveragePhi);
      CHECK_CLOSE_ABS(bminr, rmin, s_epsilon);
      CHECK_CLOSE_ABS(bmaxr, rmax, s_epsilon);
      CHECK_CLOSE_ABS(hphi, 0.25 * M_PI, s_epsilon);
      CHECK_CLOSE_ABS(mphi, 0., s_epsilon);
      GeometryView3D::drawSurface(objVis, *discSegment, tgContext);
      GeometryView3D::drawArrowForward(objVis, center,
                                       center + 1.2 * bmaxr * Vector3::UnitX(),
                                       4., 2.5, red);
      GeometryView3D::drawArrowForward(objVis, center,
                                       center + 1.2 * bmaxr * Vector3::UnitY(),
                                       4., 2.5, green);
      GeometryView3D::drawArrowForward(
          objVis, center, center + 1.2 * hz * Vector3::UnitZ(), 4., 2.5, blue);
      objVis.write("TGeoConversion_TGeoTube_DiscSegmentSurface_" +
                   std::to_string(idisc));
      objVis.clear();

    } else {
      BOOST_CHECK_THROW(TGeoSurfaceConverter::toSurface(*vols->GetShape(),
                                                        *gGeoIdentity, axes, 1),
                        std::invalid_argument);
    }
    ++idisc;
  }

  // Check exceptions for not allowed axis definition
  for (const auto &naxes : notAllowedAxes) {
    BOOST_CHECK_THROW(TGeoSurfaceConverter::toSurface(*vol->GetShape(),
                                                      *gGeoIdentity, naxes, 1),
                      std::invalid_argument);
  }
}

}  // namespace Acts::Test
