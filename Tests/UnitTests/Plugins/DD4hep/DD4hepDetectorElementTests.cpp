// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Plugins/DD4hep/DD4hepDetectorElement.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"

#include <fstream>
#include <iostream>

#include <DD4hep/DetElement.h>
#include <DD4hep/Detector.h>

#include "DD4hep/DetFactoryHelper.h"
#include "XML/Utilities.h"
#include "XMLFragments.hpp"

Acts::GeometryContext tContext;

const char* cylinder_xml =
    R""""(
    <detectors>
        <detector id="1" name="Cylinder" type="Cylinder">
            <type_flags type="DetType_TRACKER"/>
            <tubs name="Cylinder" rmin="10*cm" rmax="11*cm" dz="100*cm" material="Vacuum" sensitive="true"/>
        </detector>
    </detectors>
)"""";

const char* sectoral_cylinder_xml =
    R""""(
    <detectors>
        <detector id="1" name="SectoralCylinder" type="Cylinder">
            <type_flags type="DetType_TRACKER"/>
            <tubs name="Cylinder" rmin="10*cm" rmax="11*cm" dz="100*cm" phimin="0.1*rad" phimax="0.8*rad" material="Vacuum" sensitive="true"/>
        </detector>
    </detectors>
)"""";

const char* disc_xml =
    R""""(
    <detectors>
        <detector id="1" name="Disc" type="Disc">
            <type_flags type="DetType_TRACKER"/>
            <tubs name="Disc" rmin="10*cm" rmax="90*cm" dz="1*cm" material="Vacuum" sensitive="true"/>
        </detector>
    </detectors>
)"""";

const char* sectoral_disc_xml =
    R""""(
    <detectors>
        <detector id="1" name="SectoralDisc" type="Disc">
            <type_flags type="DetType_TRACKER"/>
            <tubs name="Disc" rmin="10*cm" rmax="90*cm" dz="1*cm" phimin="0.*rad" phimax="1.5*rad" material="Vacuum" sensitive="true"/>
        </detector>
    </detectors>
)"""";

const char* rectangle_xml =
    R""""(
    <detectors>
        <detector id="1" name="Rectangle" type="Rectangle">
            <type_flags type="DetType_TRACKER"/>
            <box name="Rectangle" dx="10*cm" dy="90*cm" dz="0.1*cm" cx="1.*cm" cy="2.*cm" cz="3.*cm" material="Vacuum" sensitive="true"/>
        </detector>
    </detectors>
)"""";

const char* trapezoid_xml =
    R""""(
    <detectors>
        <detector id="1" name="Trapezoid" type="Trapezoid">
            <type_flags type="DetType_TRACKER"/>
            <trap name="Trapezoid" x1="10*cm" x2="20*cm" dy="30*cm" dz="0.1*cm" cx="2.*cm" cy="3.*cm" cz="4.*cm" material="Vacuum" sensitive="true"/>
        </detector>
    </detectors>
)"""";

BOOST_AUTO_TEST_SUITE(DD4hepPlugin)

BOOST_AUTO_TEST_CASE(DD4hepPluginDetectorElementCylinder) {
  std::ofstream cxml;
  cxml.open("Cylinder.xml");
  cxml << head_xml;
  cxml << cylinder_xml;
  cxml << end_xml;
  cxml.close();

  auto lcdd = &(dd4hep::Detector::getInstance());
  lcdd->fromCompact("Cylinder.xml");
  lcdd->volumeManager();
  lcdd->apply("DD4hepVolumeManager", 0, nullptr);

  auto world = lcdd->world();

  std::shared_ptr<Acts::DD4hepDetectorElement> cylindricalElement = nullptr;
  for (const auto& [chn, child] : world.children()) {
    cylindricalElement =
        std::make_shared<Acts::DD4hepDetectorElement>(child, "XYZ", 10.);
  }

  BOOST_REQUIRE_NE(cylindricalElement, nullptr);

  const auto& surface = cylindricalElement->surface();
  BOOST_CHECK_EQUAL(surface.type(), Acts::Surface::SurfaceType::Cylinder);
  BOOST_CHECK(
      surface.transform(tContext).isApprox(Acts::Transform3::Identity()));
  auto boundValues = surface.bounds().values();
  CHECK_CLOSE_ABS(boundValues[0u], 105., 1e-10);
  CHECK_CLOSE_ABS(boundValues[1u], 1000., 1e-10);
  lcdd->destroyInstance();
}

