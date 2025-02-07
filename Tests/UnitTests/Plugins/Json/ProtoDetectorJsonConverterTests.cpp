// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/ProtoDetector.hpp"
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Plugins/Json/ProtoDetectorJsonConverter.hpp"
#include "Acts/Plugins/Json/SurfaceJsonConverter.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/BinningData.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Enumerate.hpp"

#include <array>
#include <cmath>
#include <fstream>
#include <iterator>
#include <numbers>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "EqualityHelpers.hpp"

using namespace Acts;

namespace {

/// @brief Helper method to compare proto volumes
/// @param one the first volume object
/// @param two the second volume object
/// @param tolerance the tolerance
/// @return a boolean to see if they are equal
bool isEqual(const Acts::ProtoVolume& one, const Acts::ProtoVolume& two,
             const double tolerance = 0.) {
  bool nameEq = (one.name == two.name);
  // Name
  BOOST_CHECK(nameEq);
  // Extent
  bool extentEq = isEqual(one.extent, two.extent, tolerance);
  BOOST_CHECK(extentEq);

  // Check internal structure
  bool internalValueEq = (one.internal.has_value() == two.internal.has_value());
  BOOST_CHECK(internalValueEq);
  bool internalEq = internalValueEq;
  if (one.internal.has_value() && two.internal.has_value()) {
    // Check consistency of the internal structure
    const auto& itsOne = one.internal.value();
    const auto& itsTwo = two.internal.value();
    bool layerTypeEq = (itsOne.layerType == itsTwo.layerType);
    BOOST_CHECK(layerTypeEq);
    internalEq = layerTypeEq;
    bool sBinningSizeEq =
        (itsOne.surfaceBinning.size() == itsTwo.surfaceBinning.size());
    BOOST_CHECK(sBinningSizeEq);
    internalEq = internalEq && sBinningSizeEq;
    for (auto [isb, sb] : Acts::enumerate(itsOne.surfaceBinning)) {
      bool sBinningEq = isEqual(sb, itsTwo.surfaceBinning[isb], tolerance);
      BOOST_CHECK(sBinningEq);
      internalEq = internalEq && sBinningEq;
    }
  }
  BOOST_CHECK(internalEq);

  // Check container structure
  bool containerValueEq =
      (one.container.has_value() == two.container.has_value());
  BOOST_CHECK(containerValueEq);
  bool containerEq = containerValueEq;
  if (one.container.has_value() && two.container.has_value()) {
    // Check consistency of the container structure
    const auto& ctsOne = one.container.value();
    const auto& ctsTwo = two.container.value();
    bool layerContainerEq = (ctsOne.layerContainer == ctsTwo.layerContainer);
    BOOST_CHECK(layerContainerEq);
    containerEq = layerContainerEq;
    bool cBinningSizeEq =
        ctsOne.constituentBinning.size() == ctsTwo.constituentBinning.size();
    containerEq = containerEq && cBinningSizeEq;
    BOOST_CHECK(cBinningSizeEq);
    for (auto [icb, cb] : Acts::enumerate(ctsOne.constituentBinning)) {
      bool cBinningEq = isEqual(cb, ctsTwo.constituentBinning[icb], tolerance);
      BOOST_CHECK(cBinningEq);
      containerEq = containerEq && cBinningEq;
    }
    // Recursively walk down
    bool cSizeEq =
        (ctsOne.constituentVolumes.size() == ctsTwo.constituentVolumes.size());
    BOOST_CHECK(cSizeEq);
    containerEq = cSizeEq;
    for (auto [ic, cOne] : Acts::enumerate(ctsOne.constituentVolumes)) {
      const auto& cTwo = ctsTwo.constituentVolumes[ic];
      bool cEq = isEqual(cOne, cTwo, tolerance);
      BOOST_CHECK(cEq);
      containerEq = containerEq && cEq;
    }
  }
  BOOST_CHECK(containerEq);

  // Give the overall judgement
  return nameEq && extentEq && internalEq && containerEq;
}

}  // namespace

