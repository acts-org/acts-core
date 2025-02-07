// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Common.hpp"
#include "Acts/Definitions/Direction.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/Charge.hpp"
#include "Acts/EventData/GenericBoundTrackParameters.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/MagneticField/ConstantBField.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/MagneticField/MagneticFieldProvider.hpp"
#include "Acts/MagneticField/NullBField.hpp"
#include "Acts/Propagator/EigenStepper.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Propagator/PropagatorOptions.hpp"
#include "Acts/Propagator/StraightLineStepper.hpp"
#include "Acts/Propagator/VoidNavigator.hpp"
#include "Acts/Surfaces/PerigeeSurface.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/Result.hpp"
#include "Acts/Vertexing/ImpactPointEstimator.hpp"
#include "Acts/Vertexing/TrackAtVertex.hpp"
#include "Acts/Vertexing/Vertex.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <numbers>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

namespace {

namespace bd = boost::unit_test::data;

using namespace Acts;
using namespace Acts::UnitLiterals;
using Acts::VectorHelpers::makeVector4;

using MagneticField = Acts::ConstantBField;
using StraightPropagator = Acts::Propagator<StraightLineStepper>;
using Stepper = Acts::EigenStepper<>;
using Propagator = Acts::Propagator<Stepper>;
using Estimator = Acts::ImpactPointEstimator;
using StraightLineEstimator = Acts::ImpactPointEstimator;

const Acts::GeometryContext geoContext;
const Acts::MagneticFieldContext magFieldContext;

Acts::MagneticFieldProvider::Cache magFieldCache() {
  return NullBField{}.makeCache(magFieldContext);
}

// perigee track parameters dataset
// only non-zero distances are tested
auto d0s = bd::make({-25_um, 25_um});
auto l0s = bd::make({-1_mm, 1_mm});
auto t0s = bd::make({-2_ns, 2_ns});
auto phis = bd::make({0_degree, -45_degree, 45_degree});
auto thetas = bd::make({90_degree, 20_degree, 160_degree});
auto ps = bd::make({0.4_GeV, 1_GeV, 10_GeV});
auto qs = bd::make({-1_e, 1_e});
// Cartesian products over all parameters
auto tracksWithoutIPs = t0s * phis * thetas * ps * qs;
auto IPs = d0s * l0s;
auto tracks = IPs * tracksWithoutIPs;

// vertex parameters dataset
auto vx0s = bd::make({0_um, -10_um, 10_um});
auto vy0s = bd::make({0_um, -10_um, 10_um});
auto vz0s = bd::make({0_mm, -25_mm, 25_mm});
auto vt0s = bd::make({0_ns, -2_ns, 2_ns});
// Cartesian products over all parameters
auto vertices = vx0s * vy0s * vz0s * vt0s;

// Construct an impact point estimator for a constant bfield along z.
Estimator makeEstimator(double bZ) {
  auto field = std::make_shared<MagneticField>(Vector3(0, 0, bZ));
  Stepper stepper(field);
  Estimator::Config cfg(field,
                        std::make_shared<Propagator>(
                            std::move(stepper), VoidNavigator(),
                            getDefaultLogger("Prop", Logging::Level::WARNING)));
  return Estimator(cfg);
}

// Construct a diagonal track covariance w/ reasonable values.
Acts::BoundSquareMatrix makeBoundParametersCovariance(
    double stdDevTime = 30_ps) {
  BoundVector stddev;
  stddev[eBoundLoc0] = 15_um;
  stddev[eBoundLoc1] = 100_um;
  stddev[eBoundTime] = stdDevTime;
  stddev[eBoundPhi] = 1_degree;
  stddev[eBoundTheta] = 1_degree;
  stddev[eBoundQOverP] = 1_e / 100_GeV;
  return stddev.cwiseProduct(stddev).asDiagonal();
}

// Construct a diagonal vertex covariance w/ reasonable values.
Acts::SquareMatrix4 makeVertexCovariance() {
  Vector4 stddev;
  stddev[ePos0] = 10_um;
  stddev[ePos1] = 10_um;
  stddev[ePos2] = 75_um;
  stddev[eTime] = 1_ns;
  return stddev.cwiseProduct(stddev).asDiagonal();
}

// random value between 0 and 1
std::uniform_real_distribution<double> uniformDist(0.0, 1.0);
// random sign
std::uniform_real_distribution<double> signDist(-1, 1);
}  // namespace

