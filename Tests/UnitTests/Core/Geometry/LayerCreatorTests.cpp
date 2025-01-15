// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/CylinderLayer.hpp"
#include "Acts/Geometry/DiscLayer.hpp"
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/LayerCreator.hpp"
#include "Acts/Geometry/ProtoLayer.hpp"
#include "Acts/Geometry/SurfaceArrayCreator.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/PlanarBounds.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceArray.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/IAxis.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numbers>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <boost/format.hpp>

namespace Acts::Test {

// Create a test context
GeometryContext tgContext = GeometryContext();

using SrfVec = std::vector<std::shared_ptr<const Surface>>;

void draw_surfaces(const SrfVec& surfaces, const std::string& fname) {
  std::ofstream os;
  os.open(fname);

  os << std::fixed << std::setprecision(4);

  std::size_t nVtx = 0;
  for (const auto& srfx : surfaces) {
    std::shared_ptr<const PlaneSurface> srf =
        std::dynamic_pointer_cast<const PlaneSurface>(srfx);
    const PlanarBounds* bounds =
        dynamic_cast<const PlanarBounds*>(&srf->bounds());

    for (const auto& vtxloc : bounds->vertices()) {
      Vector3 vtx =
          srf->transform(tgContext) * Vector3(vtxloc.x(), vtxloc.y(), 0);
      os << "v " << vtx.x() << " " << vtx.y() << " " << vtx.z() << "\n";
    }

    // connect them
    os << "f";
    for (std::size_t i = 1; i <= bounds->vertices().size(); ++i) {
      os << " " << nVtx + i;
    }
    os << "\n";

    nVtx += bounds->vertices().size();
  }

  os.close();
}

struct LayerCreatorFixture {
  std::shared_ptr<const SurfaceArrayCreator> p_SAC;
  std::shared_ptr<LayerCreator> p_LC;

  std::vector<std::shared_ptr<const Surface>> m_surfaces;

  LayerCreatorFixture() {
    p_SAC = std::make_shared<const SurfaceArrayCreator>(
        SurfaceArrayCreator::Config(),
        Acts::getDefaultLogger("SurfaceArrayCreator", Acts::Logging::VERBOSE));
    LayerCreator::Config cfg;
    cfg.surfaceArrayCreator = p_SAC;
    p_LC = std::make_shared<LayerCreator>(
        cfg, Acts::getDefaultLogger("LayerCreator", Acts::Logging::VERBOSE));
  }

  template <typename... Args>
  bool checkBinning(Args&&... args) {
    return p_LC->checkBinning(std::forward<Args>(args)...);
  }

  bool checkBinContentSize(const SurfaceArray* sArray, std::size_t n) {
    std::size_t nBins = sArray->size();
    bool result = true;
    for (std::size_t i = 0; i < nBins; ++i) {
      if (!sArray->isValidBin(i)) {
        continue;
      }
      std::vector<const Surface*> binContent = sArray->at(i);
      BOOST_TEST_INFO("Bin: " << i);
      BOOST_CHECK_EQUAL(binContent.size(), n);
      result = result && binContent.size() == n;
    }

    return result;
  }

  SrfVec fullPhiTestSurfacesEC(std::size_t n = 10, double shift = 0,
                               double zbase = 0, double r = 10) {
    SrfVec res;

    double phiStep = 2 * std::numbers::pi / n;
    for (std::size_t i = 0; i < n; ++i) {
      double z = zbase + ((i % 2 == 0) ? 1 : -1) * 0.2;

      Transform3 trans;
      trans.setIdentity();
      trans.rotate(Eigen::AngleAxisd(i * phiStep + shift, Vector3(0, 0, 1)));
      trans.translate(Vector3(r, 0, z));

      auto bounds = std::make_shared<const RectangleBounds>(2, 1);
      std::shared_ptr<PlaneSurface> srf =
          Surface::makeShared<PlaneSurface>(trans, bounds);

      res.push_back(srf);
      m_surfaces.push_back(
          std::move(srf));  // keep shared, will get destroyed at the end
    }

    return res;
  }

