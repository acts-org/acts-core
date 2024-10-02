// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Seeding/HoughTransformUtils.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <array>
#include <memory>
#include <vector>

namespace Acts::Test {

using Scalar = Acts::ActsScalar;
auto logger = Acts::getDefaultLogger("UnitTests", Acts::Logging::VERBOSE);

struct DriftCircle {
  Scalar y{0.};
  Scalar z{0.};
  Scalar rDrift{0.};
  Scalar rDriftError{0.};

  DriftCircle(const Scalar _y, const Scalar _z, const Scalar _r,
              const Scalar _rUncert)
      : y{_y}, z{_z}, rDrift{_r}, rDriftError{_rUncert} {}
};

BOOST_AUTO_TEST_CASE(hough_transform_seeder) {
  Logging::ScopedFailureThreshold ft{Logging::FATAL};

  // we are using the slope on yz plane with the y coordinate (hardcoded from
  // the csv MuonSimHit data)
  std::vector<std::pair<double, double>> simHits = {
      {-0.0401472 / 0.994974, -422.612}};

  // Define the drift Circles
  constexpr double uncert{0.3};
  std::array<DriftCircle, 6> driftCircles{
      DriftCircle{-427.981, -225.541, 14.5202, uncert},
      DriftCircle{-412.964, -199.53, 1.66237, uncert},
      DriftCircle{-427.981, -173.519, 12.3176, uncert},
      DriftCircle{-427.981, 173.519, 1.5412, uncert},
      DriftCircle{-442.999, 199.53, 12.3937, uncert},
      DriftCircle{-427.981, 225.541, 3.77967, uncert}

  };

  // configure the binning of the hough plane
  Acts::HoughTransformUtils::HoughPlaneConfig planeCfg;
  planeCfg.nBinsX = 1000;
  planeCfg.nBinsY = 1000;

  // instantiate the peak finder
  Acts::HoughTransformUtils::PeakFinders::IslandsAroundMaxConfig peakFinderCfg;
  peakFinderCfg.fractionCutoff = 0.7;
  peakFinderCfg.threshold = 3.;
  peakFinderCfg.minSpacingBetweenPeaks = {0., 30.};

  // and map the hough plane to parameter ranges.
  // The first coordinate is tan(theta), the second is z0 in mm
  Acts::HoughTransformUtils::HoughAxisRanges axisRanges{-3., 3., -2000., 2000.};

  // create the functions parametrising the hough space lines for drift circles.
  // Note that there are two solutions for each drift circle and angle

  // left solution
  auto houghParamFromDCleft = [](double tanTheta, const DriftCircle& DC) {
    return DC.y - tanTheta * DC.z - DC.rDrift / std::cos(std::atan(tanTheta));
  };
  // right solution
  auto houghParamFromDCright = [](double tanTheta, const DriftCircle& DC) {
    return DC.y - tanTheta * DC.z + DC.rDrift / std::cos(std::atan(tanTheta));
  };

  // create the function parametrising the drift radius uncertainty
  auto houghWidthFromDC = [](double, const DriftCircle& DC) {
    return std::min(DC.rDriftError * 3.,
                    1.0);  // scale reported errors up to at least 1mm or 3
                           // times the reported error as drift circle calib not
                           // fully reliable at this stage
  };

  // instantiate the hough plane
  Acts::HoughTransformUtils::HoughPlane<Acts::GeometryIdentifier::Value>
      houghPlane(planeCfg);

  // also insantiate the peak finder
  Acts::HoughTransformUtils::PeakFinders::IslandsAroundMax<
      Acts::GeometryIdentifier::Value>
      peakFinder(peakFinderCfg);

  // loop over the true hits
  for (auto& sh : simHits) {
    houghPlane.reset();

    for (std::size_t k = 0; k < driftCircles.size(); ++k) {
      auto dc = driftCircles[k];

      houghPlane.fill<DriftCircle>(dc, axisRanges, houghParamFromDCleft,
                                   houghWidthFromDC, k);
      houghPlane.fill<DriftCircle>(dc, axisRanges, houghParamFromDCright,
                                   houghWidthFromDC, k);
    }

    // now get the peaks
    auto maxima = peakFinder.findPeaks(houghPlane, axisRanges);
    for (auto& max : maxima) {
      // check the Hough Transforms results
      BOOST_CHECK_CLOSE(max.x, sh.first, 4.);
      BOOST_CHECK_CLOSE(max.y, sh.second, 0.2);
    }
  }
}

}  // namespace Acts::Test