BOOST_AUTO_TEST_SUITE(VertexingImpactPointEstimator)

// Check `calculateDistance`, `estimate3DImpactParameters`, and
// `getVertexCompatibility`.
BOOST_DATA_TEST_CASE(SingleTrackDistanceParametersCompatibility3D, tracks, d0,
                     l0, t0, phi, theta, p, q) {
  auto particleHypothesis = ParticleHypothesis::pion();

  BoundVector par;
  par[eBoundLoc0] = d0;
  par[eBoundLoc1] = l0;
  par[eBoundTime] = t0;
  par[eBoundPhi] = phi;
  par[eBoundTheta] = theta;
  par[eBoundQOverP] = particleHypothesis.qOverP(p, q);

  Estimator ipEstimator = makeEstimator(2_T);
  Estimator::State state{magFieldCache()};
  // reference position and corresponding perigee surface
  Vector3 refPosition(0., 0., 0.);
  auto perigeeSurface = Surface::makeShared<PerigeeSurface>(refPosition);
  // create the track
  BoundTrackParameters myTrack(
      perigeeSurface, par, makeBoundParametersCovariance(), particleHypothesis);

  // initial distance to the reference position in the perigee frame
  double distT = std::hypot(d0, l0);
  double dist3 =
      ipEstimator.calculateDistance(geoContext, myTrack, refPosition, state)
          .value();
  // estimated 3D distance should be less than the 2d distance in the perigee
  // frame. it should be equal if the track is a transverse track w/ theta =
  // 90deg. in that case there might be numerical deviations and we need to
  // check that it is less or equal within the numerical tolerance.
  BOOST_CHECK((dist3 < distT) ||
              (theta == 90_degree && std::abs(dist3 - distT) < 1_nm));

  // estimate parameters at the closest point in 3d
  auto res = ipEstimator.estimate3DImpactParameters(
      geoContext, magFieldContext, myTrack, refPosition, state);
  BoundTrackParameters trackAtIP3d = *res;
  const auto& atPerigee = myTrack.parameters();
  const auto& atIp3d = trackAtIP3d.parameters();

  // all parameters except the helix invariants theta, q/p should be changed
  BOOST_CHECK_NE(atPerigee[eBoundLoc0], atIp3d[eBoundLoc0]);
  BOOST_CHECK_NE(atPerigee[eBoundLoc1], atIp3d[eBoundLoc1]);
  // BOOST_CHECK_NE(atPerigee[eBoundTime], atIp3d[eBoundTime]);
  // BOOST_CHECK_NE(atPerigee[eBoundPhi], atIp3d[eBoundPhi]);
  CHECK_CLOSE_ABS(atPerigee[eBoundTheta], atIp3d[eBoundTheta], 0.01_mrad);
  CHECK_CLOSE_REL(atPerigee[eBoundQOverP], atIp3d[eBoundQOverP],
                  std::numeric_limits<double>::epsilon());

  // check that we get sensible compatibility scores
  // this is a chi2-like value and should always be positive
  auto compatibility =
      ipEstimator.getVertexCompatibility(geoContext, &trackAtIP3d, refPosition)
          .value();
  BOOST_CHECK_GT(compatibility, 0);
}

