// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Acts include(s)
#include "Acts/Definitions/Units.hpp"
#include "Acts/MagneticField/ConstantBField.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/MagneticField/MagneticFieldProvider.hpp"
#include "Acts/MagneticField/SolenoidBField.hpp"

// Covfie Plugin include(s)
#include "Acts/Plugins/Covfie/FieldConversion.hpp"

// System include(s)
#include <array>
#include <cmath>
#include <sstream>
#include <vector>

// Boost include(s)
#include <boost/test/unit_test.hpp>

using namespace Acts::UnitLiterals;

template <typename view_t, typename iterator_t>
void checkMagneticFieldEqual(const Acts::MagneticFieldProvider& fieldProvider,
                             Acts::MagneticFieldProvider::Cache& cache,
                             view_t view, iterator_t points,
                             float error_margin_half_width) {
  for (auto point : points) {
    auto x = point[0], y = point[1], z = point[2];

    auto lookupResult = fieldProvider.getField(Acts::Vector3{x, y, z}, cache);
    if (!lookupResult.ok()) {
      throw std::runtime_error{"Field lookup failure"};
    }
    auto actsValueX = (*lookupResult)[0], actsValueY = (*lookupResult)[1],
         actsValueZ = (*lookupResult)[2];

    auto covfieValues = view.at(x, y, z);
    auto covfieValueX = covfieValues[0], covfieValueY = covfieValues[1],
         covfieValueZ = covfieValues[2];

    auto isEqual =
        std::abs(covfieValueX - actsValueX) <= error_margin_half_width &&
        std::abs(covfieValueY - actsValueY) <= error_margin_half_width &&
        std::abs(covfieValueZ - actsValueZ) <= error_margin_half_width;

    std::stringstream ss;
    ss << "Fields are not equal at position (" << x << ", " << y << ", " << z
       << "). Acts: (" << actsValueX << ", " << actsValueY << ", " << actsValueZ
       << "), Covfie: (" << covfieValueX << ", " << covfieValueY << ", "
       << covfieValueZ << ")" << std::endl;

    BOOST_CHECK_MESSAGE(isEqual, ss.str());
  }
}

BOOST_AUTO_TEST_SUITE(CovfiePlugin)

BOOST_AUTO_TEST_CASE(InterpolatedMagneticField1) {
  auto localToGlobalBin_xyz = [](std::array<std::size_t, 3> binsXYZ,
                                 std::array<std::size_t, 3> nBinsXYZ) {
    return (binsXYZ.at(0) * (nBinsXYZ.at(1) * nBinsXYZ.at(2)) +
            binsXYZ.at(1) * nBinsXYZ.at(2) + binsXYZ.at(2));
  };

  std::vector<double> xPos = {0., 1., 2., 3.};
  std::vector<double> yPos = {0., 1., 2., 3.};
  std::vector<double> zPos = {0., 1., 2., 3.};

  std::vector<Acts::Vector3> bField_xyz;
  for (int i = 0; i < 64; i++) {
    bField_xyz.push_back(Acts::Vector3(i, i, i));
  }

  Acts::MagneticFieldContext fieldContext;
  auto actsField = Acts::fieldMapXYZ(localToGlobalBin_xyz, xPos, yPos, zPos,
                                     bField_xyz, 1, 1, false);
  Acts::MagneticFieldProvider::Cache cache = actsField.makeCache(fieldContext);

  Acts::CovfiePlugin::InterpolatedField field =
      Acts::CovfiePlugin::covfieField(actsField);
  typename Acts::CovfiePlugin::InterpolatedField::view_t view(field);

  std::array<std::array<float, 3>, 14> points = {{
      {0.f, 0.f, 0.f},
      {1.f, 1.f, 1.f},
      {2.f, 2.f, 2.f},
      {2.9f, 2.9f, 2.9f},
      {1.2f, 2.5f, 0.8f},
      {0.7f, 1.9f, 2.3f},
      {2.1f, 0.3f, 1.5f},
      {0.4f, 2.8f, 2.9f},
      {1.6f, 1.2f, 0.5f},
      {2.3f, 0.6f, 2.2f},
      {1.1f, 2.7f, 1.3f},
      {0.9f, 1.4f, 2.7f},
      {2.4f, 1.8f, 0.9f},
      {0.6f, 2.2f, 2.1f},
  }};

  checkMagneticFieldEqual(actsField, cache, view, points, 0.0001);
}

