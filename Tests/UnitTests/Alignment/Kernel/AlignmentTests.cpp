// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/EventData/detail/TestSourceLink.hpp"
#include "Acts/Geometry/CuboidVolumeBounds.hpp"
#include "Acts/Geometry/CuboidVolumeBuilder.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/LayerArrayCreator.hpp"
#include "Acts/Geometry/LayerCreator.hpp"
#include "Acts/Geometry/PlaneLayer.hpp"
#include "Acts/Geometry/SurfaceArrayCreator.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Geometry/TrackingGeometryBuilder.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/MagneticField/ConstantBField.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Material/HomogeneousSurfaceMaterial.hpp"
#include "Acts/Material/ISurfaceMaterial.hpp"
#include "Acts/Propagator/EigenStepper.hpp"
#include "Acts/Propagator/Navigator.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Propagator/StraightLineStepper.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Tests/CommonHelpers/DetectorElementStub.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Tests/CommonHelpers/MeasurementsCreator.hpp"
#include "Acts/Tests/CommonHelpers/PredefinedMaterials.hpp"
#include "Acts/TrackFitting/GainMatrixSmoother.hpp"
#include "Acts/TrackFitting/GainMatrixUpdater.hpp"
#include "Acts/TrackFitting/KalmanFitter.hpp"
#include "Acts/TrackFitting/detail/KalmanGlobalCovariance.hpp"
#include "Acts/Utilities/CalibrationContext.hpp"
#include "ActsAlignment/Kernel/Alignment.hpp"

#include <cmath>
#include <random>
#include <string>

namespace {
using namespace Acts;
using namespace ActsAlignment;
using namespace Acts::Test;
using namespace Acts::detail::Test;
using namespace Acts::UnitLiterals;

using StraightPropagator =
    Acts::Propagator<Acts::StraightLineStepper, Acts::Navigator>;
using ConstantFieldStepper = Acts::EigenStepper<>;
using ConstantFieldPropagator =
    Acts::Propagator<ConstantFieldStepper, Acts::Navigator>;

using KalmanUpdater = Acts::GainMatrixUpdater;
using KalmanSmoother = Acts::GainMatrixSmoother;
using KalmanFitterType =
    Acts::KalmanFitter<ConstantFieldPropagator, VectorMultiTrajectory>;

KalmanUpdater kfUpdater;
KalmanSmoother kfSmoother;

// Create a test context
const GeometryContext geoCtx;
const MagneticFieldContext magCtx;
const CalibrationContext calCtx;

std::normal_distribution<double> normalDist(0., 1.);
std::default_random_engine rng(42);

KalmanFitterExtensions<VectorMultiTrajectory> getExtensions() {
  KalmanFitterExtensions<VectorMultiTrajectory> extensions;
  extensions.calibrator
      .connect<&testSourceLinkCalibrator<VectorMultiTrajectory>>();
  extensions.updater.connect<&KalmanUpdater::operator()<VectorMultiTrajectory>>(
      &kfUpdater);
  extensions.smoother
      .connect<&KalmanSmoother::operator()<VectorMultiTrajectory>>(&kfSmoother);
  return extensions;
}

///
/// @brief Construct a telescope-like detector
///
struct TelescopeDetector {
  /// Default constructor for the Cubic tracking geometry
  ///
  /// @param gctx the geometry context for this geometry at building time
  TelescopeDetector(std::reference_wrapper<const GeometryContext> gctx)
      : geoContext(gctx) {
    // Construct the rotation
    rotation.col(0) = Acts::Vector3(0, 0, -1);
    rotation.col(1) = Acts::Vector3(0, 1, 0);
    rotation.col(2) = Acts::Vector3(1, 0, 0);

    // Boundaries of the surfaces
    rBounds = std::make_shared<const RectangleBounds>(0.1_m, 0.1_m);

    // Material of the surfaces
    MaterialSlab matProp(makeSilicon(), 80_um);

    surfaceMaterial = std::make_shared<HomogeneousSurfaceMaterial>(matProp);
  }