BOOST_DATA_TEST_CASE(TimeAtPca, tracksWithoutIPs* vertices, t0, phi, theta, p,
                     q, vx0, vy0, vz0, vt0) {
  using Propagator = Acts::Propagator<Stepper>;
  using PropagatorOptions = Propagator::Options<>;
  using StraightPropagator = Acts::Propagator<StraightLineStepper>;

  // Set up quantities for constant B field
  auto field = std::make_shared<MagneticField>(Vector3(0, 0, 2_T));
  Stepper stepper(field);
  auto propagator = std::make_shared<Propagator>(std::move(stepper));
  Estimator::Config cfg(field, propagator);
  Estimator ipEstimator(cfg);
  Estimator::State ipState{magFieldCache()};

  // Set up quantities for B = 0
  auto zeroField = std::make_shared<MagneticField>(Vector3(0, 0, 0));
  StraightLineStepper straightLineStepper;
  auto straightLinePropagator =
      std::make_shared<StraightPropagator>(straightLineStepper);
  StraightLineEstimator::Config zeroFieldCfg(zeroField, straightLinePropagator);
  StraightLineEstimator zeroFieldIPEstimator(zeroFieldCfg);
  StraightLineEstimator::State zeroFieldIPState{magFieldCache()};

  // Vertex position and vertex object
  Vector4 vtxPos(vx0, vy0, vz0, vt0);
  Vertex vtx(vtxPos, makeVertexCovariance(), {});

  // Perigee surface at vertex position
  auto vtxPerigeeSurface =
      Surface::makeShared<PerigeeSurface>(vtxPos.head<3>());

  // Track parameter vector for a track that originates at the vertex.
  // Note that 2D and 3D PCA coincide since the track passes exactly through the
  // vertex.
  BoundVector paramVec;
  paramVec[eBoundLoc0] = 0.;
  paramVec[eBoundLoc1] = 0.;
  paramVec[eBoundTime] = t0;
  paramVec[eBoundPhi] = phi;
  paramVec[eBoundTheta] = theta;
  paramVec[eBoundQOverP] = q / p;

  BoundTrackParameters params(vtxPerigeeSurface, paramVec,
                              makeBoundParametersCovariance(),
                              ParticleHypothesis::pion());

  // Correct quantities for checking if IP estimation worked
  // Time of the track with respect to the vertex
  double corrTimeDiff = t0 - vt0;

  // Momentum direction at vertex (i.e., at 3D PCA)
  double cosPhi = std::cos(phi);
  double sinPhi = std::sin(phi);
  double sinTheta = std::sin(theta);
  Vector3 corrMomDir =
      Vector3(cosPhi * sinTheta, sinPhi * sinTheta, std::cos(theta));

  // Arbitrary reference point
  Vector3 refPoint(2_mm, -2_mm, -5_mm);

  // Perigee surface at vertex position
  auto refPerigeeSurface = Surface::makeShared<PerigeeSurface>(refPoint);

  // Set up the propagator options (they are the same with and without B field)
  PropagatorOptions pOptions(geoContext, magFieldContext);
  auto intersection =
      refPerigeeSurface
          ->intersect(geoContext, params.position(geoContext),
                      params.direction(), BoundaryTolerance::Infinite())
          .closest();
  pOptions.direction =
      Direction::fromScalarZeroAsPositive(intersection.pathLength());

  StraightPropagator::Options<> straightPOptions(geoContext, magFieldContext);
  straightPOptions.direction = pOptions.direction;

  // Propagate to the 2D PCA of the reference point in a constant B field
  auto result = propagator->propagate(params, *refPerigeeSurface, pOptions);
  BOOST_CHECK(result.ok());
  const auto& refParams = *result->endParameters;

  // Propagate to the 2D PCA of the reference point when B = 0
  auto zeroFieldResult = straightLinePropagator->propagate(
      params, *refPerigeeSurface, straightPOptions);
  BOOST_CHECK(zeroFieldResult.ok());
  const auto& zeroFieldRefParams = *zeroFieldResult->endParameters;

  BOOST_TEST_CONTEXT(
      "Check time at 2D PCA (i.e., function getImpactParameters) for helical "
      "tracks") {
    // Calculate impact parameters
    auto ipParams = ipEstimator
                        .getImpactParameters(refParams, vtx, geoContext,
                                             magFieldContext, true)
                        .value();
    // Spatial impact parameters should be 0 because the track passes through
    // the vertex
    CHECK_CLOSE_ABS(ipParams.d0, 0., 30_nm);
    CHECK_CLOSE_ABS(ipParams.z0, 0., 100_nm);
    // Time impact parameter should correspond to the time where the track
    // passes through the vertex
    CHECK_CLOSE_OR_SMALL(ipParams.deltaT.value(), std::abs(corrTimeDiff), 1e-5,
                         1e-3);
  }

  auto checkGetDistanceAndMomentum = [&vtxPos, &corrMomDir, corrTimeDiff](
                                         const auto& ipe, const auto& rParams,
                                         auto& state) {
    // Find 4D distance and momentum of the track at the vertex starting from
    // the perigee representation at the reference position
    auto distAndMom = ipe.template getDistanceAndMomentum<4>(
                             geoContext, rParams, vtxPos, state)
                          .value();

    Vector4 distVec = distAndMom.first;
    Vector3 momDir = distAndMom.second;

    // Check quantities:
    // Spatial distance should be 0 as track passes through the vertex
    double dist = distVec.head<3>().norm();
    CHECK_CLOSE_ABS(dist, 0., 30_nm);
    // Distance in time should correspond to the time of the track in a
    // coordinate system with the vertex as the origin since the track passes
    // exactly through the vertex
    CHECK_CLOSE_OR_SMALL(distVec[3], corrTimeDiff, 1e-5, 1e-4);
    // Momentum direction should correspond to the momentum direction at the
    // vertex
    CHECK_CLOSE_OR_SMALL(momDir, corrMomDir, 1e-5, 1e-4);
  };

  BOOST_TEST_CONTEXT(
      "Check time at 3D PCA (i.e., function getDistanceAndMomentum) for "
      "straight tracks") {
    checkGetDistanceAndMomentum(zeroFieldIPEstimator, zeroFieldRefParams,
                                zeroFieldIPState);
  }
  BOOST_TEST_CONTEXT(
      "Check time at 3D PCA (i.e., function getDistanceAndMomentum) for "
      "helical tracks") {
    checkGetDistanceAndMomentum(ipEstimator, refParams, ipState);
  }
}

