// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>

#include "Acts/Plugins/GeoModel/GeoModelDetectorObjectFactory.hpp"
#include "Acts/Plugins/GeoModel/GeoModelReader.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/DiamondBounds.hpp"
#include "Acts/Surfaces/LineBounds.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/StrawSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/TrapezoidBounds.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <typeinfo>

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoFullPhysVol.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoMaterial.h>
#include <GeoModelKernel/GeoTrd.h>
#include <GeoModelKernel/GeoTube.h>

BOOST_AUTO_TEST_SUITE(GeoModelDetObj)

struct GeoDims {
  std::vector<double> boxO;
  std::vector<double> boxI;
  std::vector<double> tube;
  std::vector<std::vector<double>> trapVerts;
  std::vector<double> trapHls;
  std::vector<std::vector<double>> polyVerts;
};
void test(const Acts::GeoModelDetectorObjectFactory::Cache& cache,
          GeoModelDetObj::GeoDims geoDims) {
  for (const auto& box : cache.boundingBoxes) {
    const Acts::VolumeBounds& bounds = box->volumeBounds();
    for (std::size_t i = 0; i < geoDims.boxO.size(); i++) {
      BOOST_CHECK(geoDims.boxO[i] == bounds.values()[i]);
    }
    std::vector<const Acts::Surface*> surfaces = box->surfaces();

    for (auto surface : surfaces) {
      const Acts::SurfaceBounds& sbounds = surface->bounds();
      // check straws
      if (surface->type() == Acts::Surface::SurfaceType::Straw) {
        const auto* lineBounds =
            dynamic_cast<const Acts::LineBounds*>(&sbounds);
        BOOST_CHECK(geoDims.tube[1] == lineBounds->get(Acts::LineBounds::eR));
        BOOST_CHECK(geoDims.tube[2] ==
                    lineBounds->get(Acts::LineBounds::eHalfLengthZ));
      }
      // rectangle Surface check corner position without trf
      if (sbounds.type() == Acts::SurfaceBounds::eRectangle) {
        double csxmin = sbounds.values()[0];
        double csymin = sbounds.values()[1];
        double csxmax = sbounds.values()[2];
        double csymax = sbounds.values()[3];
        BOOST_CHECK(geoDims.boxI[0] == -csxmin);
        BOOST_CHECK(geoDims.boxI[1] == -csymin);
        BOOST_CHECK(geoDims.boxI[0] == csxmax);
        BOOST_CHECK(geoDims.boxI[1] == csymax);
      }
      // trap Surface without trf
      if (sbounds.type() == Acts::SurfaceBounds::eTrapezoid) {
        const auto* trapBounds =
            dynamic_cast<const Acts::TrapezoidBounds*>(&sbounds);
        std::vector<Acts::Vector2> trapVerts = trapBounds->vertices();

        for (std::size_t i = 0; i < trapVerts.size(); i++) {
          BOOST_CHECK(trapVerts[i][0] == geoDims.trapVerts[i][0]);
          BOOST_CHECK(trapVerts[i][1] == geoDims.trapVerts[i][1]);
        }
      }
    }
  }
}
struct GeoGeometry {
  std::vector<GeoIntrusivePtr<GeoFullPhysVol>> fpvs;
  GeoDims dim;
};
GeoGeometry constructGeoModel() {
  // define materials
  GeoIntrusivePtr<GeoMaterial> material(new GeoMaterial("Material", 1.0));
  GeoIntrusivePtr<GeoMaterial> al(new GeoMaterial("Aluminium", 1.0));

  // define dimensions
  GeoDims geoDims;
  geoDims.boxO = {100, 200, 2};
  geoDims.boxI = {100, 150, 1};
  geoDims.trapVerts = {{-103, -50}, {103, -50}, {183, 50}, {-183, 50}};
  geoDims.polyVerts = {{-60, -50}, {60, -50},  {153, 0},
                       {123, 50},  {-123, 50}, {-153, 0}};
  geoDims.tube = {5, 6, 100};
  geoDims.trapHls = {
      std::abs(geoDims.trapVerts[0][0] - geoDims.trapVerts[1][0]) / 2,
      std::abs(geoDims.trapVerts[2][0] - geoDims.trapVerts[3][0]) / 2,
      std::abs(geoDims.trapVerts[0][1] - geoDims.trapVerts[2][1]) / 2};

  // create shapes
  GeoIntrusivePtr<GeoBox> boxXY(
      new GeoBox(geoDims.boxO[0], geoDims.boxO[1], geoDims.boxO[2]));
  GeoIntrusivePtr<GeoTube> tube(
      new GeoTube(geoDims.tube[0], geoDims.tube[1], geoDims.tube[2]));
  GeoIntrusivePtr<GeoBox> ssurface(
      new GeoBox(geoDims.boxI[0], geoDims.boxI[1], geoDims.boxI[2]));
  GeoIntrusivePtr<GeoTrd> trd(new GeoTrd(
      1, 1, geoDims.trapHls[0], geoDims.trapHls[1], geoDims.trapHls[2]));

  // create logvols
  GeoIntrusivePtr<GeoLogVol> logXY(
      new GeoLogVol("LogVolumeXY", boxXY, material));
  GeoIntrusivePtr<GeoLogVol> logTube(new GeoLogVol("LogTube", tube, al));
  GeoIntrusivePtr<GeoLogVol> logSurface(
      new GeoLogVol("LogSurface", ssurface, al));
  GeoIntrusivePtr<GeoLogVol> logTrd(new GeoLogVol("LogTrd", trd, al));

  // create physvols
  std::vector<GeoIntrusivePtr<GeoFullPhysVol>> fpvs;
  fpvs.push_back(new GeoFullPhysVol(logXY));
  fpvs.push_back(new GeoFullPhysVol(logTube));
  fpvs.push_back(new GeoFullPhysVol(logSurface));
  fpvs.push_back(new GeoFullPhysVol(logTrd));
  GeoGeometry ret;
  ret.fpvs = fpvs;
  ret.dim = geoDims;
  return ret;
}

BOOST_AUTO_TEST_CASE(GeoModelDetectorObjectFactory) {
  GeoGeometry geom = constructGeoModel();
  GeoDims geoDims = geom.dim;
  std::vector<GeoIntrusivePtr<GeoFullPhysVol>> fpvs = geom.fpvs;

  int index = 0;
  GeoFullPhysVol* parentVol = fpvs[index];
  fpvs.erase(fpvs.begin() + index);
  // build hierarchy
  for (const auto& fpv : fpvs) {
    parentVol->add(fpv);
  }

  // create pars for conversion
  Acts::GeoModelDetectorObjectFactory::Config gmConfig;
  gmConfig.convertBox = {"LogVolumeXY"};
  Acts::GeometryContext gContext;
  Acts::GeoModelDetectorObjectFactory::Cache gmCache;

  // create factory instance
  Acts::GeoModelDetectorObjectFactory factory(gmConfig);
  // convert GeoFullPhysVol
  factory.convertFpv("LogVolumeXY", parentVol, gmCache, gContext);

  // checking the dimension of the converted bounding boxes
  test(gmCache, geoDims);
}

BOOST_AUTO_TEST_SUITE_END()
