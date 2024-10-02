// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/CylindricalContainerBuilder.hpp"
#include "Acts/Detector/Detector.hpp"
#include "Acts/Detector/DetectorBuilder.hpp"
#include "Acts/Detector/DetectorComponents.hpp"
#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Detector/GeometryIdGenerator.hpp"
#include "Acts/Detector/PortalGenerators.hpp"
#include "Acts/Detector/interface/IDetectorComponentBuilder.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Navigation/DetectorVolumeFinders.hpp"
#include "Acts/Navigation/InternalNavigation.hpp"
#include "Acts/Plugins/ActSVG/DetectorSvgConverter.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Tests/CommonHelpers/CylindricalDetector.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Enumerate.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <fstream>
#include <memory>
#include <vector>

using namespace Acts;
using namespace Acts::Test;
using namespace Acts::Experimental;

GeometryContext tContext;

BOOST_AUTO_TEST_SUITE(ActSvg)

BOOST_AUTO_TEST_CASE(CylindricalDetector) {
  auto detector = buildCylindricalDetector(tContext);

  Acts::Svg::DetectorConverter::Options detectorOptions;
  auto pDetector = Acts::Svg::DetectorConverter::convert(tContext, *detector,
                                                         detectorOptions);
  pDetector._name = detector->name();

  // Colorize in blue
  actsvg::style::color gray({{155, 155, 155}});
  actsvg::style::color pink({{255, 153, 255}});
  actsvg::style::color brown({{153, 102, 51}});
  actsvg::style::color red({{255, 0, 0}});
  actsvg::style::color green({{0, 255, 0}});
  actsvg::style::color blue({{0, 0, 255}});
  std::vector<actsvg::style::color> colors = {gray, pink,  brown,
                                              red,  green, blue};
  for (auto& c : colors) {
    c._opacity = 0.1;
  }

  pDetector.colorize(colors);

  // As sheet
  auto dv_zr = Acts::Svg::View::zr(pDetector, pDetector._name);
  Acts::Svg::toFile({dv_zr}, pDetector._name + "_zr.svg");
}

BOOST_AUTO_TEST_SUITE_END()
