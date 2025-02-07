// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/ProtoLayer.hpp"
#include "Acts/Geometry/SurfaceArrayCreator.hpp"
#include "Acts/Surfaces/PlanarBounds.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceArray.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/Axis.hpp"
#include "Acts/Utilities/AxisDefinitions.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Grid.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "Acts/Utilities/IAxis.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/detail/grid_helper.hpp"
#include "Acts/Visualization/GeometryView3D.hpp"
#include "Acts/Visualization/ObjVisualization3D.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <memory>
#include <numbers>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <boost/format.hpp>

using Acts::VectorHelpers::perp;
using Acts::VectorHelpers::phi;

namespace Acts::Test {

// Create a test context
GeometryContext tgContext = GeometryContext();

using SrfVec = std::vector<std::shared_ptr<const Surface>>;

struct SurfaceArrayCreatorFixture {
  SurfaceArrayCreator m_SAC;
  std::vector<std::shared_ptr<const Surface>> m_surfaces;

  SurfaceArrayCreatorFixture()
      : m_SAC(SurfaceArrayCreator::Config(),
              Acts::getDefaultLogger("SurfaceArrayCreator",
                                     Acts::Logging::VERBOSE)) {
    BOOST_TEST_MESSAGE("setup fixture");
  }
  ~SurfaceArrayCreatorFixture() { BOOST_TEST_MESSAGE("teardown fixture"); }

  template <typename... Args>
  SurfaceArrayCreator::ProtoAxis createEquidistantAxis(Args&&... args) {
    return m_SAC.createEquidistantAxis(std::forward<Args>(args)...);
  }

  template <typename... Args>
  SurfaceArrayCreator::ProtoAxis createVariableAxis(Args&&... args) {
    return m_SAC.createVariableAxis(std::forward<Args>(args)...);
  }

  template <AxisBoundaryType bdtA, AxisBoundaryType bdtB, typename... Args>
  std::unique_ptr<SurfaceArray::ISurfaceGridLookup> makeSurfaceGridLookup2D(
      Args&&... args) {
    return m_SAC.makeSurfaceGridLookup2D<bdtA, bdtB>(
        std::forward<Args>(args)...);
  }

  SrfVec fullPhiTestSurfacesEC(std::size_t n = 10, double shift = 0,
                               double zbase = 0, double r = 10, double w = 2,
                               double h = 1) {
    SrfVec res;
    // TODO: The test is extremely numerically unstable in the face of upward
    //       rounding in this multiplication and division. Find out why.
    double phiStep = 2 * std::numbers::pi / n;
    for (std::size_t i = 0; i < n; ++i) {
      double z = zbase + ((i % 2 == 0) ? 1 : -1) * 0.2;
      double phi = std::fma(i, phiStep, shift);

      Transform3 trans;
      trans.setIdentity();
      trans.rotate(Eigen::AngleAxisd(phi, Vector3(0, 0, 1)));
      trans.translate(Vector3(r, 0, z));

      auto bounds = std::make_shared<const RectangleBounds>(w, h);

      std::shared_ptr<Surface> srf =
          Surface::makeShared<PlaneSurface>(trans, bounds);

      res.push_back(srf);
      m_surfaces.push_back(
          std::move(srf));  // keep shared, will get destroyed at the end
    }

    return res;
  }

  SrfVec fullPhiTestSurfacesBRL(std::size_t n = 10, double shift = 0,
                                double zbase = 0,
                                double incl = std::numbers::pi / 9.,
                                double w = 2, double h = 1.5) {
    SrfVec res;
    // TODO: The test is extremely numerically unstable in the face of upward
    //       rounding in this multiplication and division. Find out why.
    double phiStep = 2 * std::numbers::pi / n;
    for (std::size_t i = 0; i < n; ++i) {
      double z = zbase;
      double phi = std::fma(i, phiStep, shift);

      Transform3 trans;
      trans.setIdentity();
      trans.rotate(Eigen::AngleAxisd(phi, Vector3(0, 0, 1)));
      trans.translate(Vector3(10, 0, z));
      trans.rotate(Eigen::AngleAxisd(incl, Vector3(0, 0, 1)));
      trans.rotate(Eigen::AngleAxisd(std::numbers::pi / 2., Vector3(0, 1, 0)));

      auto bounds = std::make_shared<const RectangleBounds>(w, h);
      std::shared_ptr<Surface> srf =
          Surface::makeShared<PlaneSurface>(trans, bounds);

      res.push_back(srf);
      m_surfaces.push_back(
          std::move(srf));  // keep shared, will get destroyed at the end
    }

    return res;
  }

