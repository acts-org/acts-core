// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Surfaces/AnnulusBounds.hpp"
#include "Acts/Surfaces/ConeBounds.hpp"
#include "Acts/Surfaces/ConeSurface.hpp"
#include "Acts/Surfaces/ConvexPolygonBounds.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/DiamondBounds.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/DiscTrapezoidBounds.hpp"
#include "Acts/Surfaces/EllipseBounds.hpp"
#include "Acts/Surfaces/LineBounds.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/StrawSurface.hpp"
#include "Acts/Surfaces/TrapezoidBounds.hpp"
#include "Acts/Visualization/GeometryView3D.hpp"
#include "Acts/Visualization/IVisualization3D.hpp"

#include <fstream>
#include <numbers>
#include <sstream>
#include <string>

namespace Acts::SurfaceView3DTest {

/// Helper method to visualize all types of surfaces
///
/// @param helper The visualization helper
/// @param triangulate The directive whether to create triangular meshes
/// @param tag The test tag (mode) identification
///
/// @return an overall string including all written output
static inline std::string run(IVisualization3D& helper, bool triangulate,
                              const std::string& tag) {
  auto gctx = GeometryContext();
  auto identity = Transform3::Identity();
  std::stringstream cStream;

  const double halfPhiSector = std::numbers::pi / 4.;
  const double centralPhi = std::numbers::pi / 2.;

  ViewConfig sConfig = s_viewSensitive;
  sConfig.triangulate = triangulate;

  //----------------------------------------------------
  // Cone Surface section
  std::vector<std::shared_ptr<ConeSurface>> coneSurfaces;

  double coneAlpha = 0.245;
  double coneMinZ = 0.;
  double coneCutZ = 2.;
  double coneMaxZ = 10.;
  // Full Cone
  auto coneBounds =
      std::make_shared<ConeBounds>(coneAlpha, -coneCutZ, coneMaxZ);
  auto cone = Surface::makeShared<ConeSurface>(identity, coneBounds);
  coneSurfaces.push_back(cone);
  GeometryView3D::drawSurface(helper, *cone, gctx, Transform3::Identity(),
                              sConfig);

  helper.write(std::string("Surfaces_ConeSurface") + tag);
  helper.write(cStream);
  helper.clear();

  // Sectoral Cone
  coneBounds = std::make_shared<ConeBounds>(coneAlpha, coneMinZ, coneMaxZ,
                                            halfPhiSector);
  cone = Surface::makeShared<ConeSurface>(identity, coneBounds);
  coneSurfaces.push_back(cone);
  GeometryView3D::drawSurface(helper, *cone, gctx, Transform3::Identity(),
                              sConfig);

  helper.write(std::string("Surfaces_ConeSurfaceSector") + tag);
  helper.write(cStream);
  helper.clear();

  // Sectoral Cone Shifted
  coneBounds = std::make_shared<ConeBounds>(coneAlpha, coneCutZ, coneMaxZ,
                                            halfPhiSector, centralPhi);
  cone = Surface::makeShared<ConeSurface>(identity, coneBounds);
  coneSurfaces.push_back(cone);
  GeometryView3D::drawSurface(helper, *cone, gctx, Transform3::Identity(),
                              sConfig);

  helper.write(std::string("Surfaces_ConeSurfaceSectorShifted") + tag);
  helper.write(cStream);
  helper.clear();

  // All in one for radial bounds
  std::vector<Transform3> threeCones = {
      Transform3(Translation3{-0.5 * coneMaxZ, 0., 0.}),
      Transform3(Translation3{0., 0., 0.}),
      Transform3(Translation3{0.75 * coneMaxZ, 0., 0.})};

  for (std::size_t ic = 0; ic < coneSurfaces.size(); ++ic) {
    GeometryView3D::drawSurface(helper, *coneSurfaces[ic], gctx, threeCones[ic],
                                sConfig);
  }
  helper.write(std::string("Surfaces_All_ConeSurfaces") + tag);
  helper.write(cStream);
  helper.clear();

  //----------------------------------------------------
  // Cylinder surface section
  std::vector<std::shared_ptr<CylinderSurface>> cylinderSurfaces;

  double cylinderRadius = 5.;
  double cylinderHalfZ = 10.;

  // Full Cylinder
  auto cylinderBounds =
      std::make_shared<CylinderBounds>(cylinderRadius, cylinderHalfZ);
  auto cylinder =
      Surface::makeShared<CylinderSurface>(identity, cylinderBounds);
  cylinderSurfaces.push_back(cylinder);
  GeometryView3D::drawSurface(helper, *cylinder, gctx, Transform3::Identity(),
                              sConfig);

  helper.write(std::string("Surfaces_CylinderSurface") + tag);
  helper.write(cStream);
  helper.clear();

  // Sectoral Cone
  cylinderBounds = std::make_shared<CylinderBounds>(
      cylinderRadius, cylinderHalfZ, halfPhiSector);
  cylinder = Surface::makeShared<CylinderSurface>(identity, cylinderBounds);
  cylinderSurfaces.push_back(cylinder);
  GeometryView3D::drawSurface(helper, *cylinder, gctx, Transform3::Identity(),
                              sConfig);

  helper.write(std::string("Surfaces_CylinderSurfaceSector") + tag);
  helper.write(cStream);
  helper.clear();

  // Sectoral Cone Shifted
  cylinderBounds = std::make_shared<CylinderBounds>(
      cylinderRadius, cylinderHalfZ, halfPhiSector, centralPhi);
  cylinder = Surface::makeShared<CylinderSurface>(identity, cylinderBounds);
  cylinderSurfaces.push_back(cylinder);
  GeometryView3D::drawSurface(helper, *cylinder, gctx, Transform3::Identity(),
                              sConfig);

  helper.write(std::string("Surfaces_CylinderSurfaceSectorShifted") + tag);
  helper.write(cStream);
  helper.clear();

  // All in one for radial bounds
  std::vector<Transform3> threeCylinders = {
      Transform3(Translation3{-2 * cylinderRadius, 0., 0.}),
      Transform3(Translation3{0., 0., 0.}),
      Transform3(Translation3{2.5 * cylinderRadius, 0., 0.})};

  for (std::size_t ic = 0; ic < cylinderSurfaces.size(); ++ic) {
    GeometryView3D::drawSurface(helper, *cylinderSurfaces[ic], gctx,
                                threeCylinders[ic], sConfig);
  }
  helper.write(std::string("Surfaces_All_CylinderSurfaces") + tag);
  helper.write(cStream);
  helper.clear();

  /// ------------- planar bounding box
  /// @param name of the file
  auto writeBoundingBox2D = [&](const RectangleBounds& rBounds,
                                const std::string& path) -> void {
    std::string bbPath = path + tag + "_bbox";

    auto bbBounds = std::make_shared<RectangleBounds>(rBounds);
    auto bbSurface = Surface::makeShared<PlaneSurface>(identity, bbBounds);
    GeometryView3D::drawSurface(helper, *bbSurface, gctx,
                                Transform3::Identity(), sConfig);

    helper.write(bbPath);
    helper.write(cStream);
    helper.clear();
  };

  //----------------------------------------------------
  // Disc Surface section

  double discRmin = 5.;
  double discRmax = 10.;

  std::vector<std::shared_ptr<DiscSurface>> radialSurfaces;

  // Full Disc
  auto radialBounds = std::make_shared<RadialBounds>(0., discRmax);
  auto disc = Surface::makeShared<DiscSurface>(identity, radialBounds);
  radialSurfaces.push_back(disc);
  GeometryView3D::drawSurface(helper, *disc, gctx, Transform3::Identity(),
                              sConfig);

  helper.write(std::string("Surfaces_DiscSurfaceFull") + tag);
  helper.write(cStream);
  helper.clear();

  // Full Sectoral Disc
  radialBounds = std::make_shared<RadialBounds>(0., discRmax, halfPhiSector);
  disc = Surface::makeShared<DiscSurface>(identity, radialBounds);
  radialSurfaces.push_back(disc);
  GeometryView3D::drawSurface(helper, *disc, gctx, Transform3::Identity(),
                              sConfig);

  helper.write(std::string("Surfaces_DiscSurfaceFullSector") + tag);
  helper.write(cStream);
  helper.clear();

  // Full Sectoral Shifted Disc
  radialBounds =
      std::make_shared<RadialBounds>(0., discRmax, halfPhiSector, centralPhi);
  disc = Surface::makeShared<DiscSurface>(identity, radialBounds);
  radialSurfaces.push_back(disc);
  GeometryView3D::drawSurface(helper, *disc, gctx, Transform3::Identity(),
                              sConfig);

  helper.write(std::string("Surfaces_DiscSurfaceFullSectorShifted") + tag);
  helper.write(cStream);
  helper.clear();

  // Full Ring
  radialBounds = std::make_shared<RadialBounds>(discRmin, discRmax);
  disc = Surface::makeShared<DiscSurface>(identity, radialBounds);
  radialSurfaces.push_back(disc);
  GeometryView3D::drawSurface(helper, *disc, gctx, Transform3::Identity(),
                              sConfig);

  helper.write(std::string("Surfaces_DiscSurfaceRing") + tag);
  helper.write(cStream);
  helper.clear();

  // Full Sectoral Rin g
  radialBounds =
      std::make_shared<RadialBounds>(discRmin, discRmax, halfPhiSector);
  disc = Surface::makeShared<DiscSurface>(identity, radialBounds);
  radialSurfaces.push_back(disc);
  GeometryView3D::drawSurface(helper, *disc, gctx, Transform3::Identity(),
                              sConfig);

  helper.write(std::string("Surfaces_DiscSurfaceRingSector") + tag);
  helper.write(cStream);
  helper.clear();

  // Full Sectoral Shifted Ring
  radialBounds = std::make_shared<RadialBounds>(discRmin, discRmax,
                                                halfPhiSector, centralPhi);
  disc = Surface::makeShared<DiscSurface>(identity, radialBounds);
  radialSurfaces.push_back(disc);
  GeometryView3D::drawSurface(helper, *disc, gctx, Transform3::Identity(),
                              sConfig);

  helper.write(std::string("Surfaces_DiscSurfaceRingSectorShifted") + tag);
  helper.write(cStream);
  helper.clear();

  // All in one for radial bounds
  std::vector<Transform3> sixDiscs = {
      Transform3(Translation3{-2.0 * discRmax, 1.5 * discRmax, 0.}),
      Transform3(Translation3{0., 1.5 * discRmax, 0.}),
      Transform3(Translation3{2.5 * discRmax, 1.5 * discRmax, 0.}),
      Transform3(Translation3{-2.0 * discRmax, -1.5 * discRmax, 0.}),
      Transform3(Translation3{0., -1.5 * discRmax, 0.}),
      Transform3(Translation3{2.5 * discRmax, -1.5 * discRmax, 0.})};
  for (std::size_t ir = 0; ir < radialSurfaces.size(); ++ir) {
    GeometryView3D::drawSurface(helper, *radialSurfaces[ir], gctx, sixDiscs[ir],
                                sConfig);
  }
  helper.write(std::string("Surfaces_All_DiscSurfaces_RadialBounds") + tag);
  helper.write(cStream);
  helper.clear();

  std::vector<std::shared_ptr<DiscSurface>> anomalDiscSurfaces;

  double annulusMinPhi = 0.75;
  double annulusMaxPhi = 1.35;
  Vector2 offset(-4., 2.);
  auto annulus = std::make_shared<AnnulusBounds>(
      discRmin, discRmax, annulusMinPhi, annulusMaxPhi, offset);
  disc = Surface::makeShared<DiscSurface>(identity, annulus);
  anomalDiscSurfaces.push_back(disc);
  GeometryView3D::drawSurface(helper, *disc, gctx, Transform3::Identity(),
                              sConfig);

  helper.write(std::string("Surfaces_DiscAnnulusBounds") + tag);
  helper.write(cStream);
  helper.clear();

  double discTrapezoidHxRmin = 3.;
  double discTrapezoidHxRmax = 6.;
  auto discTrapezoid = std::make_shared<DiscTrapezoidBounds>(
      discTrapezoidHxRmin, discTrapezoidHxRmax, discRmin, discRmax);
  disc = Surface::makeShared<DiscSurface>(identity, discTrapezoid);
  anomalDiscSurfaces.push_back(disc);
  GeometryView3D::drawSurface(helper, *disc, gctx, Transform3::Identity(),
                              sConfig);

  helper.write(std::string("Surfaces_DiscTrapezoidBounds") + tag);
  helper.write(cStream);
  helper.clear();

  // All in one for radial bounds
  std::vector<Transform3> twoAnomalDiscs = {
      Transform3(Translation3{-5., 0., 0.}),
      Transform3(Translation3{5., 0., 0.})};
  for (std::size_t id = 0; id < anomalDiscSurfaces.size(); ++id) {
    GeometryView3D::drawSurface(helper, *anomalDiscSurfaces[id], gctx,
                                sixDiscs[id], sConfig);
  }
  helper.write(std::string("Surfaces_All_DiscSurfaces_AnomalBounds") + tag);
  helper.write(cStream);
  helper.clear();

  //----------------------------------------------------
  // Plane Surface section
  std::vector<std::shared_ptr<PlaneSurface>> planarSurfaces;

  // Ellipse shaped : Full Ellipse
  double ellipseR0min = 2;
  double ellipseR0max = 4;
  double ellipseR1min = 3;
  double ellipseR1max = 6;
  std::string name = "Surfaces_PlaneSurfaceEllipse";
  auto ellipse =
      std::make_shared<EllipseBounds>(0., 0., ellipseR1min, ellipseR1max);
  auto plane = Surface::makeShared<PlaneSurface>(identity, ellipse);
  planarSurfaces.push_back(plane);
  GeometryView3D::drawSurface(helper, *plane, gctx, Transform3::Identity(),
                              sConfig);

  helper.write(name + tag);
  helper.write(cStream);
  helper.clear();
  writeBoundingBox2D(ellipse->boundingBox(), name);

  // Ellipse shaped : Ring Ellipse
  name = "Surfaces_PlaneSurfaceEllipseRing";
  ellipse = std::make_shared<EllipseBounds>(ellipseR0min, ellipseR0max,
                                            ellipseR1min, ellipseR1max);
  plane = Surface::makeShared<PlaneSurface>(identity, ellipse);
  planarSurfaces.push_back(plane);
  GeometryView3D::drawSurface(helper, *plane, gctx, Transform3::Identity(),
                              sConfig);

  helper.write(name + tag);
  helper.write(cStream);
  helper.clear();
  writeBoundingBox2D(ellipse->boundingBox(), name);

  // Ellipse shaped : Ring Ellipse Sector
  name = "Surfaces_PlaneSurfaceEllipseRingSector";
  ellipse = std::make_shared<EllipseBounds>(
      ellipseR0min, ellipseR0max, ellipseR1min, ellipseR1max, halfPhiSector);
  plane = Surface::makeShared<PlaneSurface>(identity, ellipse);
  planarSurfaces.push_back(plane);
  GeometryView3D::drawSurface(helper, *plane, gctx, Transform3::Identity(),
                              sConfig);

  helper.write(name + tag);
  helper.write(cStream);
  helper.clear();
  writeBoundingBox2D(ellipse->boundingBox(), name);

  // ConvexPolygon shaped example: Triangle
  name = "Surfaces_PlaneSurfaceTriangleRegular";
  std::vector<Vector2> tvertices = {{-3, -1.5}, {3, -1.5}, {0, 4.5}};
  auto triangle = std::make_shared<ConvexPolygonBounds<3>>(tvertices);
  plane = Surface::makeShared<PlaneSurface>(identity, triangle);
  planarSurfaces.push_back(plane);
  GeometryView3D::drawSurface(helper, *plane, gctx, Transform3::Identity(),
                              sConfig);

  helper.write(name + tag);
  helper.write(cStream);
  helper.clear();
  writeBoundingBox2D(triangle->boundingBox(), name);

  // ConvexPolygon shaped example: Triangle
  name = "Surfaces_PlaneSurfaceTriangleGeneral";
  tvertices = {{-1., 4.5}, {4, 6.5}, {3, 8.5}};
  triangle = std::make_shared<ConvexPolygonBounds<3>>(tvertices);
  plane = Surface::makeShared<PlaneSurface>(identity, triangle);
  planarSurfaces.push_back(plane);
  GeometryView3D::drawSurface(helper, *plane, gctx, Transform3::Identity(),
                              sConfig);

  helper.write(name + tag);
  helper.write(cStream);
  helper.clear();
  writeBoundingBox2D(triangle->boundingBox(), name);

  // ConvexPolygon shaped example: Triangle
  name = "Surfaces_PlaneSurfaceConvexPolygonGeneral";
  tvertices = {{-1., 4.5}, {4, 6.5}, {6, 8.5}, {0, 10.5}, {-3, 6.2}};
  auto dynamicpolygon =
      std::make_shared<ConvexPolygonBounds<PolygonDynamic>>(tvertices);
  plane = Surface::makeShared<PlaneSurface>(identity, dynamicpolygon);
  planarSurfaces.push_back(plane);
  GeometryView3D::drawSurface(helper, *plane, gctx, Transform3::Identity(),
                              sConfig);

  helper.write(name + tag);
  helper.write(cStream);
  helper.clear();
  writeBoundingBox2D(dynamicpolygon->boundingBox(), name);

  // Diamond shaped
  name = "Surfaces_PlaneSurfaceDiamond";
  auto diamond = std::make_shared<DiamondBounds>(3., 6., 2., 2., 4.);
  plane = Surface::makeShared<PlaneSurface>(identity, diamond);
  planarSurfaces.push_back(plane);
  GeometryView3D::drawSurface(helper, *plane, gctx, Transform3::Identity(),
                              sConfig);

  helper.write(name + tag);
  helper.write(cStream);
  helper.clear();
  writeBoundingBox2D(diamond->boundingBox(), name);

  // Rectangle plane
  name = "Surfaces_PlaneSurfaceRectangle";
  auto rectangle = std::make_shared<RectangleBounds>(2., 3.);
  plane = Surface::makeShared<PlaneSurface>(identity, rectangle);
  planarSurfaces.push_back(plane);
  GeometryView3D::drawSurface(helper, *plane, gctx, Transform3::Identity(),
                              sConfig);

  helper.write(name + tag);
  helper.write(cStream);
  helper.clear();
  writeBoundingBox2D(rectangle->boundingBox(), name);

  // Off-centered Rectangle plane:
  name = "Surfaces_PlaneSurfaceRectangleOffcentered";
  rectangle =
      std::make_shared<RectangleBounds>(Vector2{1., 2.}, Vector2{15., 12.});
  plane = Surface::makeShared<PlaneSurface>(identity, rectangle);
  GeometryView3D::drawSurface(helper, *plane, gctx, Transform3::Identity(),
                              sConfig);

  helper.write(name + tag);
  helper.write(cStream);
  helper.clear();
  writeBoundingBox2D(rectangle->boundingBox(), name);

  // Trapezoidal plane:
  name = "Surfaces_PlaneSurfaceTrapezoid";
  auto trapezoid = std::make_shared<TrapezoidBounds>(2., 5., 3.);
  plane = Surface::makeShared<PlaneSurface>(identity, trapezoid);
  planarSurfaces.push_back(plane);
  GeometryView3D::drawSurface(helper, *plane, gctx, Transform3::Identity(),
                              sConfig);

  helper.write(name + tag);
  helper.write(cStream);
  helper.clear();
  writeBoundingBox2D(trapezoid->boundingBox(), name);

  // All planes
  std::vector<Transform3> ninePlanes = {
      Transform3(Translation3{-10., -10., 0.}),
      Transform3(Translation3{0., -10., 0.}),
      Transform3(Translation3{10., -10., 0.}),
      Transform3(Translation3{-10., 0., 0.}),
      Transform3(Translation3{0., -6., 0.}),
      Transform3(Translation3{10., -8., 0.}),
      Transform3(Translation3{-10, 10., 0.}),
      Transform3(Translation3{0., 10., 0.}),
      Transform3(Translation3{10., 10., 0.})};
  for (std::size_t ip = 0; ip < planarSurfaces.size(); ++ip) {
    GeometryView3D::drawSurface(helper, *planarSurfaces[ip], gctx,
                                ninePlanes[ip], sConfig);
  }
  helper.write(std::string("Surfaces_All_PlaneSurfaces") + tag);
  helper.write(cStream);
  helper.clear();

  //----------------------------------------------------
  // Straw Surface section
  name = "Surfaces_StrawSurface";
  auto tube = std::make_shared<LineBounds>(2., 20.);
  auto straw = Surface::makeShared<StrawSurface>(identity, tube);
  GeometryView3D::drawSurface(helper, *straw, gctx, Transform3::Identity(),
                              sConfig);

  helper.write(name + tag);
  helper.write(cStream);
  helper.clear();

  return cStream.str();
}

}  // namespace Acts::SurfaceView3DTest
