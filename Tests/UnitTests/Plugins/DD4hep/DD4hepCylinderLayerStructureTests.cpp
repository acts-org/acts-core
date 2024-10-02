// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Plugins/DD4hep/DD4hepDetectorSurfaceFactory.hpp"
#include "Acts/Plugins/DD4hep/DD4hepLayerStructure.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Tests/CommonHelpers/CylindricalTrackingGeometry.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <fstream>
#include <string>

#include <DD4hep/DetElement.h>
#include <DD4hep/DetFactoryHelper.h>
#include <DD4hep/Detector.h>
#include <XML/Utilities.h>
#include <XMLFragments.hpp>

#include "DD4hepTestsHelper.hpp"

Acts::GeometryContext tContext;
Acts::Test::CylindricalTrackingGeometry cGeometry =
    Acts::Test::CylindricalTrackingGeometry(tContext);

const char* beampipe_head_xml =
    R""""(
    <detectors>
        <detector id="0" name="BeamPipe" type="BarrelDetector">
            <type_flags type="DetType_TRACKER + DetType_BEAMPIPE"/>
            <layers>
                <layer name="BP" id="0">
)"""";

const char* cylinder_layer_head_xml =
    R""""(
    <detectors>
        <detector id="1" name="BarrelLayer" type="BarrelDetector" readout="PixelReadout">
            <type_flags type="DetType_TRACKER + DetType_BARREL"/>
            <layers>
                <layer name="B0" id="0">
)"""";

const char* tail_xml =
    R""""(
                </layer>
            </layers>
        </detector>
    </detectors>
)"""";

const char* indent_12_xml = "            ";

BOOST_AUTO_TEST_SUITE(DD4hepPlugin)

// This tests creates a beampipe as a passive cylinder surface
BOOST_AUTO_TEST_CASE(DD4hepPluginBeampipeStructure) {
  // Create an XML from it
  std::ofstream cxml;

  std::string fNameBase = "BeamPipe";

  cxml.open(fNameBase + ".xml");
  cxml << head_xml;
  cxml << beampipe_head_xml;
  cxml << indent_12_xml << "  <acts_passive_surface>" << '\n';
  cxml << indent_12_xml
       << "    <tubs rmin=\"25*mm\" rmax=\"25.8*mm\" dz=\"1800*mm\" "
          "material=\"Air\"/>"
       << '\n';
  cxml << indent_12_xml << "  </acts_passive_surface>" << '\n';

  cxml << tail_xml;
  cxml << end_xml;
  cxml.close();

  auto lcdd = &(dd4hep::Detector::getInstance());
  lcdd->fromCompact(fNameBase + ".xml");
  lcdd->volumeManager();
  lcdd->apply("DD4hepVolumeManager", 0, nullptr);

  auto world = lcdd->world();

  // Now the test starts ...
  auto sFactory = std::make_shared<Acts::DD4hepDetectorSurfaceFactory>(
      Acts::getDefaultLogger("DD4hepDetectorSurfaceFactory",
                             Acts::Logging::DEBUG));

  Acts::Experimental::DD4hepLayerStructure beamPipeStructure(
      std::move(sFactory), Acts::getDefaultLogger("DD4hepBeamPipeStructure",
                                                  Acts::Logging::VERBOSE));

  Acts::DD4hepDetectorElement::Store dd4hepStore;

  Acts::Experimental::DD4hepLayerStructure::Options lsOptions;
  lsOptions.name = "BeamPipe";
  lsOptions.logLevel = Acts::Logging::VERBOSE;

  auto [beamPipeInternalsBuilder, beamPipeExt] =
      beamPipeStructure.builder(dd4hepStore, tContext, world, lsOptions);

  // Not configured to have the beam pipe Extent measured
  BOOST_CHECK(beamPipeExt == std::nullopt);

  // Build the internal volume structure
  auto [surfaces, volumes, surfacesUpdater, volumeUpdater] =
      beamPipeInternalsBuilder->construct(tContext);

  // All surfaces are filled
  BOOST_CHECK_EQUAL(surfaces.size(), 1u);
  // No volumes are added
  BOOST_CHECK(volumes.empty());
  // The surface updator is connected
  BOOST_CHECK(surfacesUpdater.connected());
  // The volume updator is connected
  BOOST_CHECK(volumeUpdater.connected());

  // Kill that instance before going into the next test
  lcdd->destroyInstance();
}

