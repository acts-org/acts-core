// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/MultiWireStructureBuilder.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Surfaces/LineBounds.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/StrawSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Visualization/GeometryView3D.hpp"
#include "Acts/Visualization/IVisualization3D.hpp"
#include "Acts/Visualization/ObjVisualization3D.hpp"

#include <algorithm>
#include <iterator>
#include <memory>
#include <numbers>
#include <utility>
#include <vector>

using namespace Acts;
using namespace Acts::Experimental;

GeometryContext tContext;

BOOST_AUTO_TEST_SUITE(Detector)

BOOST_AUTO_TEST_CASE(Multi_Wire_Structure_Builder_StrawSurfacesCreation) {
  // Create the surfaces of the multiwire structure-straw surfaces
  std::vector<std::shared_ptr<Acts::Surface>> strawSurfaces = {};

  // Set the number of surfaces along each dimension of the multi wire structure
  // aligned along z axis
  std::size_t nSurfacesY = 3;
  std::size_t nSurfacesX = 15;

  double radius = 15.;
  double halfZ = 250.;

  // The transform of the 1st surface
  Vector3 ipos = {-0.5 * nSurfacesX * 2 * radius + radius,
                  -0.5 * nSurfacesY * 2 * radius + radius, 0.};
  AngleAxis3 rotation(std::numbers::pi / 2., Acts::Vector3(0., 1., 0.));

  Vector3 pos = ipos;

  // Generate the surfaces
  for (std::size_t i = 0; i < nSurfacesY; i++) {
    for (std::size_t j = 0; j < nSurfacesX; j++) {
      auto surface = Surface::makeShared<StrawSurface>(
          Transform3(Translation3(pos)), radius, halfZ);
      strawSurfaces.push_back(surface);
      pos.x() = ipos.x() + 2 * j * radius;
      pos.y() = ipos.y() + 2 * i * radius;
    }
  }

  std::vector<double> vBounds = {0.5 * nSurfacesX * 2 * radius,
                                 0.5 * nSurfacesX * 2 * radius,
                                 0.5 * nSurfacesY * 2 * radius, halfZ};

  MultiWireStructureBuilder::Config mlCfg;
  mlCfg.name = "Multi_Layer_With_Wires";
  mlCfg.mlSurfaces = strawSurfaces;
  mlCfg.mlBounds = vBounds;
  mlCfg.mlBinning = {
      std::make_pair(
          ProtoAxis(Acts::AxisDirection::AxisX, Acts::AxisBoundaryType::Bound,
                    -vBounds[0], vBounds[0], nSurfacesX),
          1u),
      std::make_pair(
          ProtoAxis(Acts::AxisDirection::AxisY, Acts::AxisBoundaryType::Bound,
                    -vBounds[1], vBounds[1], nSurfacesY),
          0u)};

  MultiWireStructureBuilder mlBuilder(mlCfg);
  auto [volumes, portals, roots] = mlBuilder.construct(tContext);

  BOOST_CHECK_EQUAL(volumes.size(), 1u);
  BOOST_CHECK_EQUAL(volumes.front()->surfaces().size(),
                    nSurfacesX * nSurfacesY);
  BOOST_CHECK(volumes.front()->volumes().empty());
  BOOST_CHECK(volumes.front()->internalNavigation().connected());
}

BOOST_AUTO_TEST_SUITE_END()
