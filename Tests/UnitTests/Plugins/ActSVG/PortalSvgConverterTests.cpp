// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Detector/PortalGenerators.hpp"
#include "Acts/Detector/detail/CylindricalDetectorHelper.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Navigation/InternalNavigation.hpp"
#include "Acts/Plugins/ActSVG/PortalSvgConverter.hpp"
#include "Acts/Plugins/ActSVG/SvgUtils.hpp"
#include "Acts/Utilities/Enumerate.hpp"

#include <fstream>
#include <memory>
#include <vector>

Acts::GeometryContext tContext;

BOOST_AUTO_TEST_SUITE(ActSvg)

BOOST_AUTO_TEST_CASE(CylinderPortalsSvg) {
  //  Some style parameteers
  Acts::Svg::Style portalStyle;
  portalStyle.fillColor = {255, 255, 255};
  portalStyle.fillOpacity = 0.;
  portalStyle.highlightColor = {255, 255, 255};
  portalStyle.highlights = {};
  portalStyle.strokeColor = {25, 25, 25};
  portalStyle.strokeWidth = 0.5;
  portalStyle.quarterSegments = 72u;

  Acts::ActsScalar rInner = 10.;
  Acts::ActsScalar rOuter = 100.;
  Acts::ActsScalar zHalfL = 200.;

  Acts::Transform3 nominal = Acts::Transform3::Identity();

  auto cylinderBounds =
      std::make_unique<Acts::CylinderVolumeBounds>(rInner, rOuter, zHalfL);

  auto portalGenerator = Acts::Experimental::defaultPortalGenerator();

  auto cylinderVolume = Acts::Experimental::DetectorVolumeFactory::construct(
      portalGenerator, tContext, "CylinderVolume", nominal,
      std::move(cylinderBounds), Acts::Experimental::tryAllPortals());

  Acts::Svg::PortalConverter::Options portalOptions;
  portalOptions.volumeIndices[cylinderVolume.get()] = 0;
  Acts::Svg::SurfaceConverter::Options surfaceOptions;
  surfaceOptions.style = portalStyle;
  portalOptions.surfaceOptions = surfaceOptions;

  std::vector<Acts::Svg::ProtoPortal> protoPortals;

  std::for_each(cylinderVolume->portals().begin(),
                cylinderVolume->portals().end(), [&](const auto& p) {
                  protoPortals.push_back(Acts::Svg::PortalConverter::convert(
                      tContext, *p, portalOptions));
                });

  // rz view
  std::vector<actsvg::svg::object> zrPortals;
  std::size_t ip = 0;
  std::for_each(protoPortals.begin(), protoPortals.end(), [&](const auto& p) {
    zrPortals.push_back(
        Acts::Svg::View::zr(p, "Portal_zr" + std::to_string(ip++)));
  });
  Acts::Svg::toFile(zrPortals, "SimpleCylinderPortals_ZR.svg");

  // xy view
  std::vector<actsvg::svg::object> xyPortals;
  ip = 0;
  std::for_each(protoPortals.begin(), protoPortals.end(), [&](const auto& p) {
    xyPortals.push_back(
        Acts::Svg::View::xy(p, "Portal_xy" + std::to_string(ip++)));
  });
  Acts::Svg::toFile(xyPortals, "SimpleCylinderPortals_XY.svg");
}

BOOST_AUTO_TEST_CASE(CylinderContainerPortalsSvg) {
  //  Some style parameteers
  Acts::Svg::Style portalStyle;
  portalStyle.fillColor = {255, 255, 255};
  portalStyle.fillOpacity = 0.;
  portalStyle.highlightColor = {255, 255, 255};
  portalStyle.highlights = {};
  portalStyle.strokeColor = {25, 25, 25};
  portalStyle.strokeWidth = 0.5;
  portalStyle.quarterSegments = 72u;

  Acts::ActsScalar rInner = 10.;
  Acts::ActsScalar rMiddle = 100.;
  Acts::ActsScalar rOuter = 300.;
  Acts::ActsScalar zHalfL = 200.;

  Acts::Transform3 nominal = Acts::Transform3::Identity();

  auto portalGenerator = Acts::Experimental::defaultPortalGenerator();

  auto cylinderBoundsI =
      std::make_unique<Acts::CylinderVolumeBounds>(rInner, rMiddle, zHalfL);

  auto cylinderBoundsO =
      std::make_unique<Acts::CylinderVolumeBounds>(rMiddle, rOuter, zHalfL);

  auto cylinderVolumeI = Acts::Experimental::DetectorVolumeFactory::construct(
      portalGenerator, tContext, "CylinderVolumeI", nominal,
      std::move(cylinderBoundsI), Acts::Experimental::tryAllPortals());

  auto cylinderVolumeO = Acts::Experimental::DetectorVolumeFactory::construct(
      portalGenerator, tContext, "CylinderVolumeO", nominal,
      std::move(cylinderBoundsO), Acts::Experimental::tryAllPortals());

  std::vector<std::shared_ptr<Acts::Experimental::DetectorVolume>> rVolumes = {
      cylinderVolumeI, cylinderVolumeO};

  Acts::Experimental::detail::CylindricalDetectorHelper::connectInR(
      tContext, rVolumes, {});

  std::set<const Acts::Experimental::Portal*> portals;
  for (auto& v : rVolumes) {
    for (auto& p : v->portals()) {
      portals.insert(p);
    }
  }
  std::vector<Acts::Svg::ProtoPortal> protoPortals;

  Acts::Svg::PortalConverter::Options portalOptions;
  portalOptions.volumeIndices[cylinderVolumeI.get()] = 0;
  portalOptions.volumeIndices[cylinderVolumeO.get()] = 1;

  Acts::Svg::SurfaceConverter::Options surfaceOptions;
  surfaceOptions.style = portalStyle;
  portalOptions.surfaceOptions = surfaceOptions;

  for (const auto& portal : portals) {
    protoPortals.push_back(
        Acts::Svg::PortalConverter::convert(tContext, *portal, portalOptions));
  }
  // rz view
  std::vector<actsvg::svg::object> zrPortals;
  std::size_t ip = 0;
  std::for_each(protoPortals.begin(), protoPortals.end(), [&](const auto& p) {
    zrPortals.push_back(
        Acts::Svg::View::zr(p, "Portal_zr" + std::to_string(ip++)));
  });
  Acts::Svg::toFile(zrPortals, "CylinderContainerPortals_ZR.svg");
}

BOOST_AUTO_TEST_SUITE_END()
