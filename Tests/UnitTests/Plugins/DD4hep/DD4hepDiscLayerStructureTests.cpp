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

const char* disc_layer_head_xml =
    R""""(
    <detectors>
        <detector id="1" name="Endcap" type="EndcapDetector" readout="PixelReadout">
            <type_flags type="DetType_TRACKER + DetType_BARREL"/>
            <layers>
)"""";

const char* tail_xml =
    R""""(
            </layers>
        </detector>
    </detectors>
)"""";

const char* indent_12_xml = "            ";

BOOST_AUTO_TEST_SUITE(DD4hepPlugin)

BOOST_AUTO_TEST_CASE(DD4hepDiscLayerStructure) {
  Acts::ActsScalar rZ = 400.;

  // First create some test surfaces
  Acts::Test::CylindricalTrackingGeometry::DetectorStore dStore;
  auto rSurfacesR0 = cGeometry.surfacesRing(dStore, 6.4, 12.4, 18., 0.125, 0.,
                                            42., rZ, 2., 22u);

  auto rSurfacesR1 = cGeometry.surfacesRing(dStore, 12.4, 20.4, 30., 0.125, 0.,
                                            80., rZ, 2., 22u);

  std::vector<std::vector<const Acts::Surface*>> rSurfaces = {rSurfacesR0,
                                                              rSurfacesR1};

  // Running three tests with
  // - no binning / no support
  // - 22 x 2 bins and expansion / support
  // - 44 x 1 bins without expansion /  support with proto material
  std::vector<std::array<unsigned int, 4u>> rphiBinning = {
      {1u, 1u, 0u, 0u}, {2u, 22u, 1u, 1u}, {1u, 44u, 0u, 0u}};

  std::size_t itest = 0;
  for (auto [nr, nphi, er, ephi] : rphiBinning) {
    // Create an XML from it
    std::ofstream cxml;

    std::string fNameBase = "DiscLayer_nr";
    fNameBase += std::to_string(nr);
    fNameBase += "_nphi";
    fNameBase += std::to_string(nphi);

    cxml.open(fNameBase + ".xml");
    cxml << head_xml;
    cxml << segmentation_xml;
    cxml << disc_layer_head_xml;
    cxml << "<layer name=\"E0\" id=\"0\">" << '\n';

    if (nr * nphi > 1u) {
      cxml << indent_12_xml << "<acts_surface_binning";
      cxml << " rtype=\"equidistant\"";
      cxml << " phitype=\"equidistant\"";
      cxml << " nr=\"" << nr << "\" rmin=\"20*mm\" rmax=\"120*mm\"";
      cxml << " rexpansion= \"" << er << "\"";
      cxml << " nphi=\"" << nphi << "\"  phimin=\"-3.1415\" phimax=\"3.1415\"";
      cxml << " phiexpansion= \"" << ephi << "\"/>";
    }
    cxml << "<modules>" << '\n';

    for (const auto& ring : rSurfaces) {
      for (const auto& s : ring) {
        cxml << indent_12_xml
             << DD4hepTestsHelper::surfaceToXML(tContext, *s,
                                                Acts::Transform3::Identity())
             << "\n";
      }
    }

    cxml << "</modules>" << '\n';

    // test the support structure definition
    unsigned int passiveAddon = 0u;
    if (itest == 1u) {
      cxml << indent_12_xml << "  <acts_passive_surface>" << '\n';
      cxml << indent_12_xml
           << "    <tubs rmin=\"20*mm\" rmax=\"120*mm\" dz=\"2*mm\" "
           << "cz=\"" << rZ + 10. << "\" material=\"Air\"/>" << '\n';
      cxml << indent_12_xml << "  </acts_passive_surface>" << '\n';
      passiveAddon = 1u;
    } else if (itest == 2u) {
      cxml << indent_12_xml << "  <acts_passive_surface>" << '\n';
      cxml << indent_12_xml
           << "    <tubs rmin=\"20*mm\" rmax=\"120*mm\" dz=\"2*mm\" "
           << "cz=\"" << rZ + 10. << "\" material=\"Air\"/>" << '\n';
      cxml << "    <acts_proto_material/>" << '\n';
      cxml << indent_12_xml << "  </acts_passive_surface>" << '\n';
      passiveAddon = 1u;
    }

    cxml << "</layer>" << '\n';

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

    Acts::Experimental::DD4hepLayerStructure discStructure(
        std::move(sFactory),
        Acts::getDefaultLogger("DD4hepLayerStructure", Acts::Logging::VERBOSE));

    Acts::DD4hepDetectorElement::Store dd4hepStore;

    Acts::Experimental::DD4hepLayerStructure::Options lsOptions;
    lsOptions.name = "DiscLayer";
    lsOptions.logLevel = Acts::Logging::VERBOSE;

    auto [discInternalsBuilder, discExt] =
        discStructure.builder(dd4hepStore, tContext, world, lsOptions);

    // Build the internal volume structure
    auto [surfaces, volumes, surfacesUpdater, volumeUpdater] =
        discInternalsBuilder->construct(tContext);

    // All surfaces are filled
    BOOST_CHECK_EQUAL(surfaces.size(), 44u + passiveAddon);
    // No volumes are added
    BOOST_CHECK(volumes.empty());
    // The surface updator is connected
    BOOST_CHECK(surfacesUpdater.connected());
    // The volume updator is connected
    BOOST_CHECK(volumeUpdater.connected());

    // Kill that instance before going into the next test
    lcdd->destroyInstance();

    // Increase test counter
    ++itest;
  }
}

BOOST_AUTO_TEST_SUITE_END()