BOOST_DATA_TEST_CASE(VertexCompatibility4D, IPs* vertices, d0, l0, vx0, vy0,
                     vz0, vt0) {
  // Set up RNG
  int seed = 31415;
  std::mt19937 gen(seed);

  // Impact point estimator
  Estimator ipEstimator = makeEstimator(2_T);

  // Vertex position
  Vector4 vtxPos(vx0, vy0, vz0, vt0);

  // Dummy coordinate system with origin at vertex
  Transform3 coordinateSystem;
  // First three columns correspond to coordinate system axes
  coordinateSystem.matrix().block<3, 3>(0, 0) = ActsSquareMatrix<3>::Identity();
  // Fourth column corresponds to origin of the coordinate system
  coordinateSystem.matrix().block<3, 1>(0, 3) = vtxPos.head<3>();

  // Dummy plane surface
  std::shared_ptr<PlaneSurface> planeSurface =
      Surface::makeShared<PlaneSurface>(coordinateSystem);

  // Create two track parameter vectors that are alike except that one is closer
  // to the vertex in time. Note that momenta don't play a role in the
  // computation and we set the angles and q/p to 0.
  // Time offsets
  double timeDiffFactor = uniformDist(gen);
  double timeDiffClose = timeDiffFactor * 0.1_ps;
  double timeDiffFar = timeDiffFactor * 0.11_ps;

  // Different random signs for the time offsets
  double sgnClose = signDist(gen) < 0 ? -1. : 1.;
  double sgnFar = signDist(gen) < 0 ? -1. : 1.;

  BoundVector paramVecClose = BoundVector::Zero();
  paramVecClose[eBoundLoc0] = d0;
  paramVecClose[eBoundLoc1] = l0;
  paramVecClose[eBoundPhi] = 0;
  paramVecClose[eBoundTheta] = std::numbers::pi / 2;
  paramVecClose[eBoundQOverP] = 0;
  paramVecClose[eBoundTime] = vt0 + sgnClose * timeDiffClose;

  BoundVector paramVecFar = paramVecClose;
  paramVecFar[eBoundTime] = vt0 + sgnFar * timeDiffFar;

  // Track whose time is similar to the vertex time
  BoundTrackParameters paramsClose(planeSurface, paramVecClose,
                                   makeBoundParametersCovariance(30_ns),
                                   ParticleHypothesis::pion());

  // Track whose time is similar to the vertex time but with a larger time
  // variance
  BoundTrackParameters paramsCloseLargerCov(
      planeSurface, paramVecClose, makeBoundParametersCovariance(31_ns),
      ParticleHypothesis::pion());

  // Track whose time differs slightly more from the vertex time
  BoundTrackParameters paramsFar(planeSurface, paramVecFar,
                                 makeBoundParametersCovariance(30_ns),
                                 ParticleHypothesis::pion());

  // Calculate the 4D vertex compatibilities of the three tracks
  double compatibilityClose =
      ipEstimator.getVertexCompatibility(geoContext, &paramsClose, vtxPos)
          .value();
  double compatibilityCloseLargerCov =
      ipEstimator
          .getVertexCompatibility(geoContext, &paramsCloseLargerCov, vtxPos)
          .value();
  double compatibilityFar =
      ipEstimator.getVertexCompatibility(geoContext, &paramsFar, vtxPos)
          .value();

  // The track who is closer in time must have a better (i.e., smaller)
  // compatibility
  BOOST_CHECK_LT(compatibilityClose, compatibilityFar);
  // The track with the larger covariance must be the most compatible
  BOOST_CHECK_LT(compatibilityCloseLargerCov, compatibilityClose);
}