  SrfVec straightLineSurfaces(
      std::size_t n = 10., double step = 3, const Vector3& origin = {0, 0, 1.5},
      const Transform3& pretrans = Transform3::Identity(),
      const Vector3& dir = {0, 0, 1}) {
    SrfVec res;
    for (std::size_t i = 0; i < n; ++i) {
      Transform3 trans;
      trans.setIdentity();
      trans.translate(origin + dir * step * i);
      // trans.rotate(AngleAxis3(std::numbers::pi / 9., Vector3(0, 0, 1)));
      trans.rotate(AngleAxis3(std::numbers::pi / 2., Vector3(1, 0, 0)));
      trans = trans * pretrans;

      auto bounds = std::make_shared<const RectangleBounds>(2, 1.5);

      std::shared_ptr<Surface> srf =
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
      // std::cout << "z=" << z << std::endl;
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
    // TODO: The test is extremely numerically unstable in the face of upward
    //       rounding in this multiplication and division. Find out why.
    double phiStep = 2 * std::numbers::pi / nPhi;
    for (int i = 0; i < nZ; i++) {
      double z = i * w * 2 + z0;
      for (int j = 0; j < nPhi; ++j) {
        double phi = std::fma(j, phiStep, shift);
        Transform3 trans;
        trans.setIdentity();
        trans.rotate(Eigen::AngleAxisd(phi, Vector3(0, 0, 1)));
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
        std::shared_ptr<Surface> srfB =
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

BOOST_AUTO_TEST_SUITE(Tools)

BOOST_FIXTURE_TEST_CASE(SurfaceArrayCreator_createEquidistantAxis_Phi,
                        SurfaceArrayCreatorFixture) {
  // fail on empty srf vector
  std::vector<const Surface*> emptyRaw;
  ProtoLayer pl(tgContext, emptyRaw);
  auto tr = Transform3::Identity();
  BOOST_CHECK_THROW(createEquidistantAxis(tgContext, emptyRaw,
                                          AxisDirection::AxisPhi, pl, tr),
                    std::logic_error);

  std::vector<float> bdExp = {
      -3.14159, -2.93215, -2.72271, -2.51327, -2.30383,  -2.0944,   -1.88496,
      -1.67552, -1.46608, -1.25664, -1.0472,  -0.837758, -0.628319, -0.418879,
      -0.20944, 0,        0.20944,  0.418879, 0.628319,  0.837758,  1.0472,
      1.25664,  1.46608,  1.67552,  1.88496,  2.09439,   2.30383,   2.51327,
      2.72271,  2.93215,  3.14159};

  double step = 2 * std::numbers::pi / 30.;

  // endcap style modules

  for (int i = -1; i <= 2; i += 2) {
    double z = 10 * i;
    // case 1: one module sits at pi / -pi
    double angleShift = step / 2.;
    auto surfaces = fullPhiTestSurfacesEC(30, angleShift, z);
    std::vector<const Surface*> surfacesRaw = unpack_shared_vector(surfaces);
    pl = ProtoLayer(tgContext, surfacesRaw);
    tr = Transform3::Identity();
    auto axis = createEquidistantAxis(tgContext, surfacesRaw,
                                      AxisDirection::AxisPhi, pl, tr);

    BOOST_CHECK_EQUAL(axis.nBins, 30u);
    CHECK_CLOSE_REL(axis.max, std::numbers::pi, 1e-6);
    CHECK_CLOSE_REL(axis.min, -std::numbers::pi, 1e-6);
    BOOST_CHECK_EQUAL(axis.bType, equidistant);
    CHECK_SMALL(phi(tr * Vector3::UnitX()), 1e-6);

    // case 2: two modules sit symmetrically around pi / -pi
    angleShift = 0.;
    surfaces = fullPhiTestSurfacesEC(30, angleShift, z);
    surfacesRaw = unpack_shared_vector(surfaces);
    pl = ProtoLayer(tgContext, surfacesRaw);
    tr = Transform3::Identity();
    axis = createEquidistantAxis(tgContext, surfacesRaw, AxisDirection::AxisPhi,
                                 pl, tr);
    draw_surfaces(surfaces,
                  "SurfaceArrayCreator_createEquidistantAxis_EC_2.obj");
    BOOST_CHECK_EQUAL(axis.nBins, 30u);
    CHECK_CLOSE_REL(axis.max, std::numbers::pi, 1e-6);
    CHECK_CLOSE_REL(axis.min, -std::numbers::pi, 1e-6);
    BOOST_CHECK_EQUAL(axis.bType, equidistant);
    // CHECK_CLOSE_REL(bdExp, axis.binEdges, 0.001);
    CHECK_CLOSE_REL(phi(tr * Vector3::UnitX()), -0.5 * step, 1e-3);
    // case 3: two modules sit asymmetrically around pi / -pi shifted up
    angleShift = step / -4.;
    surfaces = fullPhiTestSurfacesEC(30, angleShift, z);
    surfacesRaw = unpack_shared_vector(surfaces);
    pl = ProtoLayer(tgContext, surfacesRaw);
    tr = Transform3::Identity();
    axis = createEquidistantAxis(tgContext, surfacesRaw, AxisDirection::AxisPhi,
                                 pl, tr);
    draw_surfaces(surfaces,
                  "SurfaceArrayCreator_createEquidistantAxis_EC_3.obj");
    BOOST_CHECK_EQUAL(axis.nBins, 30u);
    CHECK_CLOSE_REL(axis.max, std::numbers::pi, 1e-6);
    CHECK_CLOSE_REL(axis.min, -std::numbers::pi, 1e-6);
    BOOST_CHECK_EQUAL(axis.bType, equidistant);
    CHECK_CLOSE_REL(phi(tr * Vector3::UnitX()), step / -4., 1e-3);

    // case 4: two modules sit asymmetrically around pi / -pi shifted down
    angleShift = step / 4.;
    surfaces = fullPhiTestSurfacesEC(30, angleShift, z);
    surfacesRaw = unpack_shared_vector(surfaces);
    pl = ProtoLayer(tgContext, surfaces);
    surfacesRaw = unpack_shared_vector(surfaces);
    tr = Transform3::Identity();
    axis = createEquidistantAxis(tgContext, surfacesRaw, AxisDirection::AxisPhi,
                                 pl, tr);
    surfacesRaw = unpack_shared_vector(surfaces);
    draw_surfaces(surfaces,
                  "SurfaceArrayCreator_createEquidistantAxis_EC_4.obj");
    BOOST_CHECK_EQUAL(axis.nBins, 30u);
    CHECK_CLOSE_REL(axis.max, std::numbers::pi, 1e-6);
    CHECK_CLOSE_REL(axis.min, -std::numbers::pi, 1e-6);
    BOOST_CHECK_EQUAL(axis.bType, equidistant);
    CHECK_CLOSE_REL(phi(tr * Vector3::UnitX()), step / 4., 1e-3);
  }

  for (int i = -1; i <= 2; i += 2) {
    double z = 10 * i;
    // case 1: one module sits at pi / -pi
    double angleShift = step / 2.;
    auto surfaces = fullPhiTestSurfacesBRL(30, angleShift, z);
    auto surfacesRaw = unpack_shared_vector(surfaces);
    pl = ProtoLayer(tgContext, surfacesRaw);
    tr = Transform3::Identity();
    auto axis = createEquidistantAxis(tgContext, surfacesRaw,
                                      AxisDirection::AxisPhi, pl, tr);
    draw_surfaces(surfaces,
                  "SurfaceArrayCreator_createEquidistantAxis_BRL_1.obj");
    BOOST_CHECK_EQUAL(axis.nBins, 30u);
    CHECK_CLOSE_REL(axis.max, std::numbers::pi, 1e-6);
    CHECK_CLOSE_REL(axis.min, -std::numbers::pi, 1e-6);
    BOOST_CHECK_EQUAL(axis.bType, equidistant);
    CHECK_SMALL(phi(tr * Vector3::UnitX()), 1e-6);

    // case 2: two modules sit symmetrically around pi / -pi
    angleShift = 0.;
    surfaces = fullPhiTestSurfacesBRL(30, angleShift, z);
    surfacesRaw = unpack_shared_vector(surfaces);
    pl = ProtoLayer(tgContext, surfacesRaw);
    tr = Transform3::Identity();
    axis = createEquidistantAxis(tgContext, surfacesRaw, AxisDirection::AxisPhi,
                                 pl, tr);
    draw_surfaces(surfaces,
                  "SurfaceArrayCreator_createEquidistantAxis_BRL_2.obj");
    BOOST_CHECK_EQUAL(axis.nBins, 30u);
    CHECK_CLOSE_REL(axis.max, std::numbers::pi, 1e-6);
    CHECK_CLOSE_REL(axis.min, -std::numbers::pi, 1e-6);
    BOOST_CHECK_EQUAL(axis.bType, equidistant);
    // CHECK_CLOSE_REL(bdExp, axis.binEdges, 0.001);
    CHECK_CLOSE_REL(phi(tr * Vector3::UnitX()), -0.5 * step, 1e-3);

    // case 3: two modules sit asymmetrically around pi / -pi shifted up
    angleShift = step / -4.;
    surfaces = fullPhiTestSurfacesBRL(30, angleShift, z);
    surfacesRaw = unpack_shared_vector(surfaces);
    pl = ProtoLayer(tgContext, surfacesRaw);
    tr = Transform3::Identity();
    axis = createEquidistantAxis(tgContext, surfacesRaw, AxisDirection::AxisPhi,
                                 pl, tr);
    draw_surfaces(surfaces,
                  "SurfaceArrayCreator_createEquidistantAxis_BRL_3.obj");
    BOOST_CHECK_EQUAL(axis.nBins, 30u);
    CHECK_CLOSE_REL(axis.max, std::numbers::pi, 1e-6);
    CHECK_CLOSE_REL(axis.min, -std::numbers::pi, 1e-6);
    BOOST_CHECK_EQUAL(axis.bType, equidistant);
    // CHECK_CLOSE_REL(bdExp, axis.binEdges, 0.001);
    CHECK_CLOSE_REL(phi(tr * Vector3::UnitX()), step / -4., 1e-3);

    // case 4: two modules sit asymmetrically around pi / -pi shifted down
    angleShift = step / 4.;
    surfaces = fullPhiTestSurfacesBRL(30, angleShift, z);
    surfacesRaw = unpack_shared_vector(surfaces);
    pl = ProtoLayer(tgContext, surfacesRaw);
    tr = Transform3::Identity();
    axis = createEquidistantAxis(tgContext, surfacesRaw, AxisDirection::AxisPhi,
                                 pl, tr);
    draw_surfaces(surfaces,
                  "SurfaceArrayCreator_createEquidistantAxis_BRL_4.obj");
    BOOST_CHECK_EQUAL(axis.nBins, 30u);
    CHECK_CLOSE_REL(axis.max, std::numbers::pi, 1e-6);
    CHECK_CLOSE_REL(axis.min, -std::numbers::pi, 1e-6);
    BOOST_CHECK_EQUAL(axis.bType, equidistant);
    // CHECK_CLOSE_REL(bdExp, axis.binEdges, 0.001);
    CHECK_CLOSE_REL(phi(tr * Vector3::UnitX()), step / 4., 1e-3);
  }

  SrfVec surfaces;

  // single element in phi
  surfaces = fullPhiTestSurfacesEC(1);
  auto surfacesRaw = unpack_shared_vector(surfaces);
  draw_surfaces(surfaces,
                "SurfaceArrayCreator_createEquidistantAxis_EC_Single.obj");

  pl = ProtoLayer(tgContext, surfacesRaw);
  tr = Transform3::Identity();
  auto axis = createEquidistantAxis(tgContext, surfacesRaw,
                                    AxisDirection::AxisPhi, pl, tr);
  BOOST_CHECK_EQUAL(axis.nBins, 1u);

  CHECK_CLOSE_ABS(axis.max, phi(Vector3(8, 1, 0)), 1e-3);
  CHECK_CLOSE_ABS(axis.min, phi(Vector3(8, -1, 0)), 1e-3);
  BOOST_CHECK_EQUAL(axis.bType, equidistant);
}

BOOST_FIXTURE_TEST_CASE(SurfaceArrayCreator_createEquidistantAxis_Z,
                        SurfaceArrayCreatorFixture) {
  // single element in z
  auto surfaces = straightLineSurfaces(1);
  auto surfacesRaw = unpack_shared_vector(surfaces);
  ProtoLayer pl = ProtoLayer(tgContext, surfacesRaw);
  auto trf = Transform3::Identity();
  auto axis = createEquidistantAxis(tgContext, surfacesRaw,
                                    AxisDirection::AxisZ, pl, trf);
  draw_surfaces(surfaces, "SurfaceArrayCreator_createEquidistantAxis_Z_1.obj");
  BOOST_CHECK_EQUAL(axis.nBins, 1u);
  CHECK_CLOSE_ABS(axis.max, 3, 1e-6);
  CHECK_CLOSE_ABS(axis.min, 0, 1e-6);
  BOOST_CHECK_EQUAL(axis.bType, equidistant);

  // z rows with varying starting point
  for (std::size_t i = 0; i <= 20; i++) {
    double z0 = -10 + 1. * i;
    surfaces = straightLineSurfaces(10, 3, Vector3(0, 0, z0 + 1.5));
    surfacesRaw = unpack_shared_vector(surfaces);
    pl = ProtoLayer(tgContext, surfacesRaw);
    trf = Transform3::Identity();
    axis = createEquidistantAxis(tgContext, surfacesRaw, AxisDirection::AxisZ,
                                 pl, trf);
    draw_surfaces(
        surfaces,
        (boost::format(
             "SurfaceArrayCreator_createEquidistantAxis_Z_2_%1%.obj") %
         i)
            .str());
    BOOST_CHECK_EQUAL(axis.nBins, 10u);
    CHECK_CLOSE_ABS(axis.max, 30 + z0, 1e-6);
    CHECK_CLOSE_ABS(axis.min, z0, 1e-6);
    BOOST_CHECK_EQUAL(axis.bType, equidistant);
  }

  // z row where elements are rotated around y
  Transform3 tr = Transform3::Identity();
  tr.rotate(AngleAxis3(std::numbers::pi / 4., Vector3(0, 0, 1)));
  surfaces = straightLineSurfaces(10, 3, Vector3(0, 0, 0 + 1.5), tr);
  surfacesRaw = unpack_shared_vector(surfaces);
  pl = ProtoLayer(tgContext, surfacesRaw);
  trf = Transform3::Identity();
  axis = createEquidistantAxis(tgContext, surfacesRaw, AxisDirection::AxisZ, pl,
                               trf);
  draw_surfaces(surfaces, "SurfaceArrayCreator_createEquidistantAxis_Z_3.obj");
  BOOST_CHECK_EQUAL(axis.nBins, 10u);
  CHECK_CLOSE_ABS(axis.max, 30.9749, 1e-3);
  CHECK_CLOSE_ABS(axis.min, -0.974873, 1e-3);
  BOOST_CHECK_EQUAL(axis.bType, equidistant);
}

BOOST_FIXTURE_TEST_CASE(SurfaceArrayCreator_createEquidistantAxis_R,
                        SurfaceArrayCreatorFixture) {
  // single element in r
  auto surfaces = fullPhiTestSurfacesEC(1, 0, 0, 15);
  auto surfacesRaw = unpack_shared_vector(surfaces);
  draw_surfaces(surfaces, "SurfaceArrayCreator_createEquidistantAxis_R_1.obj");
  auto trf = Transform3::Identity();
  ProtoLayer pl = ProtoLayer(tgContext, surfacesRaw);
  auto axis = createEquidistantAxis(tgContext, surfacesRaw,
                                    AxisDirection::AxisR, pl, trf);
  BOOST_CHECK_EQUAL(axis.nBins, 1u);
  CHECK_CLOSE_ABS(axis.max, perp(Vector3(17, 1, 0)), 1e-3);
  CHECK_CLOSE_ABS(axis.min, 13, 1e-3);
  BOOST_CHECK_EQUAL(axis.bType, equidistant);

  // multiple rings
  surfaces.resize(0);
  auto ringa = fullPhiTestSurfacesEC(30, 0, 0, 10);
  surfaces.insert(surfaces.end(), ringa.begin(), ringa.end());
  auto ringb = fullPhiTestSurfacesEC(30, 0, 0, 15);
  surfaces.insert(surfaces.end(), ringb.begin(), ringb.end());
  auto ringc = fullPhiTestSurfacesEC(30, 0, 0, 20);
  surfaces.insert(surfaces.end(), ringc.begin(), ringc.end());
  draw_surfaces(surfaces, "SurfaceArrayCreator_createEquidistantAxis_R_2.obj");

  surfacesRaw = unpack_shared_vector(surfaces);
  pl = ProtoLayer(tgContext, surfacesRaw);
  trf = Transform3::Identity();
  axis = createEquidistantAxis(tgContext, surfacesRaw, AxisDirection::AxisR, pl,
                               trf);

  BOOST_CHECK_EQUAL(axis.nBins, 3u);
  CHECK_CLOSE_REL(axis.max, perp(Vector3(20 + 2, 1, 0)), 1e-3);
  CHECK_CLOSE_ABS(axis.min, 8, 1e-3);
  BOOST_CHECK_EQUAL(axis.bType, equidistant);
}

// if there are concentring disc or barrel modules, the bin count might be off
// we want to create _as few bins_ as possible, meaning the r-ring with
// the lowest number of surfaces should be used for the bin count or
// as basis for the variable edge procedure
// double filling will make sure no surfaces are dropped
BOOST_FIXTURE_TEST_CASE(SurfaceArrayCreator_dependentBinCounts,
                        SurfaceArrayCreatorFixture) {
  auto ringA = fullPhiTestSurfacesEC(10, 0, 0, 10, 2, 3);
  auto ringB = fullPhiTestSurfacesEC(15, 0, 0, 15, 2, 3.5);
  auto ringC = fullPhiTestSurfacesEC(20, 0, 0, 20, 2, 3.8);

  std::vector<std::shared_ptr<const Surface>> surfaces;
  std::copy(ringA.begin(), ringA.end(), std::back_inserter(surfaces));
  std::copy(ringB.begin(), ringB.end(), std::back_inserter(surfaces));
  std::copy(ringC.begin(), ringC.end(), std::back_inserter(surfaces));
  draw_surfaces(surfaces, "SurfaceArrayCreator_dependentBinCounts.obj");

  std::unique_ptr<SurfaceArray> sArray =
      m_SAC.surfaceArrayOnDisc(tgContext, surfaces, equidistant, equidistant);
  auto axes = sArray->getAxes();
  BOOST_CHECK_EQUAL(axes.at(0)->getNBins(), 3u);
  BOOST_CHECK_EQUAL(axes.at(1)->getNBins(), 10u);

  // Write the surrace array with grid
  ObjVisualization3D objVis;
  GeometryView3D::drawSurfaceArray(objVis, *sArray, tgContext);
  objVis.write("SurfaceArrayCreator_EndcapGrid");
}

BOOST_FIXTURE_TEST_CASE(SurfaceArrayCreator_completeBinning,
                        SurfaceArrayCreatorFixture) {
  SrfVec brl = makeBarrel(30, 7, 2, 1);
  std::vector<const Surface*> brlRaw = unpack_shared_vector(brl);
  draw_surfaces(brl, "SurfaceArrayCreator_completeBinning_BRL.obj");

  Axis<AxisType::Equidistant, AxisBoundaryType::Closed> phiAxis(
      -std::numbers::pi, std::numbers::pi, 30u);
  Axis<AxisType::Equidistant, AxisBoundaryType::Bound> zAxis(-14, 14, 7u);

  double R = 10.;
  auto globalToLocal = [](const Vector3& pos) {
    return Vector2(phi(pos) + 2 * std::numbers::pi / 30 / 2, pos.z());
  };
  auto localToGlobal = [R](const Vector2& loc) {
    double phi = loc[0] - 2 * std::numbers::pi / 30 / 2;
    return Vector3(R * std::cos(phi), R * std::sin(phi), loc[1]);
  };

  auto sl = std::make_unique<
      SurfaceArray::SurfaceGridLookup<decltype(phiAxis), decltype(zAxis)>>(
      globalToLocal, localToGlobal,
      std::make_tuple(std::move(phiAxis), std::move(zAxis)));
  sl->fill(tgContext, brlRaw);
  SurfaceArray sa(std::move(sl), brl);

  // Write the surrace array with grid
  ObjVisualization3D objVis;
  GeometryView3D::drawSurfaceArray(objVis, sa, tgContext);
  objVis.write("SurfaceArrayCreator_BarrelGrid");

  // actually filled SA
  for (const auto& srf : brl) {
    Vector3 ctr = srf->referencePosition(tgContext, AxisDirection::AxisR);
    auto binContent = sa.at(ctr);

    BOOST_CHECK_EQUAL(binContent.size(), 1u);
    BOOST_CHECK_EQUAL(srf.get(), binContent.at(0));
  }
}

BOOST_FIXTURE_TEST_CASE(SurfaceArrayCreator_barrelStagger,
                        SurfaceArrayCreatorFixture) {
  auto barrel = makeBarrelStagger(30, 7, 0, std::numbers::pi / 9.);
  auto brl = barrel.first;
  std::vector<const Surface*> brlRaw = unpack_shared_vector(brl);
  draw_surfaces(brl, "SurfaceArrayCreator_barrelStagger.obj");

  ProtoLayer pl(tgContext, brl);

  // EQUIDISTANT
  Transform3 tr = Transform3::Identity();

  auto pAxisPhi =
      createEquidistantAxis(tgContext, brlRaw, AxisDirection::AxisPhi, pl, tr);
  auto pAxisZ =
      createEquidistantAxis(tgContext, brlRaw, AxisDirection::AxisZ, pl, tr);

  double R = 10.;
  Transform3 itr = tr.inverse();

  auto globalToLocal = [tr](const Vector3& pos) {
    Vector3 rot = tr * pos;
    return Vector2(phi(rot), rot.z());
  };
  auto localToGlobal = [R, itr](const Vector2& loc) {
    return itr * Vector3(R * std::cos(loc[0]), R * std::sin(loc[0]), loc[1]);
  };

  auto sl = makeSurfaceGridLookup2D<AxisBoundaryType::Closed,
                                    AxisBoundaryType::Bound>(
      globalToLocal, localToGlobal, pAxisPhi, pAxisZ);

  sl->fill(tgContext, brlRaw);
  SurfaceArray sa(std::move(sl), brl);
  auto axes = sa.getAxes();
  BOOST_CHECK_EQUAL(axes.at(0)->getNBins(), 30u);
  BOOST_CHECK_EQUAL(axes.at(1)->getNBins(), 7u);

  for (const auto& pr : barrel.second) {
    auto A = pr.first;
    auto B = pr.second;

    Vector3 ctr = A->referencePosition(tgContext, AxisDirection::AxisR);
    auto binContent = sa.at(ctr);
    BOOST_CHECK_EQUAL(binContent.size(), 2u);
    std::set<const Surface*> act;
    act.insert(binContent[0]);
    act.insert(binContent[1]);

    std::set<const Surface*> exp;
    exp.insert(A);
    exp.insert(B);

    BOOST_CHECK(act == exp);
  }

  // VARIABLE
  BOOST_TEST_CONTEXT("Barrel Stagger Variable binning") {
    tr = Transform3::Identity();

    auto pAxisPhiVar =
        createVariableAxis(tgContext, brlRaw, AxisDirection::AxisPhi, pl, tr);
    auto pAxisZVar =
        createVariableAxis(tgContext, brlRaw, AxisDirection::AxisZ, pl, tr);

    itr = tr.inverse();

    auto globalToLocalVar = [tr](const Vector3& pos) {
      Vector3 rot = tr * pos;
      return Vector2(phi(rot), rot.z());
    };
    auto localToGlobalVar = [R, itr](const Vector2& loc) {
      return itr * Vector3(R * std::cos(loc[0]), R * std::sin(loc[0]), loc[1]);
    };

    auto sl2 = makeSurfaceGridLookup2D<AxisBoundaryType::Closed,
                                       AxisBoundaryType::Bound>(
        globalToLocalVar, localToGlobalVar, pAxisPhiVar, pAxisZVar);

    sl2->fill(tgContext, brlRaw);
    SurfaceArray sa2(std::move(sl2), brl);
    axes = sa2.getAxes();
    BOOST_CHECK_EQUAL(axes.at(0)->getNBins(), 30u);
    BOOST_CHECK_EQUAL(axes.at(1)->getNBins(), 7u);

    // check bin edges
    std::vector<double> phiEdgesExp = {
        -3.14159,  -2.93215,  -2.72271, -2.51327,    -2.30383, -2.0944,
        -1.88496,  -1.67552,  -1.46608, -1.25664,    -1.0472,  -0.837758,
        -0.628319, -0.418879, -0.20944, 4.44089e-16, 0.20944,  0.418879,
        0.628319,  0.837758,  1.0472,   1.25664,     1.46608,  1.67552,
        1.88496,   2.0944,    2.30383,  2.51327,     2.72271,  3.00831,
        3.14159};
    std::vector<double> zEdgesExp = {-14, -10, -6, -2, 2, 6, 10, 14};
    std::size_t i = 0;
    for (const auto& edge : axes.at(0)->getBinEdges()) {
      BOOST_TEST_INFO("phi edge index " << i);
      auto phiEdge = phiEdgesExp.at(i);
      CHECK_CLOSE_ABS(edge, phiEdge, 1e-5 * std::numbers::pi);
      i++;
    }
    i = 0;
    for (const auto& edge : axes.at(1)->getBinEdges()) {
      BOOST_TEST_INFO("z edge index " << i);
      CHECK_CLOSE_ABS(edge, zEdgesExp.at(i), 1e-6);
      i++;
    }

    for (const auto& pr : barrel.second) {
      auto A = pr.first;
      auto B = pr.second;

      Vector3 ctr = A->referencePosition(tgContext, AxisDirection::AxisR);
      auto binContent = sa2.at(ctr);
      BOOST_CHECK_EQUAL(binContent.size(), 2u);
      std::set<const Surface*> act;
      act.insert(binContent[0]);
      act.insert(binContent[1]);

      std::set<const Surface*> exp;
      exp.insert(A);
      exp.insert(B);

      BOOST_CHECK(act == exp);
    }
  }
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace Acts::Test
