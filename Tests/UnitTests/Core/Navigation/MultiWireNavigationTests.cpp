// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/Detector.hpp"
#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Detector/MultiWireStructureBuilder.hpp"
#include "Acts/Navigation/DetectorVolumeFinders.hpp"
#include "Acts/Navigation/InternalNavigation.hpp"
#include "Acts/Navigation/NavigationState.hpp"
#include "Acts/Navigation/NavigationStateFillers.hpp"
#include "Acts/Navigation/NavigationStateUpdaters.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/StrawSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Grid.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"

#include <fstream>
#include <memory>
#include <string>
#include <vector>

using namespace Acts;
using namespace Acts::Experimental;
using namespace Acts::detail;

GeometryContext tContext;

BOOST_AUTO_TEST_SUITE(Experimental)

BOOST_AUTO_TEST_CASE(Navigation_in_Indexed_Surfaces) {
  std::vector<std::shared_ptr<Acts::Surface>> strawSurfaces = {};

  // Set the number of surfaces along each dimension of the multi wire structure
  // aligned along z axis
  std::size_t nSurfacesY = 4;
  std::size_t nSurfacesX = 15;

  double radius = 15.;
  double halfZ = 250.;

  // The transform of the 1st surface
  Vector3 ipos = {-0.5 * nSurfacesX * 2 * radius + radius,
                  -0.5 * nSurfacesY * 2 * radius + radius, 0.};

  Vector3 pos = ipos;

  // Generate the surfaces
  for (std::size_t i = 0; i < nSurfacesY; i++) {
    for (std::size_t j = 0; j < nSurfacesX; j++) {
      pos.x() = ipos.x() + 2 * j * radius;

      auto surface = Surface::makeShared<StrawSurface>(
          Transform3(Translation3(pos)), radius, halfZ);

      strawSurfaces.push_back(surface);
    }

    pos.y() = ipos.y() + 2 * (i + 1) * radius;
  }

  std::vector<ActsScalar> vBounds = {0.5 * nSurfacesX * 2 * radius,
                                     0.5 * nSurfacesX * 2 * radius,
                                     0.5 * nSurfacesY * 2 * radius, halfZ};

  MultiWireStructureBuilder::Config mlCfg;
  mlCfg.name = "Multi_Layer_With_Wires";
  mlCfg.mlSurfaces = strawSurfaces;

  mlCfg.mlBinning = {
      ProtoBinning(Acts::BinningValue::binX, Acts::AxisBoundaryType::Bound,
                   -vBounds[0], vBounds[0], nSurfacesX, 1u),
      ProtoBinning(Acts::BinningValue::binY, Acts::AxisBoundaryType::Bound,
                   -vBounds[1], vBounds[1], nSurfacesY, 0u)};
  mlCfg.mlBounds = vBounds;

  MultiWireStructureBuilder mlBuilder(mlCfg);
  auto [volumes, portals, roots] = mlBuilder.construct(tContext);

  Acts::Experimental::NavigationState nState;
  nState.position = Acts::Vector3(0., -60., 0.);
  nState.direction = Acts::Vector3(0., 1., 0.);

  nState.currentVolume = volumes.front().get();
  nState.currentVolume->updateNavigationState(tContext, nState);

  // check the surface candidates after update (12 surfaces + 6 portals but only
  // 5 are reachable, but one excluded due to new > s_onTolerance rule)
  BOOST_CHECK_EQUAL(nState.surfaceCandidates.size(), 4u);
}

BOOST_AUTO_TEST_SUITE_END()