BOOST_AUTO_TEST_SUITE(ProtoDetectorJsonConverter)

BOOST_AUTO_TEST_CASE(ProtoDetectorRoundTrip) {
  Acts::ExtentEnvelope cylinderLayerEnvelope = ExtentEnvelope::Zero();
  cylinderLayerEnvelope[Acts::AxisDirection::AxisR] = {1., 1.};
  cylinderLayerEnvelope[Acts::AxisDirection::AxisZ] = {2., 2.};

  Acts::ExtentEnvelope discLayerEnvelope = ExtentEnvelope::Zero();
  discLayerEnvelope[Acts::AxisDirection::AxisR] = {1., 1.};
  discLayerEnvelope[Acts::AxisDirection::AxisZ] = {1., 1.};

  // Beam Pipe container
  Acts::ProtoVolume beamPipeContainer;
  beamPipeContainer.name = "odd-beam-pipe";
  beamPipeContainer.extent.set(Acts::AxisDirection::AxisR, 0., 25);
  Acts::ProtoVolume beamPipe;
  beamPipe.name = "odd-beam-pipe-l";
  beamPipe.extent.set(Acts::AxisDirection::AxisR, 2., 24.);
  beamPipe.internal = Acts::ProtoVolume::InternalStructure{
      Acts::Surface::SurfaceType::Cylinder};
  beamPipeContainer.container = Acts::ProtoVolume::ContainerStructure{
      {beamPipe},
      {Acts::BinningData(Acts::open, Acts::AxisDirection::AxisR, {0., 1.})},
      true};

  // Pixel section
  Acts::ProtoVolume pixelContainer;
  pixelContainer.name = "odd-pixel";
  pixelContainer.extent.set(Acts::AxisDirection::AxisR, 25., 200);

  Acts::ProtoVolume pixelNec;
  pixelNec.name = "odd-pixel-nec";
  pixelNec.extent.set(Acts::AxisDirection::AxisZ, -3100., -580);

  Acts::ProtoVolume pixNecD6;
  pixNecD6.name = "odd-pixel-nec-d6";
  pixNecD6.extent.set(Acts::AxisDirection::AxisZ, -1540., -1500);
  Acts::ProtoVolume pixNecD5;
  pixNecD5.name = "odd-pixel-nec-d5";
  pixNecD5.extent.set(Acts::AxisDirection::AxisZ, -1340., -1300);
  Acts::ProtoVolume pixNecD4;
  pixNecD4.name = "odd-pixel-nec-d4";
  pixNecD4.extent.set(Acts::AxisDirection::AxisZ, -1140., -1100);
  Acts::ProtoVolume pixNecD3;
  pixNecD3.name = "odd-pixel-nec-d3";
  pixNecD3.extent.set(Acts::AxisDirection::AxisZ, -1000., -960.);
  Acts::ProtoVolume pixNecD2;
  pixNecD2.name = "odd-pixel-nec-d2";
  pixNecD2.extent.set(Acts::AxisDirection::AxisZ, -860., -820);
  Acts::ProtoVolume pixNecD1;
  pixNecD1.name = "odd-pixel-nec-d1";
  pixNecD1.extent.set(Acts::AxisDirection::AxisZ, -740., -700);
  Acts::ProtoVolume pixNecD0;
  pixNecD0.name = "odd-pixel-nec-d0";
  pixNecD0.extent.set(Acts::AxisDirection::AxisZ, -640., -600);
  pixelNec.container = Acts::ProtoVolume::ContainerStructure{
      {pixNecD6, pixNecD5, pixNecD4, pixNecD3, pixNecD2, pixNecD1, pixNecD0},
      {Acts::BinningData(Acts::open, Acts::AxisDirection::AxisZ, {0., 1.})},
      true};

  Acts::BinningData pixEcBinningR =
      Acts::BinningData(Acts::open, Acts::AxisDirection::AxisR, 2., 0., 1.);
  Acts::BinningData pixEcBinningPhi =
      Acts::BinningData(Acts::closed, Acts::AxisDirection::AxisPhi, 30.,
                        -std::numbers::pi, std::numbers::pi);

  for (auto& cv : pixelNec.container.value().constituentVolumes) {
    cv.extent.setEnvelope(discLayerEnvelope);
    cv.internal = Acts::ProtoVolume::InternalStructure{
        Acts::Surface::SurfaceType::Disc, {pixEcBinningR, pixEcBinningPhi}};
  }

  Acts::ProtoVolume pixelBarrel;
  pixelBarrel.name = "odd-pixel-barrel";
  pixelBarrel.extent.set(Acts::AxisDirection::AxisZ, -580., 580);

  Acts::ProtoVolume pixBarrelL0;
  pixBarrelL0.name = "odd-pixel-barrel-l0";
  pixBarrelL0.extent.set(Acts::AxisDirection::AxisR, 28., 48.);
  pixBarrelL0.extent.set(Acts::AxisDirection::AxisZ, -580., 580);
  Acts::ProtoVolume pixBarrelL1;
  pixBarrelL1.name = "odd-pixel-barrel-l1";
  pixBarrelL1.extent.set(Acts::AxisDirection::AxisR, 62., 76);
  pixBarrelL1.extent.set(Acts::AxisDirection::AxisZ, -580., 580);
  Acts::ProtoVolume pixBarrelL2;
  pixBarrelL2.name = "odd-pixel-barrel-l2";
  pixBarrelL2.extent.set(Acts::AxisDirection::AxisR, 100., 120.);
  pixBarrelL2.extent.set(Acts::AxisDirection::AxisZ, -580., 580);
  Acts::ProtoVolume pixBarrelL3;
  pixBarrelL3.name = "odd-pixel-barrel-l3";
  pixBarrelL3.extent.set(Acts::AxisDirection::AxisR, 160., 180.);
  pixBarrelL3.extent.set(Acts::AxisDirection::AxisZ, -580., 580);

  pixelBarrel.container = Acts::ProtoVolume::ContainerStructure{
      {pixBarrelL0, pixBarrelL1, pixBarrelL2, pixBarrelL3},
      {Acts::BinningData(Acts::open, Acts::AxisDirection::AxisR, {0., 1})},
      true};

  for (auto& cv : pixelBarrel.container.value().constituentVolumes) {
    cv.extent.setEnvelope(cylinderLayerEnvelope);
    cv.internal = Acts::ProtoVolume::InternalStructure{
        Acts::Surface::SurfaceType::Cylinder};
  }

  Acts::ProtoVolume pixelPec;
  pixelPec.name = "odd-pixel-pec";
  pixelPec.extent.set(Acts::AxisDirection::AxisZ, 580., 3100.);

  Acts::ProtoVolume pixPecD0;
  pixPecD0.name = "odd-pixel-pec-d0";
  pixPecD0.extent.set(Acts::AxisDirection::AxisZ, 600., 640);
  Acts::ProtoVolume pixPecD1;
  pixPecD1.name = "odd-pixel-pec-d1";
  pixPecD1.extent.set(Acts::AxisDirection::AxisZ, 700., 740);
  Acts::ProtoVolume pixPecD2;
  pixPecD2.name = "odd-pixel-pec-d2";
  pixPecD2.extent.set(Acts::AxisDirection::AxisZ, 820., 860.);
  Acts::ProtoVolume pixPecD3;
  pixPecD3.name = "odd-pixel-pec-d3";
  pixPecD3.extent.set(Acts::AxisDirection::AxisZ, 960., 1000.);
  Acts::ProtoVolume pixPecD4;
  pixPecD4.name = "odd-pixel-pec-d4";
  pixPecD4.extent.set(Acts::AxisDirection::AxisZ, 1100., 1140);
  Acts::ProtoVolume pixPecD5;
  pixPecD5.name = "odd-pixel-pec-d5";
  pixPecD5.extent.set(Acts::AxisDirection::AxisZ, 1300., 1340.);
  Acts::ProtoVolume pixPecD6;
  pixPecD6.name = "odd-pixel-pec-d6";
  pixPecD6.extent.set(Acts::AxisDirection::AxisZ, 1500., 1540.);

  pixelPec.container = Acts::ProtoVolume::ContainerStructure{
      {pixPecD0, pixPecD1, pixPecD2, pixPecD3, pixPecD4, pixPecD5, pixPecD6},
      {Acts::BinningData(Acts::open, Acts::AxisDirection::AxisZ, {0., 1})},
      true};

  for (auto& cv : pixelPec.container.value().constituentVolumes) {
    cv.extent.setEnvelope(discLayerEnvelope);
    cv.internal = Acts::ProtoVolume::InternalStructure{
        Acts::Surface::SurfaceType::Disc, {pixEcBinningR, pixEcBinningPhi}};
  }

  pixelContainer.container = Acts::ProtoVolume::ContainerStructure{
      {pixelNec, pixelBarrel, pixelPec},
      {Acts::BinningData(Acts::open, Acts::AxisDirection::AxisZ,
                         {-3100., -580., 580., 3100.})}};

  // Short Strip section
  Acts::ProtoVolume pstContainer;
  pstContainer.name = "odd-pst";
  pstContainer.extent.set(Acts::AxisDirection::AxisR, 200., 210.);
  Acts::ProtoVolume pst;
  pst.name = "odd-pst-l";
  pst.extent.set(Acts::AxisDirection::AxisR, 201., 209.);
  pst.internal = Acts::ProtoVolume::InternalStructure{
      Acts::Surface::SurfaceType::Cylinder};
  pstContainer.container = Acts::ProtoVolume::ContainerStructure{
      {pst},
      {Acts::BinningData(Acts::open, Acts::AxisDirection::AxisR, {0., 1.})},
      true};

  // Short Strip section
  Acts::ProtoVolume sstripContainer;
  sstripContainer.name = "odd-sstrip";
  sstripContainer.extent.set(Acts::AxisDirection::AxisR, 210., 720);

  Acts::BinningData sstripEcBinningR =
      Acts::BinningData(Acts::open, Acts::AxisDirection::AxisR, 3., 0., 1.);
  Acts::BinningData sstripEcBinningPhi =
      Acts::BinningData(Acts::closed, Acts::AxisDirection::AxisPhi, 42.,
                        -std::numbers::pi, std::numbers::pi);

  Acts::ProtoVolume sstripNec;
  sstripNec.name = "odd-sstrip-nec";
  sstripNec.extent.set(Acts::AxisDirection::AxisZ, -3100., -1200);
  Acts::ProtoVolume sstripNecD5;
  sstripNecD5.name = "odd-sstrip-nec-d5";
  sstripNecD5.extent.set(Acts::AxisDirection::AxisZ, -3000, -2900.);
  Acts::ProtoVolume sstripNecD4;
  sstripNecD4.name = "odd-sstrip-nec-d4";
  sstripNecD4.extent.set(Acts::AxisDirection::AxisZ, -2600., -2500.);
  Acts::ProtoVolume sstripNecD3;
  sstripNecD3.name = "odd-sstrip-nec-d3";
  sstripNecD3.extent.set(Acts::AxisDirection::AxisZ, -2250, -2150.);
  Acts::ProtoVolume sstripNecD2;
  sstripNecD2.name = "odd-sstrip-nec-d2";
  sstripNecD2.extent.set(Acts::AxisDirection::AxisZ, -1900, -1800.);
  Acts::ProtoVolume sstripNecD1;
  sstripNecD1.name = "odd-sstrip-nec-d1";
  sstripNecD1.extent.set(Acts::AxisDirection::AxisZ, -1600., -1500.);
  Acts::ProtoVolume sstripNecD0;
  sstripNecD0.name = "odd-sstrip-nec-d0";
  sstripNecD0.extent.set(Acts::AxisDirection::AxisZ, -1350., -1250.);

  sstripNec.container = Acts::ProtoVolume::ContainerStructure{
      {sstripNecD5, sstripNecD4, sstripNecD3, sstripNecD2, sstripNecD1,
       sstripNecD0},
      {Acts::BinningData(Acts::open, Acts::AxisDirection::AxisZ, {0., 1})},
      true};

  for (auto& cv : sstripNec.container.value().constituentVolumes) {
    cv.extent.setEnvelope(discLayerEnvelope);
    cv.internal = Acts::ProtoVolume::InternalStructure{
        Acts::Surface::SurfaceType::Disc,
        {sstripEcBinningR, sstripEcBinningPhi}};
  }

  Acts::ProtoVolume sstripBarrel;
  sstripBarrel.name = "odd-sstrip-barrel";
  sstripBarrel.extent.set(Acts::AxisDirection::AxisZ, -1200., 1200);

  Acts::ProtoVolume sstripBarrelL0;
  sstripBarrelL0.name = "odd-sstrip-barrel-l0";
  sstripBarrelL0.extent.set(Acts::AxisDirection::AxisR, 240., 280.);
  Acts::ProtoVolume sstripBarrelL1;
  sstripBarrelL1.name = "odd-sstrip-barrel-l1";
  sstripBarrelL1.extent.set(Acts::AxisDirection::AxisR, 340., 380.);
  Acts::ProtoVolume sstripBarrelL2;
  sstripBarrelL2.name = "odd-sstrip-barrel-l2";
  sstripBarrelL2.extent.set(Acts::AxisDirection::AxisR, 480., 520.);
  Acts::ProtoVolume sstripBarrelL3;
  sstripBarrelL3.name = "odd-sstrip-barrel-l3";
  sstripBarrelL3.extent.set(Acts::AxisDirection::AxisR, 640., 680.);

  sstripBarrel.container = Acts::ProtoVolume::ContainerStructure{
      {sstripBarrelL0, sstripBarrelL1, sstripBarrelL2, sstripBarrelL3},
      {Acts::BinningData(Acts::open, Acts::AxisDirection::AxisR, {0., 1})},
      true};

  for (auto& cv : sstripBarrel.container.value().constituentVolumes) {
    cv.extent.setEnvelope(cylinderLayerEnvelope);
    cv.internal = Acts::ProtoVolume::InternalStructure{
        Acts::Surface::SurfaceType::Cylinder};
  }

  Acts::ProtoVolume sstripPec;
  sstripPec.name = "odd-sstrip-pec";
  sstripPec.extent.set(Acts::AxisDirection::AxisZ, 1200., 3100);

  Acts::ProtoVolume sstripPecD0;
  sstripPecD0.name = "odd-sstrip-pec-d0";
  sstripPecD0.extent.set(Acts::AxisDirection::AxisZ, 1250., 1350);
  Acts::ProtoVolume sstripPecD1;
  sstripPecD1.name = "odd-sstrip-pec-d1";
  sstripPecD1.extent.set(Acts::AxisDirection::AxisZ, 1500., 1600.);
  Acts::ProtoVolume sstripPecD2;
  sstripPecD2.name = "odd-sstrip-pec-d2";
  sstripPecD2.extent.set(Acts::AxisDirection::AxisZ, 1800., 1900.);
  Acts::ProtoVolume sstripPecD3;
  sstripPecD3.name = "odd-sstrip-pec-d3";
  sstripPecD3.extent.set(Acts::AxisDirection::AxisZ, 2150., 2250.);
  Acts::ProtoVolume sstripPecD4;
  sstripPecD4.name = "odd-sstrip-pec-d4";
  sstripPecD4.extent.set(Acts::AxisDirection::AxisZ, 2500., 2600.);
  Acts::ProtoVolume sstripPecD5;
  sstripPecD5.name = "odd-sstrip-pec-d5";
  sstripPecD5.extent.set(Acts::AxisDirection::AxisZ, 2900., 3000.);

  sstripPec.container = Acts::ProtoVolume::ContainerStructure{
      {sstripPecD0, sstripPecD1, sstripPecD2, sstripPecD3, sstripPecD4,
       sstripPecD5},
      {Acts::BinningData(Acts::open, Acts::AxisDirection::AxisZ, {0., 1})},
      true};
  for (auto& cv : sstripPec.container.value().constituentVolumes) {
    cv.extent.setEnvelope(discLayerEnvelope);
    cv.internal = Acts::ProtoVolume::InternalStructure{
        Acts::Surface::SurfaceType::Disc,
        {sstripEcBinningR, sstripEcBinningPhi}};
  }

  sstripContainer.container = Acts::ProtoVolume::ContainerStructure{
      {sstripNec, sstripBarrel, sstripPec},
      {Acts::BinningData(Acts::open, Acts::AxisDirection::AxisZ,
                         {-3100., -1200., 1200., 3100.})}};

  // Long Strip section
  Acts::ProtoVolume lstripContainer;
  lstripContainer.name = "odd-lstrip";
  lstripContainer.extent.set(Acts::AxisDirection::AxisR, 720, 1100.);

  Acts::ProtoVolume lstripNec;
  lstripNec.name = "odd-lstrip-nec";
  lstripNec.extent.set(Acts::AxisDirection::AxisZ, -3100., -1200);
  Acts::ProtoVolume lstripNecD5;
  lstripNecD5.name = "odd-lstrip-nec-d5";
  lstripNecD5.extent.set(Acts::AxisDirection::AxisZ, -3050, -2900.);
  Acts::ProtoVolume lstripNecD4;
  lstripNecD4.name = "odd-lstrip-nec-d4";
  lstripNecD4.extent.set(Acts::AxisDirection::AxisZ, -2650., -2500.);
  Acts::ProtoVolume lstripNecD3;
  lstripNecD3.name = "odd-lstrip-nec-d3";
  lstripNecD3.extent.set(Acts::AxisDirection::AxisZ, -2300, -2150.);
  Acts::ProtoVolume lstripNecD2;
  lstripNecD2.name = "odd-lstrip-nec-d2";
  lstripNecD2.extent.set(Acts::AxisDirection::AxisZ, -1950, -1800.);
  Acts::ProtoVolume lstripNecD1;
  lstripNecD1.name = "odd-lstrip-nec-d1";
  lstripNecD1.extent.set(Acts::AxisDirection::AxisZ, -1650., -1500.);
  Acts::ProtoVolume lstripNecD0;
  lstripNecD0.name = "odd-lstrip-nec-d0";
  lstripNecD0.extent.set(Acts::AxisDirection::AxisZ, -1400., -1250.);

  lstripNec.container = Acts::ProtoVolume::ContainerStructure{
      {lstripNecD5, lstripNecD4, lstripNecD3, lstripNecD2, lstripNecD1,
       lstripNecD0},
      {Acts::BinningData(Acts::open, Acts::AxisDirection::AxisZ, {0., 1})},
      true};

  for (auto& cv : lstripNec.container.value().constituentVolumes) {
    cv.extent.setEnvelope(discLayerEnvelope);
    cv.internal =
        Acts::ProtoVolume::InternalStructure{Acts::Surface::SurfaceType::Disc};
  }

  Acts::ProtoVolume lstripBarrel;
  lstripBarrel.name = "odd-lstrip-barrel";
  lstripBarrel.extent.set(Acts::AxisDirection::AxisZ, -1200., 1200);

  Acts::ProtoVolume lstripBarrelL0;
  lstripBarrelL0.name = "odd-lstrip-barrel-l0";
  lstripBarrelL0.extent.set(Acts::AxisDirection::AxisR, 800., 840.);
  Acts::ProtoVolume lstripBarrelL1;
  lstripBarrelL1.name = "odd-lstrip-barrel-l1";
  lstripBarrelL1.extent.set(Acts::AxisDirection::AxisR, 1000., 1050.);

  lstripBarrel.container = Acts::ProtoVolume::ContainerStructure{
      {lstripBarrelL0, lstripBarrelL1},
      {Acts::BinningData(Acts::open, Acts::AxisDirection::AxisR, {0., 1})},
      true};

  for (auto& cv : lstripBarrel.container.value().constituentVolumes) {
    cv.extent.setEnvelope(cylinderLayerEnvelope);
    cv.internal = Acts::ProtoVolume::InternalStructure{
        Acts::Surface::SurfaceType::Cylinder};
  }

  Acts::ProtoVolume lstripPec;
  lstripPec.name = "odd-lstrip-pec";
  lstripPec.extent.set(Acts::AxisDirection::AxisZ, 1200., 3100);

  Acts::ProtoVolume lstripPecD0;
  lstripPecD0.name = "odd-lstrip-pec-d0";
  lstripPecD0.extent.set(Acts::AxisDirection::AxisZ, 1250., 1400);
  Acts::ProtoVolume lstripPecD1;
  lstripPecD1.name = "odd-lstrip-pec-d1";
  lstripPecD1.extent.set(Acts::AxisDirection::AxisZ, 1500., 1650.);
  Acts::ProtoVolume lstripPecD2;
  lstripPecD2.name = "odd-lstrip-pec-d2";
  lstripPecD2.extent.set(Acts::AxisDirection::AxisZ, 1800., 1950.);
  Acts::ProtoVolume lstripPecD3;
  lstripPecD3.name = "odd-lstrip-pec-d3";
  lstripPecD3.extent.set(Acts::AxisDirection::AxisZ, 2150., 2300.);
  Acts::ProtoVolume lstripPecD4;
  lstripPecD4.name = "odd-lstrip-pec-d4";
  lstripPecD4.extent.set(Acts::AxisDirection::AxisZ, 2500., 2650.);
  Acts::ProtoVolume lstripPecD5;
  lstripPecD5.name = "odd-lstrip-pec-d5";
  lstripPecD5.extent.set(Acts::AxisDirection::AxisZ, 2900., 3050.);

  lstripPec.container = Acts::ProtoVolume::ContainerStructure{
      {lstripPecD0, lstripPecD1, lstripPecD2, lstripPecD3, lstripPecD4,
       lstripPecD5},
      {Acts::BinningData(Acts::open, Acts::AxisDirection::AxisZ, {0., 1})},
      true};
  for (auto& cv : lstripPec.container.value().constituentVolumes) {
    cv.internal =
        Acts::ProtoVolume::InternalStructure{Acts::Surface::SurfaceType::Disc};
    cv.extent.setEnvelope(discLayerEnvelope);
  }
  lstripContainer.container = Acts::ProtoVolume::ContainerStructure{
      {lstripNec, lstripBarrel, lstripPec},
      {Acts::BinningData(Acts::open, Acts::AxisDirection::AxisZ,
                         {-3100., -1200., 1200., 3100.})}};

  // The overall container
  Acts::ProtoVolume detectorContainer;
  detectorContainer.name = "odd-light-world";
  detectorContainer.extent.set(Acts::AxisDirection::AxisR, 0., 1100.);
  detectorContainer.extent.set(Acts::AxisDirection::AxisZ, -3100., 3100.);
  detectorContainer.container = Acts::ProtoVolume::ContainerStructure{
      {beamPipeContainer, pixelContainer, pstContainer, sstripContainer,
       lstripContainer},
      {Acts::BinningData(Acts::open, Acts::AxisDirection::AxisR,
                         {0., 25., 200., 210., 720., 1100.})}};

  // ----------------------------------------------------------
  Acts::ProtoDetector detector;
  detector.name = "odd-light";
  detector.worldVolume = detectorContainer;

  // Transform into json
  nlohmann::json jdet;
  jdet["detector"] = detector;

  std::ofstream out;
  out.open("odd-proto-detector.json");
  out << jdet.dump(4);
  out.close();

  Acts::ProtoDetector detectorIn = jdet["detector"];

  // Let's compare
  BOOST_CHECK_EQUAL(detector.name, detectorIn.name);

  const auto& world = detector.worldVolume;
  const auto& worldIn = detectorIn.worldVolume;

  BOOST_CHECK(isEqual(world, worldIn, 0.1));
}

BOOST_AUTO_TEST_SUITE_END()
