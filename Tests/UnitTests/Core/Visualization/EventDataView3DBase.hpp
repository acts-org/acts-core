// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/EventData/MeasurementHelpers.hpp"
#include "Acts/EventData/SourceLink.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/EventData/VectorMultiTrajectory.hpp"
#include "Acts/EventData/VectorTrackContainer.hpp"
#include "Acts/EventData/detail/TestSourceLink.hpp"
#include "Acts/Geometry/CuboidVolumeBuilder.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Geometry/TrackingGeometryBuilder.hpp"
#include "Acts/MagneticField/ConstantBField.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Material/HomogeneousSurfaceMaterial.hpp"
#include "Acts/Material/ISurfaceMaterial.hpp"
#include "Acts/Propagator/EigenStepper.hpp"
#include "Acts/Propagator/Navigator.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Tests/CommonHelpers/DetectorElementStub.hpp"
#include "Acts/Tests/CommonHelpers/PredefinedMaterials.hpp"
#include "Acts/TrackFitting/GainMatrixSmoother.hpp"
#include "Acts/TrackFitting/GainMatrixUpdater.hpp"
#include "Acts/TrackFitting/KalmanFitter.hpp"
#include "Acts/Utilities/CalibrationContext.hpp"
#include "Acts/Visualization/EventDataView3D.hpp"
#include "Acts/Visualization/IVisualization3D.hpp"

#include <cmath>
#include <fstream>
#include <optional>
#include <random>
#include <sstream>
#include <string>

using Acts::VectorHelpers::makeVector4;