  SrfVec fullPhiTestSurfacesBRL(int n = 10, double shift = 0, double zbase = 0,
                                double incl = std::numbers::pi / 9.,
                                double w = 2, double h = 1.5) {
    SrfVec res;

    double phiStep = 2 * std::numbers::pi / n;
    for (int i = 0; i < n; ++i) {
      double z = zbase;

      Transform3 trans;
      trans.setIdentity();
      trans.rotate(Eigen::AngleAxisd(i * phiStep + shift, Vector3(0, 0, 1)));
      trans.translate(Vector3(10, 0, z));
      trans.rotate(Eigen::AngleAxisd(incl, Vector3(0, 0, 1)));
      trans.rotate(Eigen::AngleAxisd(std::numbers::pi / 2., Vector3(0, 1, 0)));

      auto bounds = std::make_shared<const RectangleBounds>(w, h);
      std::shared_ptr<PlaneSurface> srf =
          Surface::makeShared<PlaneSurface>(trans, bounds);

      res.push_back(srf);
      m_surfaces.push_back(
          std::move(srf));  // keep shared, will get destroyed at the end
    }

    return res;
  }

  SrfVec makeBarrel(int nPhi, int nZ, double w, double h) {
    double z0 = -(nZ - 1) * w;
    SrfVec res;

    for (int i = 0; i < nZ; i++) {
      double z = i * w * 2 + z0;
      std::cout << "z=" << z << std::endl;
      SrfVec ring =
          fullPhiTestSurfacesBRL(nPhi, 0, z, std::numbers::pi / 9., w, h);
      res.insert(res.end(), ring.begin(), ring.end());
    }

    return res;
  }

