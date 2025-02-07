// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/DetectorComponents.hpp"
#include "Acts/Detector/LayerStructureBuilder.hpp"
#include "Acts/Detector/PortalGenerators.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/LayerCreator.hpp"
#include "Acts/Navigation/NavigationDelegates.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Tests/CommonHelpers/CylindricalTrackingGeometry.hpp"
#include "Acts/Utilities/BinningData.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <cmath>
#include <functional>
#include <memory>
#include <numbers>
#include <string>
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

}  // namespace

BOOST_AUTO_TEST_SUITE(Detector)

// Test the creation of a ring like structure
BOOST_AUTO_TEST_CASE(LayerStructureBuilder_creationRing) {
  // Detector store
  CylindricalTrackingGeometry::DetectorStore dStore;
  auto rSurfaces = cGeometry.surfacesRing(dStore, 6.4, 12.4, 36., 0.125, 0.,
                                          55., -800, 0., 22u);

  auto endcapSurfaces = std::make_shared<LayerStructureBuilder::SurfacesHolder>(
      unpackSurfaces(rSurfaces));
  // Configure the layer structure builder
  Acts::Experimental::LayerStructureBuilder::Config lsConfig;
  lsConfig.auxiliary = "*** Endcap with 22 surfaces ***";
  lsConfig.surfacesProvider = endcapSurfaces;
  lsConfig.binnings = {
      ProtoBinning(Acts::AxisDirection::AxisPhi, Acts::AxisBoundaryType::Closed,
                   -std::numbers::pi, std::numbers::pi, 22u, 1u)};

  auto endcapBuilder = Acts::Experimental::LayerStructureBuilder(
      lsConfig, Acts::getDefaultLogger("EndcapBuilder", Logging::VERBOSE));

  auto [surfaces0, volumes0, surfacesUpdater0, volumeUpdater0] =
      endcapBuilder.construct(tContext);

  BOOST_CHECK_EQUAL(surfaces0.size(), 22u);
  BOOST_CHECK(surfacesUpdater0.connected());
  BOOST_CHECK(volumes0.empty());
  BOOST_CHECK(volumeUpdater0.connected());

  // Define the layer support
  //
  // First test with one support disc
  using LayerSupport = Acts::Experimental::ProtoSupport;
  LayerSupport supportDisc;
  supportDisc.type = Acts::Surface::SurfaceType::Disc;
  supportDisc.offset = 15.;
  supportDisc.internalConstraints = {Acts::AxisDirection::AxisZ,
                                     Acts::AxisDirection::AxisR};

  lsConfig.auxiliary =
      "*** Endcap with 22 surfaces + 1 support disc, "
      "r/z - range/pos estimated from internals ***";
  lsConfig.supports = {supportDisc};
  endcapBuilder = Acts::Experimental::LayerStructureBuilder(
      lsConfig, Acts::getDefaultLogger("EndcapBuilder", Logging::VERBOSE));

  auto [surfaces1, volumes1, surfacesUpdater1, volumeUpdater1] =
      endcapBuilder.construct(tContext);

  BOOST_CHECK_EQUAL(surfaces1.size(), 22u + 1u);

  // Inspect the back surface
  const auto& supportSurface1 = (*surfaces1.back());
  BOOST_CHECK_EQUAL(supportSurface1.type(), Acts::Surface::SurfaceType::Disc);
  BOOST_CHECK_CLOSE(supportSurface1.transform(tContext).translation().z(),
                    -785., 1e-6);

  BOOST_CHECK(surfacesUpdater1.connected());
  BOOST_CHECK(volumes1.empty());
  BOOST_CHECK(volumeUpdater1.connected());

  // Redo and let r be estimated by a volume exetent with a [ 2_mm, 1_mm]
  // clearance: z is still from internals, but r is from the volume/external
  //
  // Second test with one support disc, but external constraint
  supportDisc.internalConstraints = {Acts::AxisDirection::AxisZ};
  supportDisc.volumeExtent.set(Acts::AxisDirection::AxisR, 10., 120.);
  supportDisc.volumeClearance[Acts::AxisDirection::AxisR] = {2., 1.};

  lsConfig.supports = {supportDisc};

  lsConfig.auxiliary =
      "*** Endcap with 22 surfaces + 1 support disc, "
      "z - pos estimated from internals,  "
      "r - range from external constraint  *** ";

  endcapBuilder = Acts::Experimental::LayerStructureBuilder(
      lsConfig, Acts::getDefaultLogger("EndcapBuilder", Logging::VERBOSE));

  auto [surfaces2, volumes2, surfacesUpdater2, volumeUpdater2] =
      endcapBuilder.construct(tContext);

  BOOST_CHECK_EQUAL(surfaces2.size(), 22u + 1u);

  // Inspect the back surface
  const auto& supportSurface2 = (*surfaces2.back());
  BOOST_CHECK_EQUAL(supportSurface2.type(), Acts::Surface::SurfaceType::Disc);
  BOOST_CHECK_CLOSE(supportSurface2.transform(tContext).translation().z(),
                    -785., 1e-6);
  const auto& supportBoundValues = supportSurface2.bounds().values();
  BOOST_CHECK_CLOSE(supportBoundValues[0u], 12., 1e-6);
  BOOST_CHECK_CLOSE(supportBoundValues[1u], 119., 1e-6);

  BOOST_CHECK(surfacesUpdater2.connected());
  BOOST_CHECK(volumes2.empty());
  BOOST_CHECK(volumeUpdater2.connected());

  // Redo with split
  //
  // Third test with splitting
  supportDisc.splits = 11u;
  lsConfig.supports = {supportDisc};

  lsConfig.auxiliary =
      "*** Endcap with 22 surfaces + 1 support -> split into 11 planes ***";

  endcapBuilder = Acts::Experimental::LayerStructureBuilder(
      lsConfig, Acts::getDefaultLogger("EndcapBuilder", Logging::VERBOSE));

  auto [surfaces3, volumes3, surfacesUpdater3, volumeUpdater3] =
      endcapBuilder.construct(tContext);

  BOOST_CHECK_EQUAL(surfaces3.size(), 22u + 11u);
  BOOST_CHECK_EQUAL(surfaces3.back()->type(),
                    Acts::Surface::SurfaceType::Plane);
  BOOST_CHECK(surfacesUpdater3.connected());
  BOOST_CHECK(volumes3.empty());
  BOOST_CHECK(volumeUpdater3.connected());
}

