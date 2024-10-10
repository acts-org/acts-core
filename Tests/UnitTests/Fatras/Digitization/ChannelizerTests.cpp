// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Units.hpp"
#include "Acts/Surfaces/CurvilinearSurface.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "ActsFatras/Digitization/Channelizer.hpp"
#include "ActsFatras/Digitization/PlanarSurfaceDrift.hpp"
#include "ActsFatras/Digitization/PlanarSurfaceMask.hpp"

#include <numeric>

using namespace Acts::UnitLiterals;

struct Helper {
  std::shared_ptr<Acts::Surface> surface;
  Acts::BinUtility segmentation;

  Acts::GeometryContext gctx{};
  double thickness = 125_um;
  Acts::Vector3 driftDir = Acts::Vector3::Zero();

  ActsFatras::Channelizer channelizer;

  Helper() {
    surface = Acts::CurvilinearSurface(Acts::Vector3::Zero(),
                                       Acts::Vector3{0.0, 0.0, 1.0})
                  .planeSurface();

    float pitchSize = 50_um;
    float min = -200_um;
    float max = 200_um;
    int bins = static_cast<int>((max - min) / pitchSize);
    segmentation = Acts::BinUtility(bins, min, max, Acts::BinningOption::open,
                                    Acts::BinningValue::binX);
    segmentation += Acts::BinUtility(bins, min, max, Acts::BinningOption::open,
                                     Acts::BinningValue::binY);
  }

  auto channelize(const Acts::Vector3 &pos3, const Acts::Vector3 &dir3) const {
    Acts::Vector4 pos4 = Acts::Vector4::Zero();
    pos4.segment<3>(Acts::ePos0) = pos3;
    Acts::Vector4 mom4 = Acts::Vector4::Zero();
    mom4.segment<3>(Acts::eMom0) = dir3;
    ActsFatras::Hit hit({}, {}, pos4, mom4, mom4);
    auto res = channelizer.channelize(hit, *surface, gctx, driftDir,
                                      segmentation, thickness);
    BOOST_REQUIRE(res.ok());
    return *res;
  }
};

BOOST_AUTO_TEST_CASE(test_upright_particle) {
  Helper helper;

  Acts::Vector3 pos3 = Acts::Vector3{10_um, 10_um, 0.0};
  Acts::Vector3 dir3 = Acts::Vector3{0.0, 0.0, helper.thickness}.normalized();

  auto segments = helper.channelize(pos3, dir3);

  BOOST_CHECK_EQUAL(segments.size(), 1);
  BOOST_CHECK_CLOSE(segments[0].activation, helper.thickness, 1.e-8);
}

BOOST_AUTO_TEST_CASE(test_tilted_particle) {
  Helper helper;

  const double disp = 10_um;

  Acts::Vector3 hitPosition = Acts::Vector3{10_um, 10_um, 0.0};
  Acts::Vector3 hitDirection =
      Acts::Vector3({disp, 0.0, helper.thickness}).normalized();

  auto segments = helper.channelize(hitPosition, hitDirection);

  std::cout << "Segments:\n";
  for (const auto &seg : segments) {
    std::cout << " - (" << seg.bin[0] << ", " << seg.bin[1]
              << "), activation: " << seg.activation << "\n";
  }

  BOOST_CHECK_EQUAL(segments.size(), 1);
  BOOST_CHECK_CLOSE(segments[0].activation, std::hypot(disp, helper.thickness),
                    1.e-8);
}

BOOST_AUTO_TEST_CASE(test_more_tilted_particle) {
  Helper helper;

  const double disp = 50_um;

  Acts::Vector3 hitPosition = Acts::Vector3{10_um, 10_um, 0.0};
  Acts::Vector3 hitDirection =
      Acts::Vector3{disp, 0.0, helper.thickness}.normalized();

  auto segments = helper.channelize(hitPosition, hitDirection);

  BOOST_CHECK_EQUAL(segments.size(), 2);
  auto sum =
      std::accumulate(segments.begin(), segments.end(), 0.0,
                      [](double s, auto seg) { return s + seg.activation; });
  BOOST_CHECK_CLOSE(sum, std::hypot(disp, helper.thickness), 1.e-8);
}

// This should go directly up on the segment border
BOOST_AUTO_TEST_CASE(test_pathological_upright_particle) {
  Helper helper;

  Acts::Vector3 hitPosition = Acts::Vector3{0.0, 10_um, 0.0};
  Acts::Vector3 hitDirection =
      Acts::Vector3{0.0, 0.0, helper.thickness}.normalized();

  auto segments = helper.channelize(hitPosition, hitDirection);

  BOOST_CHECK_EQUAL(segments.size(), 1);
  BOOST_CHECK_CLOSE(segments[0].activation, helper.thickness, 1.e-8);
}

// This should go directly up on the segment border
// TODO why does this does not activate both cells with half of the path???
BOOST_AUTO_TEST_CASE(test_pathological_tilted_particle) {
  Helper helper;

  double disp = 2.0_um;

  Acts::Vector3 hitPosition = Acts::Vector3{-0.5 * disp, 10_um, 0.0};
  Acts::Vector3 hitDirection =
      Acts::Vector3{disp, 0.0, helper.thickness}.normalized();

  auto segments = helper.channelize(hitPosition, hitDirection);

  std::cout << "Segments:\n";
  for (const auto &seg : segments) {
    std::cout << " - (" << seg.bin[0] << ", " << seg.bin[1]
              << "), activation: " << seg.activation << "\n";
  }

  BOOST_CHECK_EQUAL(segments.size(), 2);
  auto sum =
      std::accumulate(segments.begin(), segments.end(), 0.0,
                      [](double s, auto seg) { return s + seg.activation; });
  BOOST_CHECK_CLOSE(sum, std::hypot(disp, helper.thickness), 1.e-8);
}