// This test creates DD4hep xml compact files for cylindrical
// layer structures and then converts them into IndexedSurfacesUpdater
// and other additional components needed for Internal DetectorVolume
// structure.
//
// It tests without explicit binning first
//
// It tests also with Bin expansion, i.e. the filling of additional
// bins with the found surface object.
//
BOOST_AUTO_TEST_CASE(DD4hepPluginCylinderLayerStructure) {
  // First create some test surfaces
  Acts::Test::CylindricalTrackingGeometry::DetectorStore dStore;
  auto cSurfaces = cGeometry.surfacesCylinder(dStore, 8.4, 36., 0.15, 0.145,
                                              116., 3., 2., {52, 14});

  // Running three tests with
  // - no binning / no support
  // - 14 x 52 bins and expansion / support
  // - 28 x 104 bins without expansion /  support with proto material
  std::vector<std::array<unsigned int, 4u> > zphiBinning = {
      {1u, 1u, 0u, 0u}, {14u, 52u, 1u, 1u}, {28u, 104u, 0u, 0u}};

  std::size_t itest = 0;
  for (auto [nz, nphi, ez, ephi] : zphiBinning) {
    // Create an XML from it
    std::ofstream cxml;

    std::string fNameBase = "CylinderLayer_nz";
    fNameBase += std::to_string(nz);
    fNameBase += "_nphi";
    fNameBase += std::to_string(nphi);

    cxml.open(fNameBase + ".xml");
    cxml << head_xml;
    cxml << segmentation_xml;
    cxml << cylinder_layer_head_xml;

    // Test binning
    if (nz * nphi > 1u) {
      cxml << indent_12_xml << "<acts_surface_binning";
      cxml << " ztype=\"equidistant\"";
      cxml << " phitype=\"equidistant\"";
      cxml << " nz=\"" << nz << "\" zmin=\"-500*mm\" zmax=\"500*mm\"";
      cxml << " zexpansion= \"" << ez << "\"";
      cxml << " nphi=\"" << nphi << "\"  phimin=\"-3.1415\" phimax=\"3.1415\"";
      cxml << " phiexpansion= \"" << ephi << "\"/>";
    }
    cxml << "<modules>" << '\n';

    for (const auto& s : cSurfaces) {
      cxml << indent_12_xml
           << DD4hepTestsHelper::surfaceToXML(tContext, *s,
                                              Acts::Transform3::Identity())
           << "\n";
    }

    cxml << "</modules>" << '\n';

    // test the support structure definition
    unsigned int passiveAddon = 0u;
    if (itest == 1u) {
      cxml << indent_12_xml << "  <acts_passive_surface>" << '\n';
      cxml << indent_12_xml
           << "    <tubs rmin=\"122*mm\" rmax=\"124*mm\" dz=\"500*mm\" "
              "material=\"Air\"/>"
           << '\n';
      cxml << indent_12_xml << "  </acts_passive_surface>" << '\n';
      passiveAddon = 1u;
    }
    ++itest;

    cxml << tail_xml;
    cxml << end_xml;
    cxml.close();

    auto lcdd = &(dd4hep::Detector::getInstance());
    lcdd->fromCompact(fNameBase + ".xml");
    lcdd->volumeManager();
    lcdd->apply("DD4hepVolumeManager", 0, nullptr);

    auto world = lcdd->world();

    // Now the test starts ...
    auto sFactory = std::make_shared<Acts::DD4hepDetectorSurfaceFactory>(
        Acts::getDefaultLogger("DD4hepDetectorSurfaceFactory",
                               Acts::Logging::VERBOSE));

    Acts::Experimental::DD4hepLayerStructure barrelStructure(
        std::move(sFactory),
        Acts::getDefaultLogger("DD4hepLayerStructure", Acts::Logging::VERBOSE));

    Acts::DD4hepDetectorElement::Store dd4hepStore;

    Acts::Experimental::DD4hepLayerStructure::Options lsOptions;
    lsOptions.name = "BarrelLayer";
    lsOptions.logLevel = Acts::Logging::VERBOSE;

    auto [barrelInternalsBuilder, barrelExt] =
        barrelStructure.builder(dd4hepStore, tContext, world, lsOptions);

    // Build the internal volume structure
    auto [surfaces, volumes, surfacesUpdater, volumeUpdater] =
        barrelInternalsBuilder->construct(tContext);

    // All surfaces are filled
    BOOST_CHECK_EQUAL(surfaces.size(), 14u * 52u + passiveAddon);
    // No volumes are added
    BOOST_CHECK(volumes.empty());
    // The surface updator is connected
    BOOST_CHECK(surfacesUpdater.connected());
    // The volume updator is connected
    BOOST_CHECK(volumeUpdater.connected());

    // Kill that instance before going into the next test
    lcdd->destroyInstance();
  }
}