  ///
  /// Call operator to build the standard cubic tracking geometry
  ///
  std::shared_ptr<const TrackingGeometry> operator()() {
    using namespace UnitLiterals;

    unsigned int nLayers = 6;
    std::vector<ActsScalar> positions = {-500_mm, -300_mm, -100_mm,
                                         100_mm,  300_mm,  500_mm};
    auto length = positions.back() - positions.front();

    std::vector<LayerPtr> layers(nLayers);
    for (unsigned int i = 0; i < nLayers; ++i) {
      // The transform
      Translation3 trans(0., 0., positions[i]);
      Transform3 trafo(rotation * trans);
      auto detElement = std::make_shared<DetectorElementStub>(
          trafo, rBounds, 1._um, surfaceMaterial);
      // The surface is not right!!!
      auto surface = detElement->surface().getSharedPtr();
      // Add it to the event store
      detectorStore.push_back(std::move(detElement));
      std::unique_ptr<SurfaceArray> surArray(new SurfaceArray(surface));
      // The layer thickness should not be too large
      layers[i] =
          PlaneLayer::create(trafo, rBounds, std::move(surArray),
                             1._mm);  // Associate the layer to the surface
      auto mutableSurface = const_cast<Surface*>(surface.get());
      mutableSurface->associateLayer(*layers[i]);
    }

    // The volume transform
    Translation3 transVol(0, 0, 0);
    Transform3 trafoVol(rotation * transVol);
    auto boundsVol = std::make_shared<CuboidVolumeBounds>(
        rBounds->halfLengthX() + 10._mm, rBounds->halfLengthY() + 10._mm,
        length + 10._mm);

    LayerArrayCreator::Config lacConfig;
    LayerArrayCreator layArrCreator(
        lacConfig, getDefaultLogger("LayerArrayCreator", Logging::INFO));
    LayerVector layVec;
    for (unsigned int i = 0; i < nLayers; i++) {
      layVec.push_back(layers[i]);
    }

    // Create the layer array
    std::unique_ptr<const LayerArray> layArr(layArrCreator.layerArray(
        geoContext, layVec, positions.front() - 2._mm, positions.back() + 2._mm,
        BinningType::arbitrary, BinningValue::binX));

    // Build the tracking volume
    auto trackVolume = std::make_shared<TrackingVolume>(
        trafoVol, boundsVol, nullptr, std::move(layArr), nullptr,
        MutableTrackingVolumeVector{}, "Telescope");

    return std::make_shared<const TrackingGeometry>(trackVolume);
  }

  RotationMatrix3 rotation = RotationMatrix3::Identity();
  std::shared_ptr<const RectangleBounds> rBounds = nullptr;
  std::shared_ptr<const ISurfaceMaterial> surfaceMaterial = nullptr;

  std::vector<std::shared_ptr<DetectorElementStub>> detectorStore;