namespace Acts::EventDataView3DTest {

using Covariance = BoundSquareMatrix;

std::normal_distribution<double> gauss(0., 1.);
std::default_random_engine generator(42);

/// A function that creates a simple telescope detector with surfaces for the
/// EventDataView3D tests
///
/// @param helper The visualization helper
/// @param surfaces Reference to the surfaces, because we need them outside
/// @param detector The tracking geometry that will be filled
/// @param nSurfaces Number of surfaces to generate
///
/// @return an overall string including all written output
void createDetector(GeometryContext& tgContext,
                    std::vector<const Surface*>& surfaces,
                    std::shared_ptr<const TrackingGeometry>& detector,
                    const std::size_t nSurfaces = 7) {
  using namespace UnitLiterals;

  if (nSurfaces < 1) {
    throw std::invalid_argument("At least 1 surfaces needs to be created.");
  }

  // Construct the rotation
  RotationMatrix3 rotation = RotationMatrix3::Identity();
  double rotationAngle = 90_degree;
  Vector3 xPos(cos(rotationAngle), 0., sin(rotationAngle));
  Vector3 yPos(0., 1., 0.);
  Vector3 zPos(-sin(rotationAngle), 0., cos(rotationAngle));
  rotation.col(0) = xPos;
  rotation.col(1) = yPos;
  rotation.col(2) = zPos;

  // Boundaries of the surfaces
  const auto rBounds =
      std::make_shared<const RectangleBounds>(RectangleBounds(50_mm, 50_mm));

  // Material of the surfaces
  MaterialSlab matProp(Acts::Test::makeSilicon(), 0.5_mm);
  const auto surfaceMaterial =
      std::make_shared<HomogeneousSurfaceMaterial>(matProp);

  // Set translation vectors
  std::vector<Vector3> translations;
  translations.reserve(nSurfaces);
  for (unsigned int i = 0; i < nSurfaces; i++) {
    translations.push_back({i * 100_mm - 300_mm, 0., 0.});
  }

  // Construct layer configs
  std::vector<CuboidVolumeBuilder::LayerConfig> lConfs;
  lConfs.reserve(nSurfaces);
  for (unsigned int i = 0; i < translations.size(); i++) {
    CuboidVolumeBuilder::SurfaceConfig sConf;
    sConf.position = translations[i];
    sConf.rotation = rotation;
    sConf.rBounds = rBounds;
    sConf.surMat = surfaceMaterial;
    // The thickness to construct the associated detector element
    sConf.thickness = 1._um;
    sConf.detElementConstructor =
        [](const Transform3& trans,
           const std::shared_ptr<const RectangleBounds>& bounds,
           double thickness) {
          return new Test::DetectorElementStub(trans, bounds, thickness);
        };
    CuboidVolumeBuilder::LayerConfig lConf;
    lConf.surfaceCfg = {sConf};
    lConfs.push_back(lConf);
  }

  // Construct volume config
  CuboidVolumeBuilder::VolumeConfig vConf;
  vConf.position = {0., 0., 0.};
  vConf.length = {1.2_m, 1._m, 1._m};
  vConf.layerCfg = lConfs;
  vConf.name = "Tracker";

  // Construct volume builder config
  CuboidVolumeBuilder::Config conf;
  conf.position = {0., 0., 0.};
  conf.length = {1.2_m, 1._m, 1._m};
  conf.volumeCfg = {vConf};  // one volume

  // Build detector
  std::cout << "Build the detector" << std::endl;
  CuboidVolumeBuilder cvb;
  cvb.setConfig(conf);
  TrackingGeometryBuilder::Config tgbCfg;
  tgbCfg.trackingVolumeBuilders.push_back(
      [=](const auto& context, const auto& inner, const auto& vb) {
        return cvb.trackingVolume(context, inner, vb);
      });
  TrackingGeometryBuilder tgb(tgbCfg);
  detector = tgb.trackingGeometry(tgContext);

  // Get the surfaces;
  surfaces.reserve(nSurfaces);
  detector->visitSurfaces([&](const Surface* surface) {
    if (surface != nullptr && surface->associatedDetectorElement() != nullptr) {
      std::cout << "surface " << surface->geometryId() << " placed at: ("
                << surface->center(tgContext).transpose() << " )" << std::endl;
      surfaces.push_back(surface);
    }
  });
  std::cout << "There are " << surfaces.size() << " surfaces" << std::endl;
}

/// Helper method to visualize all types of surfaces
///
/// @param helper The visualization helper
///
/// @return an overall string including all written output
static inline std::string testBoundTrackParameters(IVisualization3D& helper) {
  std::stringstream ss;

  ViewConfig pcolor{.color = {20, 120, 20}};
  ViewConfig scolor{.color = {235, 198, 52}};

  auto gctx = GeometryContext();
  auto identity = Transform3::Identity();

  // rectangle and plane
  auto rectangle = std::make_shared<RectangleBounds>(15., 15.);
  auto plane = Surface::makeShared<PlaneSurface>(identity, rectangle);

  double momentumScale = 0.005;
  double localErrorScale = 10.;
  double directionErrorScale = 1000.;

  // now create parameters on this surface
  // l_x, l_y, phi, theta, q/p (1/p), t
  std::array<double, 6> pars_array = {
      {-0.1234, 4.8765, 0.45, 0.128, 0.001, 21.}};

  BoundTrackParameters::ParametersVector pars =
      BoundTrackParameters::ParametersVector::Zero();
  pars << pars_array[0], pars_array[1], pars_array[2], pars_array[3],
      pars_array[4], pars_array[5];

  Covariance cov = Covariance::Zero();
  cov << 0.25, 0.0042, -0.00076, 6.156e-06, -2.11e-07, 0, 0.0042, 0.859,
      -0.000173, 0.000916, -4.017e-08, 0, -0.00076, -0.000173, 2.36e-04,
      -2.76e-07, 1.12e-08, 0, 6.15e-06, 0.000916, -2.76e-07, 8.84e-04,
      -2.85e-11, 0, -2.11 - 07, -4.017e-08, 1.123e-08, -2.85 - 11, 1.26e-10, 0,
      0, 0, 0, 0, 0, 1;

  EventDataView3D::drawBoundTrackParameters(
      helper,
      BoundTrackParameters(plane, pars, std::move(cov),
                           ParticleHypothesis::pion()),
      gctx, momentumScale, localErrorScale, directionErrorScale, pcolor,
      scolor);

  helper.write("EventData_BoundAtPlaneParameters");
  helper.write(ss);

  return ss.str();
}

/// Helper method to visualize measurements
///
/// @param helper The visualization helper
///
/// @return an overall string including all written output
static inline std::string testMeasurement(IVisualization3D& helper,
                                          const double localErrorScale = 100.) {
  using namespace UnitLiterals;
  std::stringstream ss;

  // Create a test context
  GeometryContext tgContext = GeometryContext();

  // Create a detector
  const std::size_t nSurfaces = 7;
  std::vector<const Surface*> surfaces;
  std::shared_ptr<const TrackingGeometry> detector;
  createDetector(tgContext, surfaces, detector, nSurfaces);

  // Create measurements (assuming they are for a linear track parallel to
  // global x-axis)
  std::cout << "Creating measurements:" << std::endl;
  std::vector<detail::Test::TestSourceLink> sourceLinks;
  sourceLinks.reserve(nSurfaces);
  Vector2 lPosCenter{5_mm, 5_mm};
  Vector2 resolution{200_um, 150_um};
  SquareMatrix2 cov2D = resolution.cwiseProduct(resolution).asDiagonal();
  for (const auto& surface : surfaces) {
    // 2D measurements
    Vector2 loc = lPosCenter;
    loc[0] += resolution[0] * gauss(generator);
    loc[1] += resolution[1] * gauss(generator);
    sourceLinks.emplace_back(detail::Test::TestSourceLink{
        eBoundLoc0, eBoundLoc1, loc, cov2D, surface->geometryId()});
  }

  ViewConfig mcolor{.color = {255, 145, 48}};
  mcolor.offset = 0.01;

  // Draw the measurements
  std::cout << "Draw the measurements" << std::endl;
  //  auto singleMeasurement = sourceLinks[0];
  for (auto& singleMeasurement : sourceLinks) {
    auto cov = singleMeasurement.covariance;
    auto lposition = singleMeasurement.parameters;

    auto surf = detector->findSurface(singleMeasurement.m_geometryId);
    auto transf = surf->transform(tgContext);

    EventDataView3D::drawMeasurement(helper, lposition, cov, transf,
                                     localErrorScale, mcolor);
  }

  helper.write("EventData_Measurement");
  helper.write(ss);

  return ss.str();
}

/// Helper method to visualize a MultiTrajectory
///
/// @param helper The visualization helper
///
/// @return an overall string including all written output
static inline std::string testMultiTrajectory(IVisualization3D& helper) {
  using namespace UnitLiterals;
  std::stringstream ss;

  // Create a test context
  GeometryContext tgContext = GeometryContext();
  MagneticFieldContext mfContext = MagneticFieldContext();
  CalibrationContext calContext = CalibrationContext();

  // Create a detector
  const std::size_t nSurfaces = 7;
  std::vector<const Surface*> surfaces;
  std::shared_ptr<const TrackingGeometry> detector;
  createDetector(tgContext, surfaces, detector, nSurfaces);

  // Create measurements (assuming they are for a linear track parallel to
  // global x-axis)
  std::cout << "Creating measurements:" << std::endl;
  std::vector<Acts::SourceLink> sourceLinks;
  sourceLinks.reserve(nSurfaces);
  Vector2 lPosCenter{5_mm, 5_mm};
  Vector2 resolution{200_um, 150_um};
  SquareMatrix2 cov2D = resolution.cwiseProduct(resolution).asDiagonal();
  for (const auto& surface : surfaces) {
    // 2D measurements
    Vector2 loc = lPosCenter;
    loc[0] += resolution[0] * gauss(generator);
    loc[1] += resolution[1] * gauss(generator);
    sourceLinks.emplace_back(detail::Test::TestSourceLink{
        eBoundLoc0, eBoundLoc1, loc, cov2D, surface->geometryId()});
  }

  // The KalmanFitter - we use the eigen stepper for covariance transport
  std::cout << "Construct KalmanFitter and perform fit" << std::endl;
  Navigator::Config cfg{detector};
  cfg.resolvePassive = false;
  cfg.resolveMaterial = true;
  cfg.resolveSensitive = true;
  Navigator rNavigator(cfg);

  // Configure propagation with deactivated B-field
  auto bField = std::make_shared<ConstantBField>(Vector3(0., 0., 0.));
  using RecoStepper = EigenStepper<>;
  RecoStepper rStepper(bField);
  using RecoPropagator = Propagator<RecoStepper, Navigator>;
  RecoPropagator rPropagator(rStepper, rNavigator);

  // Set initial parameters for the particle track
  Covariance cov;
  cov << std::pow(100_um, 2), 0., 0., 0., 0., 0., 0., std::pow(100_um, 2), 0.,
      0., 0., 0., 0., 0., 0.0025, 0., 0., 0., 0., 0., 0., 0.0025, 0., 0., 0.,
      0., 0., 0., 0.01, 0., 0., 0., 0., 0., 0., 1.;
  Vector3 rPos(-350._mm, 100_um * gauss(generator), 100_um * gauss(generator));
  Vector3 rDir(1, 0.025 * gauss(generator), 0.025 * gauss(generator));
  CurvilinearTrackParameters rStart(makeVector4(rPos, 42_ns), rDir, 1_e / 1_GeV,
                                    cov, ParticleHypothesis::pion());

  const Surface* rSurface = &rStart.referenceSurface();

  using KalmanFitter = KalmanFitter<RecoPropagator, VectorMultiTrajectory>;

  KalmanFitter kFitter(rPropagator);

  auto logger = getDefaultLogger("KalmanFilter", Logging::WARNING);

  Acts::GainMatrixUpdater kfUpdater;
  Acts::GainMatrixSmoother kfSmoother;

  KalmanFitterExtensions<VectorMultiTrajectory> extensions;
  extensions.calibrator.connect<
      &detail::Test::testSourceLinkCalibrator<VectorMultiTrajectory>>();
  extensions.updater
      .connect<&Acts::GainMatrixUpdater::operator()<VectorMultiTrajectory>>(
          &kfUpdater);
  extensions.smoother
      .connect<&Acts::GainMatrixSmoother::operator()<VectorMultiTrajectory>>(
          &kfSmoother);

  detail::Test::TestSourceLink::SurfaceAccessor surfaceAccessor{*detector};
  extensions.surfaceAccessor
      .connect<&detail::Test::TestSourceLink::SurfaceAccessor::operator()>(
          &surfaceAccessor);

  KalmanFitterOptions kfOptions(tgContext, mfContext, calContext, extensions,
                                PropagatorPlainOptions(tgContext, mfContext),
                                rSurface);

  Acts::TrackContainer tracks{Acts::VectorTrackContainer{},
                              Acts::VectorMultiTrajectory{}};

  // Fit the track
  auto fitRes = kFitter.fit(sourceLinks.begin(), sourceLinks.end(), rStart,
                            kfOptions, tracks);
  if (!fitRes.ok()) {
    std::cout << "Fit failed" << std::endl;
    return ss.str();
  }
  auto& track = *fitRes;

  // Draw the track
  std::cout << "Draw the fitted track" << std::endl;
  double momentumScale = 10;
  double localErrorScale = 100.;
  double directionErrorScale = 100000;

  ViewConfig scolor{.color = {214, 214, 214}};
  ViewConfig mcolor{.color = {255, 145, 48}};
  mcolor.offset = -0.01;
  ViewConfig ppcolor{.color = {51, 204, 51}};
  ppcolor.offset = -0.02;
  ViewConfig fpcolor{.color = {255, 255, 0}};
  fpcolor.offset = -0.03;
  ViewConfig spcolor{.color = {0, 125, 255}};
  spcolor.offset = -0.04;

  EventDataView3D::drawMultiTrajectory(
      helper, tracks.trackStateContainer(), track.tipIndex(), tgContext,
      momentumScale, localErrorScale, directionErrorScale, scolor, mcolor,
      ppcolor, fpcolor, spcolor);

  helper.write("EventData_MultiTrajectory");
  helper.write(ss);

  return ss.str();
}

}  // namespace Acts::EventDataView3DTest