BOOST_AUTO_TEST_CASE(InterpolatedMagneticField2) {
  auto localToGlobalBin_xyz = [](std::array<std::size_t, 3> binsXYZ,
                                 std::array<std::size_t, 3> nBinsXYZ) {
    return (binsXYZ.at(0) * (nBinsXYZ.at(1) * nBinsXYZ.at(2)) +
            binsXYZ.at(1) * nBinsXYZ.at(2) + binsXYZ.at(2));
  };

  std::vector<double> xPos = {8., 12., 16., 20.};
  std::vector<double> yPos = {8., 12., 16., 20.};
  std::vector<double> zPos = {8., 12., 16., 20.};

  std::vector<Acts::Vector3> bField_xyz;
  for (int i = 0; i < 64; i++) {
    bField_xyz.push_back(Acts::Vector3(i, i * i * 0.01, i));
  }

  Acts::MagneticFieldContext fieldContext;
  auto actsField = Acts::fieldMapXYZ(localToGlobalBin_xyz, xPos, yPos, zPos,
                                     bField_xyz, 1, 1, false);
  Acts::MagneticFieldProvider::Cache cache = actsField.makeCache(fieldContext);

  Acts::CovfiePlugin::InterpolatedField field =
      Acts::CovfiePlugin::covfieField(actsField);
  typename Acts::CovfiePlugin::InterpolatedField::view_t view(field);

  std::array<std::array<float, 3>, 14> points = {{
      {8.f, 8.f, 8.f},
      {12.f, 12.f, 12.f},
      {16.f, 16.f, 16.f},
      {19.9, 19.9, 19.9},
      {8.1f, 10.2f, 12.3f},
      {9.4f, 11.5f, 13.6f},
      {10.7f, 12.8f, 14.9f},
      {11.0f, 13.1f, 15.2f},
      {12.3f, 14.4f, 16.5f},
      {13.6f, 15.7f, 17.8f},
      {14.9f, 16.0f, 18.1f},
      {16.2f, 17.3f, 19.4f},
      {17.5f, 18.6f, 19.7f},
      {18.8f, 19.9f, 14.0f},
  }};

  checkMagneticFieldEqual(actsField, cache, view, points, 0.0001f);
}

BOOST_AUTO_TEST_CASE(ConstantMagneticField1) {
  Acts::ConstantBField actsField(Acts::Vector3{1.3f, 2.5f, 2.f});
  Acts::MagneticFieldContext ctx;
  Acts::MagneticFieldProvider::Cache cache = actsField.makeCache(ctx);

  Acts::CovfiePlugin::ConstantField field =
      Acts::CovfiePlugin::covfieField(actsField);
  typename Acts::CovfiePlugin::ConstantField::view_t view(field);

  std::array<std::array<float, 3>, 13> points = {{
      {8.f, 8.f, 8.f},
      {12.f, 12.f, 12.f},
      {16.f, 16.f, 16.f},
      {8.1f, 10.2f, 12.3f},
      {9.4f, 11.5f, 13.6f},
      {10.7f, 12.8f, 14.9f},
      {11.0f, 13.1f, 15.2f},
      {12.3f, 14.4f, 16.5f},
      {13.6f, 15.7f, 17.8f},
      {14.9f, 16.0f, 18.1f},
      {16.2f, 17.3f, 19.4f},
      {17.5f, 18.6f, 19.7f},
      {18.8f, 19.9f, 14.0f},
  }};

  checkMagneticFieldEqual(actsField, cache, view, points, 0.0001f);
}

BOOST_AUTO_TEST_CASE(SolenoidBField1) {
  Acts::SolenoidBField::Config cfg{};
  cfg.length = 5.8_m;
  cfg.radius = (2.56 + 2.46) * 0.5 * 0.5_m;
  cfg.nCoils = 1154;
  cfg.bMagCenter = 2_T;
  Acts::SolenoidBField actsField(cfg);
  Acts::MagneticFieldContext ctx;
  Acts::MagneticFieldProvider::Cache cache = actsField.makeCache(ctx);

  Acts::CovfiePlugin::InterpolatedField field = Acts::CovfiePlugin::covfieField(
      actsField, cache, {21UL, 21UL, 21UL}, {0., 0., 0.}, {20., 20., 20.});
  typename Acts::CovfiePlugin::InterpolatedField::view_t view(field);

  std::array<std::array<float, 3>, 13> points = {{
      {8.f, 8.f, 8.f},
      {12.f, 12.f, 12.f},
      {16.f, 16.f, 16.f},
      {8.1f, 10.2f, 12.3f},
      {9.4f, 11.5f, 13.6f},
      {10.7f, 12.8f, 14.9f},
      {11.0f, 13.1f, 15.2f},
      {12.3f, 14.4f, 16.5f},
      {13.6f, 15.7f, 17.8f},
      {14.9f, 16.0f, 18.1f},
      {16.2f, 17.3f, 19.4f},
      {17.5f, 18.6f, 19.7f},
      {18.8f, 19.9f, 14.0f},
  }};

  checkMagneticFieldEqual(actsField, cache, view, points, 0.0001);
}

BOOST_AUTO_TEST_SUITE_END()