  std::reference_wrapper<const GeometryContext> geoContext;
};

// Construct a straight-line propagator.
StraightPropagator makeStraightPropagator(
    std::shared_ptr<const TrackingGeometry> geo) {
  Navigator::Config cfg{std::move(geo)};
  cfg.resolvePassive = false;
  cfg.resolveMaterial = true;
  cfg.resolveSensitive = true;
  Navigator navigator(cfg);
  StraightLineStepper stepper;
  return StraightPropagator(stepper, std::move(navigator));
}

// Construct a propagator using a constant magnetic field along z.
ConstantFieldPropagator makeConstantFieldPropagator(
    std::shared_ptr<const TrackingGeometry> geo, double bz,
    std::unique_ptr<const Logger> logger) {
  Navigator::Config cfg{std::move(geo)};
  cfg.resolvePassive = false;
  cfg.resolveMaterial = true;
  cfg.resolveSensitive = true;
  Navigator navigator(cfg, logger->cloneWithSuffix("Nav"));
  auto field = std::make_shared<ConstantBField>(Vector3(0.0, 0.0, bz));
  ConstantFieldStepper stepper(std::move(field));
  return ConstantFieldPropagator(std::move(stepper), std::move(navigator),
                                 logger->cloneWithSuffix("Prop"));
}

// Construct initial track parameters.
CurvilinearTrackParameters makeParameters() {
  // create covariance matrix from reasonable standard deviations
  BoundVector stddev;
  stddev[eBoundLoc0] = 100_um;
  stddev[eBoundLoc1] = 100_um;
  stddev[eBoundTime] = 25_ns;
  stddev[eBoundPhi] = 0.5_degree;
  stddev[eBoundTheta] = 0.5_degree;
  stddev[eBoundQOverP] = 1 / 100_GeV;
  BoundSquareMatrix cov = stddev.cwiseProduct(stddev).asDiagonal();

  auto loc0 = 0. + stddev[eBoundLoc0] * normalDist(rng);
  auto loc1 = 0. + stddev[eBoundLoc1] * normalDist(rng);
  auto t = 42_ns + stddev[eBoundTime] * normalDist(rng);
  auto phi = 0_degree + stddev[eBoundPhi] * normalDist(rng);
  auto theta = 90_degree + stddev[eBoundTheta] * normalDist(rng);
  auto qOverP = 1_e / 1_GeV + stddev[eBoundQOverP] * normalDist(rng);

  // define a track in the transverse plane along x
  Vector4 mPos4(-1_m, loc0, loc1, t);

  return CurvilinearTrackParameters(mPos4, phi, theta, qOverP, cov,
                                    ParticleHypothesis::pion());
}

// detector resolutions
const MeasurementResolution resPixel = {MeasurementType::eLoc01,
                                        {30_um, 50_um}};
const MeasurementResolutionMap resolutions = {
    {GeometryIdentifier(), resPixel},
};

struct KalmanFitterInputTrajectory {
  // The source links
  std::vector<TestSourceLink> sourceLinks;
  // The start parameters
  std::optional<CurvilinearTrackParameters> startParameters;
};

///
/// Function to create trajectories for kalman fitter
///
std::vector<KalmanFitterInputTrajectory> createTrajectories(
    std::shared_ptr<const TrackingGeometry> geo, std::size_t nTrajectories) {
  // simulation propagator
  const auto simPropagator = makeStraightPropagator(std::move(geo));

  std::vector<KalmanFitterInputTrajectory> trajectories;
  trajectories.reserve(nTrajectories);

  for (unsigned int iTrack = 0; iTrack < nTrajectories; iTrack++) {
    auto start = makeParameters();
    // Launch and collect - the measurements
    auto measurements = createMeasurements(simPropagator, geoCtx, magCtx, start,
                                           resolutions, rng);

    // Extract measurements from result of propagation.
    KalmanFitterInputTrajectory traj;
    traj.startParameters = start;
    traj.sourceLinks = measurements.sourceLinks;

    trajectories.push_back(std::move(traj));
  }
  return trajectories;
}
}  // namespace