// Compare calculations w/ known good values from Athena.
//
// Checks the results for a single track with the same test values as in Athena
// unit test algorithm
//
//   Tracking/TrkVertexFitter/TrkVertexFitterUtils/test/ImpactPointEstimator_test
//
BOOST_AUTO_TEST_CASE(SingleTrackDistanceParametersAthenaRegression) {
  Estimator ipEstimator = makeEstimator(1.9971546939_T);
  Estimator::State state{magFieldCache()};

  // Use same values as in Athena unit test
  Vector4 pos1(2_mm, 1_mm, -10_mm, 0_ns);
  Vector3 mom1(400_MeV, 600_MeV, 200_MeV);
  Vector3 vtxPos(1.2_mm, 0.8_mm, -7_mm);

  // Start creating some track parameters
  auto perigeeSurface =
      Surface::makeShared<PerigeeSurface>(pos1.segment<3>(ePos0));
  // Some fixed track parameter values
  auto params1 = BoundTrackParameters::create(
                     perigeeSurface, geoContext, pos1, mom1, 1_e / mom1.norm(),
                     BoundTrackParameters::CovarianceMatrix::Identity(),
                     ParticleHypothesis::pion())
                     .value();

  // Compare w/ desired result from Athena unit test
  auto distance =
      ipEstimator.calculateDistance(geoContext, params1, vtxPos, state).value();
  CHECK_CLOSE_ABS(distance, 3.10391_mm, 10_nm);

  auto res2 = ipEstimator.estimate3DImpactParameters(
      geoContext, magFieldContext, params1, vtxPos, state);
  BOOST_CHECK(res2.ok());
  BoundTrackParameters endParams = *res2;
  Vector3 surfaceCenter = endParams.referenceSurface().center(geoContext);

  BOOST_CHECK_EQUAL(surfaceCenter, vtxPos);
}

