// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Direction.hpp"
#include "Acts/Geometry/GenericCuboidVolumeBounds.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Surfaces/PlanarBounds.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/BoundingBox.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "Acts/Visualization/IVisualization3D.hpp"
#include "Acts/Visualization/PlyVisualization3D.hpp"

#include <array>
#include <cmath>
#include <fstream>
#include <memory>
#include <utility>
#include <vector>

namespace Acts::Test {

GeometryContext gctx = GeometryContext();

BOOST_AUTO_TEST_SUITE(Volumes)

BOOST_AUTO_TEST_CASE(construction_test) {
  std::array<Vector3, 8> vertices;
  vertices = {{{0, 0, 0},
               {2, 0, 0},
               {2, 1, 0},
               {0, 1, 0},
               {0, 0, 1},
               {2, 0, 1},
               {2, 1, 1},
               {0, 1, 1}}};
  GenericCuboidVolumeBounds cubo(vertices);

  BOOST_CHECK(cubo.inside({0.5, 0.5, 0.5}));
  BOOST_CHECK(cubo.inside({1.5, 0.5, 0.5}));
  BOOST_CHECK(!cubo.inside({2.5, 0.5, 0.5}));
  BOOST_CHECK(!cubo.inside({0.5, 1.5, 0.5}));
  BOOST_CHECK(!cubo.inside({0.5, 0.5, 1.5}));
  BOOST_CHECK(!cubo.inside({-0.5, 0.5, 0.5}));

  BOOST_CHECK(!cubo.inside({2.2, 1, 1}, 0.1));
  BOOST_CHECK(cubo.inside({2.2, 1, 1}, 0.21));
  BOOST_CHECK(cubo.inside({2.2, 1, 1}, 0.3));
}

BOOST_AUTO_TEST_CASE(GenericCuboidBoundsOrientedSurfaces) {
  std::array<Vector3, 8> vertices;
  vertices = {{{0, 0, 0},
               {2, 0, 0},
               {2, 1, 0},
               {0, 1, 0},
               {0, 0, 1},
               {2, 0, 1},
               {2, 1, 1},
               {0, 1, 1}}};
  GenericCuboidVolumeBounds cubo(vertices);

  auto is_in = [](const auto& tvtx, const auto& vertices_) {
    for (const auto& vtx : vertices_) {
      if (checkCloseAbs(vtx, tvtx, 1e-9)) {
        return true;
      }
    }
    return false;
  };

  auto surfaces = cubo.orientedSurfaces(Transform3::Identity());
  for (const auto& srf : surfaces) {
    auto pbounds = dynamic_cast<const PlanarBounds*>(&srf.surface->bounds());
    for (const auto& vtx : pbounds->vertices()) {
      Vector3 glob = srf.surface->localToGlobal(gctx, vtx, {});
      // check if glob is in actual vertex list
      BOOST_CHECK(is_in(glob, vertices));
    }
  }

  vertices = {{{0, 0, 0},
               {2, 0, 0.4},
               {2, 1, 0.4},
               {0, 1, 0},
               {0, 0, 1},
               {1.8, 0, 1},
               {1.8, 1, 1},
               {0, 1, 1}}};
  cubo = GenericCuboidVolumeBounds(vertices);

  surfaces = cubo.orientedSurfaces(Transform3::Identity());
  for (const auto& srf : surfaces) {
    auto pbounds = dynamic_cast<const PlanarBounds*>(&srf.surface->bounds());
    for (const auto& vtx : pbounds->vertices()) {
      Vector3 glob = srf.surface->localToGlobal(gctx, vtx, {});
      // check if glob is in actual vertex list
      BOOST_CHECK(is_in(glob, vertices));
    }
  }

  Transform3 trf;
  trf = Translation3(Vector3(0, 8, -5)) *
        AngleAxis3(M_PI / 3., Vector3(1, -3, 9).normalized());

  surfaces = cubo.orientedSurfaces(trf);
  for (const auto& srf : surfaces) {
    auto pbounds = dynamic_cast<const PlanarBounds*>(&srf.surface->bounds());
    for (const auto& vtx : pbounds->vertices()) {
      Vector3 glob = srf.surface->localToGlobal(gctx, vtx, {});
      // check if glob is in actual vertex list
      BOOST_CHECK(is_in(trf.inverse() * glob, vertices));
    }
  }
}

BOOST_AUTO_TEST_CASE(ply_test) {
  std::array<Vector3, 8> vertices;
  vertices = {{{0, 0, 0},
               {2, 0, 0},
               {2, 1, 0},
               {0, 1, 0},
               {0, 0, 1},
               {2, 0, 1},
               {2, 1, 1},
               {0, 1, 1}}};
  GenericCuboidVolumeBounds cubo(vertices);
  PlyVisualization3D<double> ply;
  cubo.draw(ply);

  std::ofstream os("cuboid.ply");
  os << ply << std::flush;
  os.close();
}

BOOST_AUTO_TEST_CASE(bounding_box_creation) {
  float tol = 1e-4;
  std::array<Vector3, 8> vertices;
  vertices = {{{0, 0, 0},
               {2, 0, 0.4},
               {2, 1, 0.4},
               {0, 1, 0},
               {0, 0, 1},
               {1.8, 0, 1},
               {1.8, 1, 1},
               {0, 1, 1}}};

  GenericCuboidVolumeBounds gcvb(vertices);
  auto bb = gcvb.boundingBox();

  Transform3 rot;
  rot = AngleAxis3(M_PI / 2., Vector3::UnitX());

  BOOST_CHECK_EQUAL(bb.entity(), nullptr);
  BOOST_CHECK_EQUAL(bb.max(), Vector3(2, 1, 1));
  BOOST_CHECK_EQUAL(bb.min(), Vector3(0., 0., 0.));

  bb = gcvb.boundingBox(&rot);

  BOOST_CHECK_EQUAL(bb.entity(), nullptr);
  CHECK_CLOSE_ABS(bb.max(), Vector3(2, 0, 1), tol);
  BOOST_CHECK_EQUAL(bb.min(), Vector3(0, -1, 0));

  rot = AngleAxis3(M_PI / 2., Vector3::UnitZ());
  bb = gcvb.boundingBox(&rot);
  BOOST_CHECK_EQUAL(bb.entity(), nullptr);
  CHECK_CLOSE_ABS(bb.max(), Vector3(0, 2, 1), tol);
  CHECK_CLOSE_ABS(bb.min(), Vector3(-1, 0., 0.), tol);

  rot = AngleAxis3(0.542, Vector3::UnitZ()) *
        AngleAxis3(M_PI / 5., Vector3(1, 3, 6).normalized());

  bb = gcvb.boundingBox(&rot);
  BOOST_CHECK_EQUAL(bb.entity(), nullptr);
  CHECK_CLOSE_ABS(bb.max(), Vector3(1.00976, 2.26918, 1.11988), tol);
  CHECK_CLOSE_ABS(bb.min(), Vector3(-0.871397, 0, -0.0867708), tol);

  // Check recreation from bound values
  const auto boundValues = gcvb.values();
  BOOST_CHECK_EQUAL(boundValues.size(), 24u);

  auto bValueArrray =
      toArray<GenericCuboidVolumeBounds::BoundValues::eSize, ActsScalar>(
          boundValues);
  GenericCuboidVolumeBounds gcvbCopy(bValueArrray);
  BOOST_CHECK_EQUAL(gcvbCopy.values().size(), 24u);

  // Redo the check from above
  rot = AngleAxis3(0.542, Vector3::UnitZ()) *
        AngleAxis3(M_PI / 5., Vector3(1, 3, 6).normalized());

  bb = gcvbCopy.boundingBox(&rot);
  BOOST_CHECK_EQUAL(bb.entity(), nullptr);
  CHECK_CLOSE_ABS(bb.max(), Vector3(1.00976, 2.26918, 1.11988), tol);
  CHECK_CLOSE_ABS(bb.min(), Vector3(-0.871397, 0, -0.0867708), tol);
}

BOOST_AUTO_TEST_CASE(GenericCuboidVolumeBoundarySurfaces) {
  std::array<Vector3, 8> vertices;
  vertices = {{{0, 0, 0},
               {4, 0, 0},
               {4, 2, 0},
               {0, 2, 0},
               {0, 0, 2},
               {4, 0, 2},
               {4, 2, 2},
               {0, 2, 2}}};

  GenericCuboidVolumeBounds cubo(vertices);

  auto gcvbOrientedSurfaces = cubo.orientedSurfaces(Transform3::Identity());
  BOOST_CHECK_EQUAL(gcvbOrientedSurfaces.size(), 6);

  for (auto& os : gcvbOrientedSurfaces) {
    auto geoCtx = GeometryContext();
    auto osCenter = os.surface->center(geoCtx);
    const auto* pSurface =
        dynamic_cast<const Acts::PlaneSurface*>(os.surface.get());
    BOOST_REQUIRE_MESSAGE(pSurface != nullptr,
                          "The surface is not a plane surface");
    auto osNormal = pSurface->normal(geoCtx);
    // Check if you step inside the volume with the oriented normal
    Vector3 insideGcvb = osCenter + os.direction * osNormal;
    Vector3 outsideGcvb = osCenter - os.direction * osNormal;
    BOOST_CHECK(cubo.inside(insideGcvb));
    BOOST_CHECK(!cubo.inside(outsideGcvb));
  }
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace Acts::Test
