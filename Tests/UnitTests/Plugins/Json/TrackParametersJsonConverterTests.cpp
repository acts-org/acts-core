// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>

#include "Acts/EventData/ParticleHypothesis.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Plugins/Json/TrackParametersJsonConverter.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"

#include <memory>

#include <nlohmann/json.hpp>

BOOST_AUTO_TEST_SUITE(TrackParametersJsonIO)

BOOST_AUTO_TEST_CASE(TrackParametersJsonIO) {
  Acts::GeometryContext gctx;

  // Track parameters
  Acts::Vector4 position(1., 2., 3., 4.);
  double phi = 0.1;
  double theta = 0.2;
  double qOverP = 3.0;
  Acts::ParticleHypothesis particle = Acts::ParticleHypothesis::electron();
  Acts::FreeMatrix freeCov = Acts::FreeMatrix::Identity();
  Acts::BoundMatrix boundCov = Acts::BoundMatrix::Identity();

  auto surface = Acts::Surface::makeShared<Acts::PlaneSurface>(
      Acts::Transform3::Identity(),
      std::make_shared<Acts::RectangleBounds>(10., 10.));
  surface->assignGeometryId(Acts::GeometryIdentifier(1u));

  // Free track parameters conversion
  Acts::FreeTrackParameters ftp(position, phi, theta, qOverP, freeCov,
                                particle);

  nlohmann::json ftpJson = ftp;

  Acts::FreeTrackParameters ftpRead = ftpJson;

  BOOST_CHECK_EQUAL(ftp.position(), ftpRead.position());
  BOOST_CHECK_EQUAL(ftp.direction(), ftpRead.direction());
  BOOST_CHECK_EQUAL(ftp.qOverP(), ftpRead.qOverP());
  BOOST_CHECK_EQUAL(ftp.covariance().value(), ftpRead.covariance().value());
  BOOST_CHECK_EQUAL(ftp.particleHypothesis(), ftpRead.particleHypothesis());

  // Curvilinear track parameters conversion
  Acts::CurvilinearTrackParameters ctp(position, phi, theta, qOverP, boundCov,
                                       particle);

  nlohmann::json ctpJson = ctp;

  Acts::CurvilinearTrackParameters ctpRead = ctpJson;

  BOOST_CHECK_EQUAL(ctp.position(), ctpRead.position());
  BOOST_CHECK_EQUAL(ctp.direction(), ctpRead.direction());
  BOOST_CHECK_EQUAL(ctp.qOverP(), ctpRead.qOverP());
  BOOST_CHECK_EQUAL(ctp.covariance().value(), ctpRead.covariance().value());
  BOOST_CHECK_EQUAL(ctp.particleHypothesis(), ctpRead.particleHypothesis());

  BOOST_CHECK(ctp.referenceSurface().transform(gctx).isApprox(
      ctpRead.referenceSurface().transform(gctx)));
  BOOST_CHECK_EQUAL(ctp.referenceSurface().geometryId(),
                    ctpRead.referenceSurface().geometryId());
  BOOST_CHECK_EQUAL(ctp.referenceSurface().bounds(),
                    ctpRead.referenceSurface().bounds());

  // Bound track parameters conversion
  Acts::BoundVector boundPosition{1., 2., 3., 4., 5., 6.};
  Acts::BoundTrackParameters btp(surface, boundPosition, boundCov, particle);

  nlohmann::json btpJson = btp;

  Acts::BoundTrackParameters btpRead = btpJson;

  BOOST_CHECK_EQUAL(btp.position(gctx), btpRead.position(gctx));
  BOOST_CHECK_EQUAL(btp.direction(), btpRead.direction());
  BOOST_CHECK_EQUAL(btp.qOverP(), btpRead.qOverP());
  BOOST_CHECK_EQUAL(btp.covariance().value(), btpRead.covariance().value());
  BOOST_CHECK_EQUAL(btp.particleHypothesis(), btpRead.particleHypothesis());

  BOOST_CHECK(btp.referenceSurface().transform(gctx).isApprox(
      btpRead.referenceSurface().transform(gctx)));
  BOOST_CHECK_EQUAL(btp.referenceSurface().geometryId(),
                    btpRead.referenceSurface().geometryId());
  BOOST_CHECK_EQUAL(btp.referenceSurface().bounds(),
                    btpRead.referenceSurface().bounds());
}

BOOST_AUTO_TEST_SUITE_END()