BOOST_AUTO_TEST_CASE(DD4hepPluginDetectorElementSectoralCylinder) {
  std::ofstream cxml;
  cxml.open("SectoralCylinder.xml");
  cxml << head_xml;
  cxml << sectoral_cylinder_xml;
  cxml << end_xml;
  cxml.close();

  auto lcdd = &(dd4hep::Detector::getInstance());
  lcdd->fromCompact("SectoralCylinder.xml");
  lcdd->volumeManager();
  lcdd->apply("DD4hepVolumeManager", 0, nullptr);

  auto world = lcdd->world();

  std::shared_ptr<Acts::DD4hepDetectorElement> cylindricalElement = nullptr;
  for (const auto& [chn, child] : world.children()) {
    cylindricalElement =
        std::make_shared<Acts::DD4hepDetectorElement>(child, "XYZ", 10.);
  }

  BOOST_REQUIRE_NE(cylindricalElement, nullptr);

  const auto& surface = cylindricalElement->surface();
  BOOST_CHECK_EQUAL(surface.type(), Acts::Surface::SurfaceType::Cylinder);
  BOOST_CHECK(
      surface.transform(tContext).isApprox(Acts::Transform3::Identity()));
  auto boundValues = surface.bounds().values();
  CHECK_CLOSE_ABS(boundValues[0u], 105., 1e-10);
  CHECK_CLOSE_ABS(boundValues[1u], 1000., 1e-10);
  CHECK_CLOSE_ABS(boundValues[2u], 0.35, 1e-10);
  CHECK_CLOSE_ABS(boundValues[3u], 0.45, 1e-10);
  lcdd->destroyInstance();
}

BOOST_AUTO_TEST_CASE(DD4hepPluginDetectorElementDisc) {
  std::ofstream cxml;
  cxml.open("Disc.xml");
  cxml << head_xml;
  cxml << disc_xml;
  cxml << end_xml;
  cxml.close();

  auto lcdd = &(dd4hep::Detector::getInstance());
  lcdd->fromCompact("Disc.xml");
  lcdd->volumeManager();
  lcdd->apply("DD4hepVolumeManager", 0, nullptr);

  auto world = lcdd->world();

  std::shared_ptr<Acts::DD4hepDetectorElement> discElement = nullptr;
  for (const auto& [chn, child] : world.children()) {
    discElement =
        std::make_shared<Acts::DD4hepDetectorElement>(child, "XYZ", 10., true);
  }

  BOOST_REQUIRE_NE(discElement, nullptr);

  const auto& surface = discElement->surface();
  BOOST_CHECK_EQUAL(surface.type(), Acts::Surface::SurfaceType::Disc);
  BOOST_CHECK(
      surface.transform(tContext).isApprox(Acts::Transform3::Identity()));
  auto boundValues = surface.bounds().values();
  CHECK_CLOSE_ABS(boundValues[0u], 100., 1e-10);
  CHECK_CLOSE_ABS(boundValues[1u], 900., 1e-10);
  lcdd->destroyInstance();
}

BOOST_AUTO_TEST_CASE(DD4hepPluginDetectorElementSectoralDisc) {
  std::ofstream cxml;
  cxml.open("SectoralDisc.xml");
  cxml << head_xml;
  cxml << sectoral_disc_xml;
  cxml << end_xml;
  cxml.close();

  auto lcdd = &(dd4hep::Detector::getInstance());
  lcdd->fromCompact("SectoralDisc.xml");
  lcdd->volumeManager();
  lcdd->apply("DD4hepVolumeManager", 0, nullptr);

  auto world = lcdd->world();

  std::shared_ptr<Acts::DD4hepDetectorElement> discElement = nullptr;
  for (const auto& [chn, child] : world.children()) {
    discElement =
        std::make_shared<Acts::DD4hepDetectorElement>(child, "XYZ", 10., true);
  }

  BOOST_REQUIRE_NE(discElement, nullptr);

  const auto& surface = discElement->surface();
  BOOST_CHECK_EQUAL(surface.type(), Acts::Surface::SurfaceType::Disc);
  BOOST_CHECK(
      surface.transform(tContext).isApprox(Acts::Transform3::Identity()));
  auto boundValues = surface.bounds().values();

  CHECK_CLOSE_ABS(boundValues[0u], 100., 1e-10);
  CHECK_CLOSE_ABS(boundValues[1u], 900., 1e-10);
  CHECK_CLOSE_ABS(boundValues[2u], 0.75, 1e-10);
  CHECK_CLOSE_ABS(boundValues[3u], 0.75, 1e-10);
  lcdd->destroyInstance();
}