// Test the Impact3d Point estimator 2d and 3d lifetimes sign
// on a single track.

BOOST_AUTO_TEST_CASE(Lifetimes2d3d) {
  Estimator ipEstimator = makeEstimator(2_T);

  // Create a track from a decay
  BoundVector trk_par;
  trk_par[eBoundLoc0] = 200_um;
  trk_par[eBoundLoc1] = 300_um;
  trk_par[eBoundTime] = 1_ns;
  trk_par[eBoundPhi] = 45_degree;
  trk_par[eBoundTheta] = 45_degree;
  trk_par[eBoundQOverP] = 1_e / 10_GeV;

  Vector4 ip_pos{0., 0., 0., 0.};
  Vertex ip_vtx(ip_pos, makeVertexCovariance(), {});

  // Form the bound track parameters at the ip
  auto perigeeSurface = Surface::makeShared<PerigeeSurface>(ip_pos.head<3>());
  BoundTrackParameters track(perigeeSurface, trk_par,
                             makeBoundParametersCovariance(),
                             ParticleHypothesis::pion());

  Vector3 direction{0., 1., 0.};
  auto lifetimes_signs = ipEstimator.getLifetimeSignOfTrack(
      track, ip_vtx, direction, geoContext, magFieldContext);

  // Check if the result is OK
  BOOST_CHECK(lifetimes_signs.ok());

  // Check that d0 sign is positive
  BOOST_CHECK_GT((*lifetimes_signs).first, 0.);

  // Check that z0 sign is negative
  BOOST_CHECK_LT((*lifetimes_signs).second, 0.);

  // Check the 3d sign

  auto sign3d = ipEstimator.get3DLifetimeSignOfTrack(
      track, ip_vtx, direction, geoContext, magFieldContext);

  // Check result is OK
  BOOST_CHECK(sign3d.ok());

  // Check 3D sign (should be positive)
  BOOST_CHECK_GT((*sign3d), 0.);
}

// Check `.getImpactParameters`.
BOOST_DATA_TEST_CASE(SingeTrackImpactParameters, tracks* vertices, d0, l0, t0,
                     phi, theta, p, q, vx0, vy0, vz0, vt0) {
  BoundVector par;
  par[eBoundLoc0] = d0;
  par[eBoundLoc1] = l0;
  par[eBoundTime] = t0;
  par[eBoundPhi] = phi;
  par[eBoundTheta] = theta;
  par[eBoundQOverP] = q / p;
  Vector4 vtxPos;
  vtxPos[ePos0] = vx0;
  vtxPos[ePos1] = vy0;
  vtxPos[ePos2] = vz0;
  vtxPos[eTime] = vt0;

  Estimator ipEstimator = makeEstimator(1_T);
  Estimator::State state{magFieldCache()};

  // reference position and corresponding perigee surface
  Vector3 refPosition(0., 0., 0.);
  auto perigeeSurface = Surface::makeShared<PerigeeSurface>(refPosition);
  // create track and vertex
  BoundTrackParameters track(perigeeSurface, par,
                             makeBoundParametersCovariance(),
                             ParticleHypothesis::pionLike(std::abs(q)));
  Vertex myConstraint(vtxPos, makeVertexCovariance(), {});

  // check that computed impact parameters are meaningful
  ImpactParametersAndSigma output =
      ipEstimator
          .getImpactParameters(track, myConstraint, geoContext, magFieldContext)
          .value();
  BOOST_CHECK_NE(output.d0, 0.);
  BOOST_CHECK_NE(output.z0, 0.);
  // TODO what about the other struct members? can the parameter space be
  // restricted further?
}

BOOST_AUTO_TEST_SUITE_END()