// Test the auto-range determination
BOOST_AUTO_TEST_CASE(DD4hepPluginCylinderLayerStructureAutoRange) {
  // First create some test surfaces
  Acts::Test::CylindricalTrackingGeometry::DetectorStore dStore;
  auto cSurfaces = cGeometry.surfacesCylinder(dStore, 8.4, 36., 0.15, 0.145,
                                              116., 3., 2., {52, 14});

  // Create an XML from it
  std::ofstream cxml;
  std::string fName = "CylinderLayer_auto_range.xml";

  cxml.open(fName);
  cxml << head_xml;
  cxml << segmentation_xml;
  cxml << cylinder_layer_head_xml;

  cxml << "<modules>" << '\n';
  for (const auto& s : cSurfaces) {
    cxml << indent_12_xml
         << DD4hepTestsHelper::surfaceToXML(tContext, *s,
                                            Acts::Transform3::Identity())
         << "\n";
  }
  cxml << "</modules>" << '\n';
  cxml << tail_xml;
  cxml << end_xml;
  cxml.close();

  auto lcdd = &(dd4hep::Detector::getInstance());
  lcdd->fromCompact(fName);
  lcdd->volumeManager();
  lcdd->apply("DD4hepVolumeManager", 0, nullptr);

  auto world = lcdd->world();

  // Now the test starts ...
  auto sFactory = std::make_shared<Acts::DD4hepDetectorSurfaceFactory>(
      Acts::getDefaultLogger("DD4hepDetectorSurfaceFactory",
                             Acts::Logging::VERBOSE));

  Acts::Experimental::DD4hepLayerStructure barrelStructure(
      std::move(sFactory),
      Acts::getDefaultLogger("DD4hepLayerStructure", Acts::Logging::VERBOSE));

  Acts::DD4hepDetectorElement::Store dd4hepStore;

  Acts::Experimental::DD4hepLayerStructure::Options lsOptions;
  lsOptions.name = "AutoRangeLayer";
  auto extent = Acts::Extent();
  lsOptions.extent = extent;
  lsOptions.extentConstraints = {Acts::BinningValue::binZ,
                                 Acts::BinningValue::binR};
  lsOptions.logLevel = Acts::Logging::VERBOSE;

  auto [barrelInternalsBuilder, barrelExt] =
      barrelStructure.builder(dd4hepStore, tContext, world, lsOptions);

  BOOST_CHECK(barrelExt != std::nullopt);
  BOOST_CHECK(barrelExt.value().constrains(Acts::BinningValue::binZ));
  BOOST_CHECK(barrelExt.value().constrains(Acts::BinningValue::binR));
}

BOOST_AUTO_TEST_SUITE_END()