BOOST_AUTO_TEST_CASE(DD4hepPluginDetectorElementRectangle) {
  std::ofstream cxml;
  cxml.open("Rectangle.xml");
  cxml << head_xml;
  cxml << rectangle_xml;
  cxml << end_xml;
  cxml.close();

  auto lcdd = &(dd4hep::Detector::getInstance());
  lcdd->fromCompact("Rectangle.xml");
  lcdd->volumeManager();
  lcdd->apply("DD4hepVolumeManager", 0, nullptr);

  auto world = lcdd->world();

  std::shared_ptr<Acts::DD4hepDetectorElement> rectangleElement = nullptr;
  for (const auto& [chn, child] : world.children()) {
    rectangleElement =
        std::make_shared<Acts::DD4hepDetectorElement>(child, "XYZ", 10., true);
  }

  BOOST_REQUIRE_NE(rectangleElement, nullptr);

  const auto& surface = rectangleElement->surface();
  BOOST_CHECK_EQUAL(surface.type(), Acts::Surface::SurfaceType::Plane);

  auto sTransform = surface.transform(tContext);
  BOOST_CHECK(sTransform.translation().isApprox(Acts::Vector3(10., 20., 30.)));

  const auto& sBounds = surface.bounds();
  BOOST_CHECK_EQUAL(sBounds.type(),
                    Acts::SurfaceBounds::BoundsType::eRectangle);

  auto boundValues = sBounds.values();

  CHECK_CLOSE_ABS(boundValues[0u], -50., 1e-10);
  CHECK_CLOSE_ABS(boundValues[1u], -450., 1e-10);
  CHECK_CLOSE_ABS(boundValues[2u], 50, 1e-10);
  CHECK_CLOSE_ABS(boundValues[3u], 450, 1e-10);

  lcdd->destroyInstance();
}

BOOST_AUTO_TEST_CASE(DD4hepPluginDetectorElementTrapezoid) {
  std::ofstream cxml;
  cxml.open("Trapezoid.xml");
  cxml << head_xml;
  cxml << trapezoid_xml;
  cxml << end_xml;
  cxml.close();

  auto lcdd = &(dd4hep::Detector::getInstance());
  lcdd->fromCompact("Trapezoid.xml");
  lcdd->volumeManager();
  lcdd->apply("DD4hepVolumeManager", 0, nullptr);

  auto world = lcdd->world();

  std::shared_ptr<Acts::DD4hepDetectorElement> trapezoidElement = nullptr;
  for (const auto& [chn, child] : world.children()) {
    trapezoidElement =
        std::make_shared<Acts::DD4hepDetectorElement>(child, "xZ", 10., true);
  }

  BOOST_REQUIRE_NE(trapezoidElement, nullptr);

  const auto& surface = trapezoidElement->surface();
  BOOST_CHECK_EQUAL(surface.type(), Acts::Surface::SurfaceType::Plane);

  auto sTransform = surface.transform(tContext);
  BOOST_CHECK(sTransform.translation().isApprox(Acts::Vector3(20., 30., 40.)));

  const auto& sBounds = surface.bounds();
  BOOST_CHECK_EQUAL(sBounds.type(),
                    Acts::SurfaceBounds::BoundsType::eTrapezoid);

  auto boundValues = sBounds.values();
  CHECK_CLOSE_ABS(boundValues[0u], 100., 1e-10);
  CHECK_CLOSE_ABS(boundValues[1u], 200., 1e-10);
  CHECK_CLOSE_ABS(boundValues[2u], 150, 1e-10);

  lcdd->destroyInstance();
}

BOOST_AUTO_TEST_SUITE_END()