// Test the creation of a cylindrical structure
BOOST_AUTO_TEST_CASE(LayerStructureBuilder_creationCylinder) {
  CylindricalTrackingGeometry::DetectorStore dStore;
  auto cSurfaces = cGeometry.surfacesCylinder(dStore, 8.4, 36., 0.15, 0.145, 72,
                                              3., 2., {32u, 14u});

  auto barrelSurfaces = std::make_shared<LayerStructureBuilder::SurfacesHolder>(
      unpackSurfaces(cSurfaces));

  // Configure the layer structure builder
  Acts::Experimental::LayerStructureBuilder::Config lsConfig;
  lsConfig.auxiliary = "*** Barrel with 448 surfaces ***";
  lsConfig.surfacesProvider = barrelSurfaces;
  lsConfig.binnings = {
      Acts::Experimental::ProtoBinning{Acts::AxisDirection::AxisZ,
                                       Acts::AxisBoundaryType::Bound, -480.,
                                       480., 14u, 1u},
      Acts::Experimental::ProtoBinning(
          Acts::AxisDirection::AxisPhi, Acts::AxisBoundaryType::Closed,
          -std::numbers::pi, std::numbers::pi, 32u, 1u)};

  auto barrelBuilder = Acts::Experimental::LayerStructureBuilder(
      lsConfig, Acts::getDefaultLogger("BarrelBuilder", Logging::VERBOSE));

  auto [surfaces0, volumes0, surfacesUpdater0, volumeUpdater0] =
      barrelBuilder.construct(tContext);

  BOOST_CHECK_EQUAL(surfaces0.size(), 448u);
  BOOST_CHECK(surfacesUpdater0.connected());
  BOOST_CHECK(volumes0.empty());
  BOOST_CHECK(volumeUpdater0.connected());

  using LayerSupport = Acts::Experimental::ProtoSupport;

  // First test with one support cylinder
  LayerSupport supportCylinder;
  supportCylinder.type = Acts::Surface::SurfaceType::Cylinder;
  supportCylinder.offset = 15.;
  supportCylinder.internalConstraints = {Acts::AxisDirection::AxisZ,
                                         Acts::AxisDirection::AxisR};
  lsConfig.supports = {supportCylinder};
  lsConfig.auxiliary =
      "*** Barrel with 448 surfaces + 1 support cylinder, r/z evaluated ***";

  barrelBuilder = Acts::Experimental::LayerStructureBuilder(
      lsConfig, Acts::getDefaultLogger("BarrelBuilder", Logging::VERBOSE));

  auto [surfaces1, volumes1, surfacesUpdater1, volumeUpdater1] =
      barrelBuilder.construct(tContext);

  BOOST_CHECK_EQUAL(surfaces1.size(), 448u + 1u);
  BOOST_CHECK(surfacesUpdater1.connected());
  BOOST_CHECK(volumes1.empty());
  BOOST_CHECK(volumeUpdater1.connected());

  // Second test: z-range externally given
  supportCylinder.internalConstraints = {Acts::AxisDirection::AxisR};
  supportCylinder.volumeExtent.set(Acts::AxisDirection::AxisZ, -600., 600.);
  supportCylinder.volumeClearance[Acts::AxisDirection::AxisZ] = {2., 2.};
  lsConfig.supports = {supportCylinder};
  lsConfig.auxiliary =
      "*** Barrel with 448 surfaces + 1 support cylinder, r evaluated, z given "
      "by external constraint ***";

  barrelBuilder = Acts::Experimental::LayerStructureBuilder(
      lsConfig, Acts::getDefaultLogger("BarrelBuilder", Logging::VERBOSE));

  auto [surfaces2, volumes2, surfacesUpdater2, volumeUpdater2] =
      barrelBuilder.construct(tContext);

  BOOST_CHECK_EQUAL(surfaces2.size(), 448u + 1u);
  BOOST_CHECK(surfacesUpdater2.connected());
  BOOST_CHECK(volumes2.empty());
  BOOST_CHECK(volumeUpdater2.connected());
  // Inspect the back surface
  const auto& supportSurface2 = (*surfaces2.back());
  BOOST_CHECK_EQUAL(supportSurface2.type(),
                    Acts::Surface::SurfaceType::Cylinder);
  const auto supportBoundValues = supportSurface2.bounds().values();
  BOOST_CHECK_CLOSE(supportBoundValues[1u], 598., 1e-6);

  // Third test: split
  supportCylinder.splits = 32u;
  lsConfig.supports = {supportCylinder};
  lsConfig.auxiliary =
      "*** Barrel with 448 surfaces + 1 support -> split into 32 planes ***";

  barrelBuilder = Acts::Experimental::LayerStructureBuilder(
      lsConfig, Acts::getDefaultLogger("BarrelBuilder", Logging::VERBOSE));

  auto [surfaces3, volumes3, surfacesUpdater3, volumeUpdater3] =
      barrelBuilder.construct(tContext);

  BOOST_CHECK_EQUAL(surfaces3.size(), 448u + 32u);
  BOOST_CHECK(surfacesUpdater3.connected());
  BOOST_CHECK(volumes3.empty());
  BOOST_CHECK(volumeUpdater3.connected());
}

BOOST_AUTO_TEST_SUITE_END()