  std::pair<SrfVec, std::vector<std::pair<const Surface*, const Surface*>>>
  makeBarrelStagger(int nPhi, int nZ, double shift = 0,
                    double incl = std::numbers::pi / 9., double w = 2,
                    double h = 1.5) {
    double z0 = -(nZ - 1) * w;
    SrfVec res;

    std::vector<std::pair<const Surface*, const Surface*>> pairs;

    for (int i = 0; i < nZ; i++) {
      double z = i * w * 2 + z0;

      double phiStep = 2 * std::numbers::pi / nPhi;
      for (int j = 0; j < nPhi; ++j) {
        Transform3 trans;
        trans.setIdentity();
        trans.rotate(Eigen::AngleAxisd(j * phiStep + shift, Vector3(0, 0, 1)));
        trans.translate(Vector3(10, 0, z));
        trans.rotate(Eigen::AngleAxisd(incl, Vector3(0, 0, 1)));
        trans.rotate(
            Eigen::AngleAxisd(std::numbers::pi / 2., Vector3(0, 1, 0)));

        auto bounds = std::make_shared<const RectangleBounds>(w, h);
        std::shared_ptr<PlaneSurface> srfA =
            Surface::makeShared<PlaneSurface>(trans, bounds);

        Vector3 nrm = srfA->normal(tgContext);
        Transform3 transB = trans;
        transB.pretranslate(nrm * 0.1);
        std::shared_ptr<PlaneSurface> srfB =
            Surface::makeShared<PlaneSurface>(transB, bounds);

        pairs.push_back(std::make_pair(srfA.get(), srfB.get()));

        res.push_back(srfA);
        res.push_back(srfB);
        m_surfaces.push_back(std::move(srfA));
        m_surfaces.push_back(std::move(srfB));
      }
    }

    return {res, pairs};
  }
};

BOOST_AUTO_TEST_SUITE(Tools)

BOOST_FIXTURE_TEST_CASE(LayerCreator_createCylinderLayer, LayerCreatorFixture) {
  std::vector<std::shared_ptr<const Surface>> srf;

  srf = makeBarrel(30, 7, 2, 1.5);
  draw_surfaces(srf, "LayerCreator_createCylinderLayer_BRL_1.obj");

  // CASE I
  double envR = 0.1, envZ = 0.5;
  ProtoLayer pl(tgContext, srf);
  pl.envelope[Acts::AxisDirection::AxisR] = {envR, envR};
  pl.envelope[Acts::AxisDirection::AxisZ] = {envZ, envZ};
  std::shared_ptr<CylinderLayer> layer =
      std::dynamic_pointer_cast<CylinderLayer>(
          p_LC->cylinderLayer(tgContext, srf, equidistant, equidistant, pl));

  //
  double rMax = 10.6071, rMin = 9.59111;  // empirical - w/o envelopes
  CHECK_CLOSE_REL(layer->thickness(), (rMax - rMin) + 2. * envR, 1e-3);

  const CylinderBounds* bounds = &layer->bounds();
  CHECK_CLOSE_REL(bounds->get(CylinderBounds::eR), (rMax + rMin) / 2., 1e-3);
  CHECK_CLOSE_REL(bounds->get(CylinderBounds::eHalfLengthZ), 14 + envZ, 1e-3);
  BOOST_CHECK(checkBinning(tgContext, *layer->surfaceArray()));
  auto axes = layer->surfaceArray()->getAxes();
  BOOST_CHECK_EQUAL(axes.at(0)->getNBins(), 30u);
  BOOST_CHECK_EQUAL(axes.at(1)->getNBins(), 7u);
  CHECK_CLOSE_REL(axes.at(0)->getMin(), -std::numbers::pi, 1e-3);
  CHECK_CLOSE_REL(axes.at(0)->getMax(), std::numbers::pi, 1e-3);
  CHECK_CLOSE_REL(axes.at(1)->getMin(), -14, 1e-3);
  CHECK_CLOSE_REL(axes.at(1)->getMax(), 14, 1e-3);

  // CASE II

  ProtoLayer pl2(tgContext, srf);
  pl2.envelope[Acts::AxisDirection::AxisR] = {envR, envR};
  pl2.envelope[Acts::AxisDirection::AxisZ] = {envZ, envZ};
  layer = std::dynamic_pointer_cast<CylinderLayer>(
      p_LC->cylinderLayer(tgContext, srf, 30, 7, pl2));
  CHECK_CLOSE_REL(layer->thickness(), (rMax - rMin) + 2 * envR, 1e-3);
  bounds = &layer->bounds();
  CHECK_CLOSE_REL(bounds->get(CylinderBounds::eR), (rMax + rMin) / 2., 1e-3);
  CHECK_CLOSE_REL(bounds->get(CylinderBounds::eHalfLengthZ), 14 + envZ, 1e-3);
  BOOST_CHECK(checkBinning(tgContext, *layer->surfaceArray()));
  axes = layer->surfaceArray()->getAxes();
  BOOST_CHECK_EQUAL(axes.at(0)->getNBins(), 30u);
  BOOST_CHECK_EQUAL(axes.at(1)->getNBins(), 7u);
  CHECK_CLOSE_REL(axes.at(0)->getMin(), -std::numbers::pi, 1e-3);
  CHECK_CLOSE_REL(axes.at(0)->getMax(), std::numbers::pi, 1e-3);
  CHECK_CLOSE_REL(axes.at(1)->getMin(), -14, 1e-3);
  CHECK_CLOSE_REL(axes.at(1)->getMax(), 14, 1e-3);

  layer = std::dynamic_pointer_cast<CylinderLayer>(
      p_LC->cylinderLayer(tgContext, srf, 13, 3, pl2));
  CHECK_CLOSE_REL(layer->thickness(), (rMax - rMin) + 2 * envR, 1e-3);
  bounds = &layer->bounds();
  CHECK_CLOSE_REL(bounds->get(CylinderBounds::eR), (rMax + rMin) / 2., 1e-3);
  CHECK_CLOSE_REL(bounds->get(CylinderBounds::eHalfLengthZ), 14 + envZ, 1e-3);
  // this succeeds despite sub-optimal binning
  // since we now have multientry bins
  BOOST_CHECK(checkBinning(tgContext, *layer->surfaceArray()));
  axes = layer->surfaceArray()->getAxes();
  BOOST_CHECK_EQUAL(axes.at(0)->getNBins(), 13u);
  BOOST_CHECK_EQUAL(axes.at(1)->getNBins(), 3u);
  CHECK_CLOSE_REL(axes.at(0)->getMin(), -std::numbers::pi, 1e-3);
  CHECK_CLOSE_REL(axes.at(0)->getMax(), std::numbers::pi, 1e-3);
  CHECK_CLOSE_REL(axes.at(1)->getMin(), -14, 1e-3);
  CHECK_CLOSE_REL(axes.at(1)->getMax(), 14, 1e-3);

  // CASE III
  ProtoLayer pl3;
  pl3.extent.range(Acts::AxisDirection::AxisR).set(1, 20);
  pl3.extent.range(Acts::AxisDirection::AxisZ).set(-25, 25);
  layer = std::dynamic_pointer_cast<CylinderLayer>(
      p_LC->cylinderLayer(tgContext, srf, equidistant, equidistant, pl3));
  CHECK_CLOSE_REL(layer->thickness(), 19, 1e-3);
  bounds = &layer->bounds();
  CHECK_CLOSE_REL(bounds->get(CylinderBounds::eR), 10.5, 1e-3);
  CHECK_CLOSE_REL(bounds->get(CylinderBounds::eHalfLengthZ), 25, 1e-3);

  // this should fail, b/c it's a completely inconvenient binning
  // but it succeeds despite sub-optimal binning
  // since we now have multientry bins
  BOOST_CHECK(checkBinning(tgContext, *layer->surfaceArray()));

  axes = layer->surfaceArray()->getAxes();
  BOOST_CHECK_EQUAL(axes.at(0)->getNBins(), 30u);
  BOOST_CHECK_EQUAL(axes.at(1)->getNBins(), 7u);
  CHECK_CLOSE_REL(axes.at(0)->getMin(), -std::numbers::pi, 1e-3);
  CHECK_CLOSE_REL(axes.at(0)->getMax(), std::numbers::pi, 1e-3);
  CHECK_CLOSE_REL(axes.at(1)->getMin(), -25, 1e-3);
  CHECK_CLOSE_REL(axes.at(1)->getMax(), 25, 1e-3);
}

BOOST_FIXTURE_TEST_CASE(LayerCreator_createDiscLayer, LayerCreatorFixture) {
  std::vector<std::shared_ptr<const Surface>> surfaces;
  auto ringa = fullPhiTestSurfacesEC(30, 0, 0, 10);
  surfaces.insert(surfaces.end(), ringa.begin(), ringa.end());
  auto ringb = fullPhiTestSurfacesEC(30, 0, 0, 15);
  surfaces.insert(surfaces.end(), ringb.begin(), ringb.end());
  auto ringc = fullPhiTestSurfacesEC(30, 0, 0, 20);
  surfaces.insert(surfaces.end(), ringc.begin(), ringc.end());
  draw_surfaces(surfaces, "LayerCreator_createDiscLayer_EC_1.obj");

  ProtoLayer pl(tgContext, surfaces);
  pl.extent.range(AxisDirection::AxisZ).set(-10, 10);
  pl.extent.range(AxisDirection::AxisR).set(5., 25.);
  std::shared_ptr<DiscLayer> layer = std::dynamic_pointer_cast<DiscLayer>(
      p_LC->discLayer(tgContext, surfaces, equidistant, equidistant, pl));
  CHECK_CLOSE_REL(layer->thickness(), 20, 1e-3);
  const RadialBounds* bounds =
      dynamic_cast<const RadialBounds*>(&layer->bounds());
  CHECK_CLOSE_REL(bounds->rMin(), 5, 1e-3);
  CHECK_CLOSE_REL(bounds->rMax(), 25, 1e-3);
  BOOST_CHECK(checkBinning(tgContext, *layer->surfaceArray()));
  auto axes = layer->surfaceArray()->getAxes();
  BOOST_CHECK_EQUAL(axes.at(0)->getNBins(), 3u);
  BOOST_CHECK_EQUAL(axes.at(1)->getNBins(), 30u);
  CHECK_CLOSE_REL(axes.at(0)->getMin(), 5, 1e-3);
  CHECK_CLOSE_REL(axes.at(0)->getMax(), 25, 1e-3);
  CHECK_CLOSE_REL(axes.at(1)->getMin(), -std::numbers::pi, 1e-3);
  CHECK_CLOSE_REL(axes.at(1)->getMax(), std::numbers::pi, 1e-3);
  checkBinContentSize(layer->surfaceArray(), 1);

  // check that it's applying a rotation transform to improve phi binning
  // BOOST_CHECK_NE(bu->transform(), nullptr);
  // double actAngle = ((*bu->transform()) * Vector3(1, 0, 0)).phi();
  // double expAngle = -2 * std::numbers::pi / 30 / 2.;
  // CHECK_CLOSE_REL(actAngle, expAngle, 1e-3);

  double envMinR = 1, envMaxR = 1, envZ = 5;
  std::size_t nBinsR = 3, nBinsPhi = 30;
  ProtoLayer pl2(tgContext, surfaces);
  pl2.envelope[AxisDirection::AxisR] = {envMinR, envMaxR};
  pl2.envelope[AxisDirection::AxisZ] = {envZ, envZ};
  layer = std::dynamic_pointer_cast<DiscLayer>(
      p_LC->discLayer(tgContext, surfaces, nBinsR, nBinsPhi, pl2));

  double rMin = 8, rMax = 22.0227;
  CHECK_CLOSE_REL(layer->thickness(), 0.4 + 2 * envZ, 1e-3);
  bounds = dynamic_cast<const RadialBounds*>(&layer->bounds());
  CHECK_CLOSE_REL(bounds->rMin(), rMin - envMinR, 1e-3);
  CHECK_CLOSE_REL(bounds->rMax(), rMax + envMaxR, 1e-3);
  BOOST_CHECK(checkBinning(tgContext, *layer->surfaceArray()));
  axes = layer->surfaceArray()->getAxes();
  BOOST_CHECK_EQUAL(axes.at(0)->getNBins(), nBinsR);
  BOOST_CHECK_EQUAL(axes.at(1)->getNBins(), nBinsPhi);
  CHECK_CLOSE_REL(axes.at(0)->getMin(), rMin, 1e-3);
  CHECK_CLOSE_REL(axes.at(0)->getMax(), rMax, 1e-3);
  CHECK_CLOSE_REL(axes.at(1)->getMin(), -std::numbers::pi, 1e-3);
  CHECK_CLOSE_REL(axes.at(1)->getMax(), std::numbers::pi, 1e-3);
  checkBinContentSize(layer->surfaceArray(), 1);

  // check that it's applying a rotation transform to improve phi binning
  // BOOST_CHECK_NE(bu->transform(), nullptr);
  // actAngle = ((*bu->transform()) * Vector3(1, 0, 0)).phi();
  // expAngle = -2 * std::numbers::pi / 30 / 2.;
  // CHECK_CLOSE_REL(actAngle, expAngle, 1e-3);

  layer = std::dynamic_pointer_cast<DiscLayer>(
      p_LC->discLayer(tgContext, surfaces, equidistant, equidistant, pl2));
  CHECK_CLOSE_REL(layer->thickness(), 0.4 + 2 * envZ, 1e-3);
  bounds = dynamic_cast<const RadialBounds*>(&layer->bounds());
  CHECK_CLOSE_REL(bounds->rMin(), rMin - envMinR, 1e-3);
  CHECK_CLOSE_REL(bounds->rMax(), rMax + envMaxR, 1e-3);
  BOOST_CHECK(checkBinning(tgContext, *layer->surfaceArray()));
  axes = layer->surfaceArray()->getAxes();
  BOOST_CHECK_EQUAL(axes.at(0)->getNBins(), nBinsR);
  BOOST_CHECK_EQUAL(axes.at(1)->getNBins(), nBinsPhi);
  CHECK_CLOSE_REL(axes.at(0)->getMin(), rMin, 1e-3);
  CHECK_CLOSE_REL(axes.at(0)->getMax(), rMax, 1e-3);
  CHECK_CLOSE_REL(axes.at(1)->getMin(), -std::numbers::pi, 1e-3);
  CHECK_CLOSE_REL(axes.at(1)->getMax(), std::numbers::pi, 1e-3);
  checkBinContentSize(layer->surfaceArray(), 1);

  // check that it's applying a rotation transform to improve phi binning
  // BOOST_CHECK_NE(bu->transform(), nullptr);
  // actAngle = ((*bu->transform()) * Vector3(1, 0, 0)).phi();
  // expAngle = -2 * std::numbers::pi / 30 / 2.;
  // CHECK_CLOSE_REL(actAngle, expAngle, 1e-3);
}

BOOST_FIXTURE_TEST_CASE(LayerCreator_barrelStagger, LayerCreatorFixture) {
  auto barrel = makeBarrelStagger(30, 7, 0, std::numbers::pi / 9.);
  auto brl = barrel.first;
  draw_surfaces(brl, "LayerCreator_barrelStagger.obj");

  double envR = 0, envZ = 0;
  ProtoLayer pl(tgContext, brl);
  pl.envelope[AxisDirection::AxisR] = {envR, envR};
  pl.envelope[AxisDirection::AxisZ] = {envZ, envZ};
  std::shared_ptr<CylinderLayer> layer =
      std::dynamic_pointer_cast<CylinderLayer>(
          p_LC->cylinderLayer(tgContext, brl, equidistant, equidistant, pl));

  auto axes = layer->surfaceArray()->getAxes();
  BOOST_CHECK_EQUAL(axes.at(0)->getNBins(), 30u);
  BOOST_CHECK_EQUAL(axes.at(1)->getNBins(), 7u);

  // check if binning is good!
  for (const auto& pr : barrel.second) {
    auto A = pr.first;
    auto B = pr.second;

    // std::cout << A->center().phi() << " ";
    // std::cout << B->center().phi() << std::endl;
    // std::cout << "dPHi = " << A->center().phi() - B->center().phi() <<
    // std::endl;

    Vector3 ctr = A->referencePosition(tgContext, AxisDirection::AxisR);
    auto binContent = layer->surfaceArray()->at(ctr);
    BOOST_CHECK_EQUAL(binContent.size(), 2u);
    std::set<const Surface*> act;
    act.insert(binContent[0]);
    act.insert(binContent[1]);

    std::set<const Surface*> exp;
    exp.insert(A);
    exp.insert(B);
    BOOST_CHECK(exp == act);
  }

  // checkBinning should also report everything is fine
  checkBinning(tgContext, *layer->surfaceArray());
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace Acts::Test
