// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Detector/CylindricalContainerBuilder.hpp"
#include "Acts/Detector/Detector.hpp"
#include "Acts/Detector/DetectorBuilder.hpp"
#include "Acts/Detector/detail/BlueprintDrawer.hpp"
#include "Acts/Detector/detail/BlueprintHelper.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Plugins/DD4hep/DD4hepBlueprintFactory.hpp"
#include "Acts/Plugins/DD4hep/DD4hepDetectorStructure.hpp"
#include "Acts/Plugins/DD4hep/DD4hepDetectorSurfaceFactory.hpp"
#include "Acts/Plugins/DD4hep/DD4hepLayerStructure.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Tests/CommonHelpers/CylindricalTrackingGeometry.hpp"
#include "Acts/Utilities/Enumerate.hpp"
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
                <layer id="0"  name="BeamPipeLayer">
)"""";

const char* nec_head_xml =
    R""""(
        <detector id="1" name="PixelNegativeEndcap" type="EndcapDetector" readout="PixelReadout">
            <type_flags type="DetType_TRACKER + DetType_BARREL"/>
)"""";

const char* barrel_head_xml =
    R""""(
        <detector id="2" name="PixelBarrel" type="BarrelDetector" readout="PixelReadout">
            <type_flags type="DetType_TRACKER + DetType_BARREL"/>
)"""";

const char* pec_head_xml =
    R""""(
        <detector id="3" name="PixelPositiveEndcap" type="EndcapDetector" readout="PixelReadout">
            <type_flags type="DetType_TRACKER + DetType_BARREL"/>
)"""";

const char* plugin_xml =
    R"""(<plugins>
    <plugin name="DD4hep_ParametersPlugin">
      <argument value="world"/>
      <argument value="acts_world: bool = true"/>
      <argument value="acts_world_type: int = 3"/>
      <argument value="acts_world_bvalues_n: int = 3"/>
      <argument value="acts_world_bvalues_0: double = 0"/>
      <argument value="acts_world_bvalues_1: double = 150*mm"/>
      <argument value="acts_world_bvalues_2: double = 1000*mm"/>
      <argument value="acts_world_binning: str = r"/>
      <argument value="acts_world_geo_id: str = incremental"/>
      <argument value="acts_world_root_volume_finder: str = indexed"/>
    </plugin>
    <plugin name="DD4hep_ParametersPlugin">
      <argument value="BeamPipe"/>
      <argument value="acts_volume: bool = true"/>
      <argument value="acts_volume_type: int = 3"/>
      <argument value="acts_volume_bvalues_n: int = 3"/>
      <argument value="acts_volume_bvalues_0: double = 0"/>
      <argument value="acts_volume_bvalues_1: double = 20*mm"/>
      <argument value="acts_volume_bvalues_2: double = 1000*mm"/>
      <argument value="acts_volume_internals: bool = true"/>
      <argument value="acts_volume_internals_type: str = layer"/>
    </plugin>
    <plugin name="DD4hep_ParametersPlugin">
      <argument value="Pixel"/>
      <argument value="acts_container: bool = true"/>
      <argument value="acts_container_type: int = 3"/>
      <argument value="acts_container_bvalues_n: int = 3"/>
      <argument value="acts_container_bvalues_0: double = 20*mm"/>
      <argument value="acts_container_bvalues_1: double = 150*mm"/>
      <argument value="acts_container_bvalues_2: double = 1000*mm"/>
      <argument value="acts_container_binning: str = z"/>
    </plugin>
    <plugin name="DD4hep_ParametersPlugin">
      <argument value="PixelNegativeEndcap"/>
      <argument value="acts_container: bool = true"/>
      <argument value="acts_container_type: int = 3"/>
      <argument value="acts_container_bvalues_n: int = 3"/>
      <argument value="acts_container_bvalues_0: double = 20*mm"/>
      <argument value="acts_container_bvalues_1: double = 150*mm"/>
      <argument value="acts_container_bvalues_2: double = 200*mm"/>
      <argument value="acts_container_binning: str = z"/>
      <argument value="acts_container_z: double = -800*mm"/>
    </plugin>
    <plugin name="DD4hep_ParametersPlugin">
      <argument value="PixelBarrel"/>
      <argument value="acts_container: bool = true"/>
      <argument value="acts_container_type: int = 3"/>
      <argument value="acts_container_bvalues_n: int = 3"/>
      <argument value="acts_container_bvalues_0: double = 20*mm"/>
      <argument value="acts_container_bvalues_1: double = 150*mm"/>
      <argument value="acts_container_bvalues_2: double = 600*mm"/>
      <argument value="acts_container_binning: str = r"/>
    </plugin>
    <plugin name="DD4hep_ParametersPlugin">
      <argument value="PixelPositiveEndcap"/>
      <argument value="acts_container: bool = true"/>
      <argument value="acts_container_type: int = 3"/>
      <argument value="acts_container_bvalues_n: int = 3"/>
      <argument value="acts_container_bvalues_0: double = 20*mm"/>
      <argument value="acts_container_bvalues_1: double = 150*mm"/>
      <argument value="acts_container_bvalues_2: double = 200*mm"/>
      <argument value="acts_container_binning: str = z"/>
      <argument value="acts_container_z: double = 800*mm"/>
    </plugin>
  </plugins>
  )""";

const char* indent_4_xml = "    ";
const char* indent_8_xml = "        ";
const char* indent_12_xml = "            ";

namespace {

Acts::Test::CylindricalTrackingGeometry::DetectorStore generateXML() {
  Acts::Test::CylindricalTrackingGeometry::DetectorStore dStore;

  // Nec surfaces
  Acts::ActsScalar necZ = -800.;
  auto necR0Surfaces = cGeometry.surfacesRing(dStore, 6.4, 12.4, 18., 0.125, 0.,
                                              42., necZ, 2., 22u);

  auto necR1Surfaces = cGeometry.surfacesRing(dStore, 12.4, 20.4, 30., 0.125,
                                              0., 80., necZ, 2., 22u);

  std::vector<std::vector<const Acts::Surface*>> necSurfaces = {necR0Surfaces,
                                                                necR1Surfaces};

  // Barrel surfaces
  std::vector<std::array<Acts::ActsScalar, 2u>> innerOuter = {
      {25., 35.}, {65., 75.}, {110., 120.}};
  auto b0Surfaces = cGeometry.surfacesCylinder(dStore, 8.4, 36., 0.15, 0.14,
                                               31., 3., 2., {16, 14});

  auto b1Surfaces = cGeometry.surfacesCylinder(dStore, 8.4, 36., 0.15, 0.14,
                                               71., 3., 2., {32, 14});

  auto b2Surfaces = cGeometry.surfacesCylinder(dStore, 8.4, 36., 0.15, 0.14,
                                               116., 3., 2., {52, 14});

  std::vector<std::vector<const Acts::Surface*>> barrelSurfaces = {
      b0Surfaces, b1Surfaces, b2Surfaces};

  // Nec surfaces
  Acts::ActsScalar pecZ = 800.;
  auto pecR0Surfaces = cGeometry.surfacesRing(dStore, 6.4, 12.4, 18., 0.125, 0.,
                                              42., pecZ, 2., 22u);

  auto pecR1Surfaces = cGeometry.surfacesRing(dStore, 12.4, 20.4, 30., 0.125,
                                              0., 80., pecZ, 2., 22u);

  std::vector<std::vector<const Acts::Surface*>> pecSurfaces = {pecR0Surfaces,
                                                                pecR1Surfaces};

  // Create an XML from it
  std::ofstream cxml;
  cxml.open("CylindricalDetector.xml");
  cxml << head_xml;
  cxml << segmentation_xml;

  // Add the beam pipe first
  cxml << beampipe_head_xml << '\n';
  cxml << indent_12_xml << "<acts_passive_surface>" << '\n';
  cxml << indent_12_xml
       << "<tubs rmin=\"19*mm\" rmax=\"20*mm\" dz=\"800*mm\" "
          "material=\"Air\"/>"
       << '\n';
  cxml << indent_12_xml << "</acts_passive_surface>" << '\n';
  cxml << indent_12_xml << "</layer> " << '\n';
  cxml << indent_8_xml << "</layers>" << '\n';
  cxml << indent_8_xml << "</detector>" << '\n';

  // Declare the assembly
  const char* pixel_assembly_xml =
      R"""(<detector id="4" name="Pixel" type="DD4hep_SubdetectorAssembly" vis="invisible">
            <shape name="PixelVolume" type="Tube" rmin="20*mm" rmax="150*mm" dz="1000*mm" material="Air"/>
            <composite name="PixelNegativeEndcap"/>
            <composite name="PixelBarrel"/>
            <composite name="PixelPositiveEndcap"/>
        </detector>)""";

  cxml << pixel_assembly_xml << '\n';
  cxml << "</detectors>" << '\n';

  // The Pixel detector

  // Nec - 1 Layer only
  cxml << "<detectors>" << '\n';

  cxml << nec_head_xml << '\n';
  cxml << indent_8_xml << "<layers>" << '\n';
  cxml << indent_8_xml << "<acts_container/> " << '\n';
  cxml << indent_4_xml << "<layer name=\"NegEndcapLayer_0\" id=\"0\">" << '\n';
  cxml << indent_4_xml << "<acts_volume dz=\"20*mm\" cz=\"-800.*mm\"/>" << '\n';
  cxml << indent_8_xml << "<modules>" << '\n';
  for (const auto& ring : necSurfaces) {
    for (const auto& s : ring) {
      cxml << indent_12_xml
           << DD4hepTestsHelper::surfaceToXML(tContext, *s,
                                              Acts::Transform3::Identity())
           << "\n";
    }
  }
  cxml << indent_8_xml << "</modules>" << '\n';
  cxml << indent_8_xml << "</layer> " << '\n';
  cxml << indent_8_xml << "</layers>" << '\n';
  cxml << indent_8_xml << "</detector>" << '\n';

  // Barrel
  cxml << barrel_head_xml << '\n';
  cxml << indent_8_xml << "<layers>" << '\n';
  cxml << indent_8_xml << "<acts_container/> " << '\n';
  for (const auto [ib, bs] : Acts::enumerate(barrelSurfaces)) {
    cxml << indent_4_xml << "<layer name=\"PixelBarrel_" << ib << "\" id=\""
         << ib << "\">" << '\n';
    cxml << indent_4_xml << "<acts_volume rmin=\"" << innerOuter[ib][0u]
         << "*mm\" rmax=\"" << innerOuter[ib][1u] << "*mm\"/>";
    cxml << indent_8_xml << "<modules>" << '\n';
    for (const auto& s : bs) {
      cxml << indent_12_xml
           << DD4hepTestsHelper::surfaceToXML(tContext, *s,
                                              Acts::Transform3::Identity())
           << "\n";
    }
    cxml << indent_8_xml << "</modules>" << '\n';
    cxml << indent_8_xml << "</layer>" << '\n';
  }
  cxml << indent_8_xml << "</layers>" << '\n';
  cxml << indent_8_xml << "</detector>" << '\n';

  // Pec - 1 layer only
  cxml << pec_head_xml << '\n';
  cxml << indent_8_xml << "<layers>" << '\n';
  cxml << indent_8_xml << "<acts_container/> " << '\n';
  cxml << indent_4_xml << "<layer name=\"PosEndcapLayer_0\" id=\"0\">" << '\n';
  cxml << indent_4_xml << "<acts_volume dz=\"20*mm\" cz=\"800.*mm\"/>" << '\n';
  cxml << indent_8_xml << "<modules>" << '\n';
  for (const auto& ring : pecSurfaces) {
    for (const auto& s : ring) {
      cxml << indent_12_xml
           << DD4hepTestsHelper::surfaceToXML(tContext, *s,
                                              Acts::Transform3::Identity())
           << "\n";
    }
  }
  cxml << indent_8_xml << "</modules>" << '\n';
  cxml << indent_8_xml << "</layer> " << '\n';
  cxml << indent_8_xml << "</layers>" << '\n';
  cxml << indent_8_xml << "</detector>" << '\n';
  cxml << indent_8_xml << "</detectors>" << '\n';

  cxml << plugin_xml << '\n';
  cxml << end_xml << '\n';
  cxml.close();

  return dStore;
}

}  // namespace

auto store = generateXML();

BOOST_AUTO_TEST_SUITE(DD4hepPlugin)

BOOST_AUTO_TEST_CASE(DD4hepCylidricalDetectorExplicit) {
  auto lcdd = &(dd4hep::Detector::getInstance());
  lcdd->fromCompact("CylindricalDetector.xml");
  lcdd->volumeManager();
  lcdd->apply("DD4hepVolumeManager", 0, nullptr);

  auto world = lcdd->world();

  // Test starts here
  auto surfaceFactory = std::make_shared<Acts::DD4hepDetectorSurfaceFactory>(
      Acts::getDefaultLogger("DD4hepDetectorSurfaceFactory",
                             Acts::Logging::VERBOSE));

  auto layerStructure =
      std::make_shared<Acts::Experimental::DD4hepLayerStructure>(
          std::move(surfaceFactory),
          Acts::getDefaultLogger("DD4hepLayerStructure",
                                 Acts::Logging::VERBOSE));

  Acts::Experimental::DD4hepBlueprintFactory::Config bpCfg{layerStructure};
  Acts::Experimental::DD4hepBlueprintFactory::Cache bpCache;

  Acts::Experimental::DD4hepBlueprintFactory bp(
      bpCfg,
      Acts::getDefaultLogger("DD4hepBlueprintFactory", Acts::Logging::VERBOSE));
  auto dd4hepBlueprint = bp.create(bpCache, tContext, world);

  // We should have 6 store entries now
  // 1 : beam pipe (empty)
  // 1 : endcap
  // 2 : barrel
  // 1 : endcap
  BOOST_CHECK_EQUAL(bpCache.dd4hepStore.size(), 6u);

  // Now fill the gaps
  Acts::Experimental::detail::BlueprintHelper::fillGaps(*dd4hepBlueprint);

  // dot -P -Tsvg -o plugins.svg
  std::ofstream cbp("cylindrical_detector_dd4hep.dot");
  Acts::Experimental::detail::BlueprintDrawer::dotStream(cbp, *dd4hepBlueprint);
  cbp.close();

  // Create a Cylindrical detector builder from this blueprint
  auto detectorBuilder =
      std::make_shared<Acts::Experimental::CylindricalContainerBuilder>(
          *dd4hepBlueprint, Acts::Logging::VERBOSE);

  // Detector builder
  Acts::Experimental::DetectorBuilder::Config dCfg;
  dCfg.auxiliary =
      "*** Test : auto generated cylindrical detector builder  ***";
  dCfg.name = "Cylindrical detector from blueprint";
  dCfg.builder = detectorBuilder;
  dCfg.geoIdGenerator = dd4hepBlueprint->geoIdGenerator;

  auto detector = Acts::Experimental::DetectorBuilder(dCfg).construct(tContext);

  // Detector construction check
  BOOST_REQUIRE_NE(detector, nullptr);
  // We should have 14 volumes
  // 1 : beampipe
  // 3 : negative encap
  // 7 : barrel
  // 3 : positive encap
  BOOST_CHECK_EQUAL(detector->volumes().size(), 14u);

  // Kill that instance before going into the next test
  lcdd->destroyInstance();
}

BOOST_AUTO_TEST_CASE(DD4hepCylidricalDetectorStructure) {
  auto lcdd = &(dd4hep::Detector::getInstance());
  lcdd->fromCompact("CylindricalDetector.xml");
  lcdd->volumeManager();
  lcdd->apply("DD4hepVolumeManager", 0, nullptr);

  auto world = lcdd->world();

  Acts::Experimental::DD4hepDetectorStructure::Options dsOptions;
  dsOptions.logLevel = Acts::Logging::VERBOSE;
  dsOptions.emulateToGraph = "cylindrical_detector_structure";

  auto [detectorEm, detectorStoreEm] =
      Acts::Experimental::DD4hepDetectorStructure(
          Acts::getDefaultLogger("DD4hepDetectorStructure",
                                 Acts::Logging::VERBOSE))
          .construct(tContext, world, dsOptions);

  // Detector construction : no detector constructed, as we have only
  // emulated the grapth writing
  BOOST_CHECK_EQUAL(detectorEm, nullptr);

  // Now build in non-emulation mode
  dsOptions.emulateToGraph = "";
  auto [detector, detectorStore] =
      Acts::Experimental::DD4hepDetectorStructure(
          Acts::getDefaultLogger("DD4hepDetectorStructure",
                                 Acts::Logging::VERBOSE))
          .construct(tContext, world, dsOptions);

  BOOST_REQUIRE_NE(detector, nullptr);

  // We should have 14 volumes
  // 1 : beampipe
  // 3 : negative endcap
  // 7 : barrel
  // 3 : positive endcap
  BOOST_CHECK_EQUAL(detector->volumes().size(), 14u);

  // We should have 6 store entries now
  // 1 : beam pipe (empty)
  // 1 : endcap
  // 3 : barrel
  // 1 : endcap
  BOOST_CHECK_EQUAL(detectorStore.size(), 6u);

  int elements = 0;
  for (const auto& [key, value] : detectorStore) {
    elements += value.size();
  }

  // There should be 1488 elements
  // NegEndcapLayer_0 has : 44 detector elements.
  // PixelBarrel_0 has : 224 detector elements.
  // PixelBarrel_1 has : 448 detector elements.
  // PixelBarrel_2 has : 728 detector elements.
  // PosEndcapLayer_0 has : 44 detector elements.
  BOOST_CHECK_EQUAL(elements, 1488);

  // Cross-check with the surfaces
  int surfaces = 0;
  for (const auto& v : detector->volumes()) {
    surfaces += v->surfaces().size();
  }
  // Sensitives + 1 (beampipe)
  BOOST_CHECK_EQUAL(surfaces, 1489);

  // Kill that instance before going into the next test
  lcdd->destroyInstance();
}

BOOST_AUTO_TEST_SUITE_END()