///
/// @brief Unit test for KF-based alignment algorithm
///
BOOST_AUTO_TEST_CASE(ZeroFieldKalmanAlignment) {
  // Build detector
  TelescopeDetector detector(geoCtx);
  const auto geometry = detector();

  // reconstruction propagator and fitter
  auto kfLogger = getDefaultLogger("KalmanFilter", Logging::INFO);
  const auto kfZeroPropagator =
      makeConstantFieldPropagator(geometry, 0_T, std::move(kfLogger));
  auto kfZero = KalmanFitterType(kfZeroPropagator);

  // alignment
  auto alignLogger = getDefaultLogger("Alignment", Logging::INFO);
  const auto alignZero = Alignment(std::move(kfZero), std::move(alignLogger));

  // Create 10 trajectories
  const auto& trajectories = createTrajectories(geometry, 10);

  // Construct the KalmanFitter options

  auto extensions = getExtensions();
  TestSourceLink::SurfaceAccessor surfaceAccessor{*geometry};
  extensions.surfaceAccessor
      .connect<&TestSourceLink::SurfaceAccessor::operator()>(&surfaceAccessor);
  KalmanFitterOptions kfOptions(geoCtx, magCtx, calCtx, extensions,
                                PropagatorPlainOptions(geoCtx, magCtx));

  // Construct a non-updating alignment updater
  AlignedTransformUpdater voidAlignUpdater =
      [](DetectorElementBase* /*element*/, const GeometryContext& /*gctx*/,
         const Transform3& /*transform*/) { return true; };

  // Construct the alignment options
  AlignmentOptions<KalmanFitterOptions<VectorMultiTrajectory>> alignOptions(
      kfOptions, voidAlignUpdater);
  alignOptions.maxIterations = 1;

  // Set the surfaces to be aligned (fix the layer 8)
  unsigned int iSurface = 0;
  std::unordered_map<const Surface*, std::size_t> idxedAlignSurfaces;
  // Loop over the detector elements
  for (auto& det : detector.detectorStore) {
    const auto& surface = det->surface();
    if (surface.geometryId().layer() != 8) {
      alignOptions.alignedDetElements.push_back(det.get());
      idxedAlignSurfaces.emplace(&surface, iSurface);
      iSurface++;
    }
  }

  // Test the method to evaluate alignment state for a single track
  const auto& inputTraj = trajectories.front();
  kfOptions.referenceSurface = &(*inputTraj.startParameters).referenceSurface();

  auto evaluateRes = alignZero.evaluateTrackAlignmentState(
      kfOptions.geoContext, inputTraj.sourceLinks, *inputTraj.startParameters,
      kfOptions, idxedAlignSurfaces, AlignmentMask::All);
  BOOST_CHECK(evaluateRes.ok());

  const auto& alignState = evaluateRes.value();
  CHECK_CLOSE_ABS(alignState.chi2 / alignState.alignmentDof, 0.5, 1);

  // Check the dimensions
  BOOST_CHECK_EQUAL(alignState.measurementDim, 12);
  BOOST_CHECK_EQUAL(alignState.trackParametersDim, 36);
  // Check the alignment dof
  BOOST_CHECK_EQUAL(alignState.alignmentDof, 30);
  BOOST_CHECK_EQUAL(alignState.alignedSurfaces.size(), 5);
  // Check the measurements covariance
  BOOST_CHECK_EQUAL(alignState.measurementCovariance.rows(), 12);
  const SquareMatrix2 measCov =
      alignState.measurementCovariance.block<2, 2>(2, 2);
  SquareMatrix2 cov2D;
  cov2D << 30_um * 30_um, 0, 0, 50_um * 50_um;
  CHECK_CLOSE_ABS(measCov, cov2D, 1e-10);
  // Check the track parameters covariance matrix. Its rows/columns scales
  // with the number of measurement states
  BOOST_CHECK_EQUAL(alignState.trackParametersCovariance.rows(), 36);
  // Check the projection matrix
  BOOST_CHECK_EQUAL(alignState.projectionMatrix.rows(), 12);
  BOOST_CHECK_EQUAL(alignState.projectionMatrix.cols(), 36);
  const ActsMatrix<2, 6> proj = alignState.projectionMatrix.block<2, 6>(0, 0);
  const ActsMatrix<2, 6> refProj = ActsMatrix<2, 6>::Identity();
  CHECK_CLOSE_ABS(proj, refProj, 1e-10);
  // Check the residual
  BOOST_CHECK_EQUAL(alignState.residual.size(), 12);
  // Check the residual covariance
  BOOST_CHECK_EQUAL(alignState.residualCovariance.rows(), 12);
  // Check the alignment to residual derivative
  BOOST_CHECK_EQUAL(alignState.alignmentToResidualDerivative.rows(), 12);
  BOOST_CHECK_EQUAL(alignState.alignmentToResidualDerivative.cols(), 30);
  // Check the chi2 derivative
  BOOST_CHECK_EQUAL(alignState.alignmentToChi2Derivative.size(), 30);
  BOOST_CHECK_EQUAL(alignState.alignmentToChi2SecondDerivative.rows(), 30);

  // Test the align method
  std::vector<std::vector<TestSourceLink>> trajCollection;
  trajCollection.reserve(10);
  std::vector<CurvilinearTrackParameters> sParametersCollection;
  sParametersCollection.reserve(10);
  for (const auto& traj : trajectories) {
    trajCollection.push_back(traj.sourceLinks);
    sParametersCollection.push_back(*traj.startParameters);
  }
  auto alignRes =
      alignZero.align(trajCollection, sParametersCollection, alignOptions);

  // BOOST_CHECK(alignRes.ok());
}
