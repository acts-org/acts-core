// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Geometry/CylinderVolumeBuilder.hpp"

#include <utility>

namespace bdata = boost::unit_test::data;

namespace Acts::Test {

/// Unit test for testing the wraps() function of the CylinderVolumeBuilder
BOOST_DATA_TEST_CASE(
    CylinderVolumeBuilder_wraps,
    bdata::random((bdata::engine = std::mt19937(), bdata::seed = 1,
                   bdata::distribution =
                       std::uniform_real_distribution<double>(-11., -15.))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 2,
                       bdata::distribution =
                           std::uniform_real_distribution<double>(11., 15.))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 3,
                       bdata::distribution =
                           std::uniform_real_distribution<double>(-10., 10.))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 4,
                       bdata::distribution =
                           std::uniform_real_distribution<double>(0., 4.))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 5,
                       bdata::distribution =
                           std::uniform_real_distribution<double>(11., 15.))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 6,
                       bdata::distribution =
                           std::uniform_real_distribution<double>(10., 15.))) ^
        bdata::xrange(100),
    left, right, central, inner, outer, length, index) {
  (void)index;
  // inner volume
  VolumeConfig innerConfig;
  innerConfig.rMin = 0.;
  innerConfig.rMax = 10.;
  innerConfig.zMin = -10.;
  innerConfig.zMax = 10.;

  // volume to the left of the inner volume
  VolumeConfig outerConfig1;
  outerConfig1.rMin = inner;
  outerConfig1.rMax = inner + 5.;
  outerConfig1.zMin = left - 5.;
  outerConfig1.zMax = left;

  // volume to the right of the inner volume
  VolumeConfig outerConfig2;
  outerConfig2.rMin = inner;
  outerConfig2.rMax = inner + 5.;
  outerConfig2.zMin = right;
  outerConfig2.zMax = right + 5.;

  // volume around the inner volume
  VolumeConfig outerConfig3;
  outerConfig3.rMin = outer;
  outerConfig3.rMax = outer + 5.;
  outerConfig3.zMin = central - 5.;
  outerConfig3.zMax = central + 5.;

  // volume inside the inner volume
  VolumeConfig outerConfig4;
  outerConfig4.rMin = inner;
  outerConfig4.rMax = inner + 5.;
  outerConfig4.zMin = central - 5.;
  outerConfig4.zMax = central + 5.;

  // volume around the inner volume config
  VolumeConfig outerConfig5;
  outerConfig5.rMin = outer;
  outerConfig5.rMax = outer + 5.;
  outerConfig5.zMin = -length;
  outerConfig5.zMax = length;

  // volume around inner volume with same z boundaries
  VolumeConfig outerConfig6;
  outerConfig6.rMin = outer;
  outerConfig6.rMax = outer + 5.;
  outerConfig6.zMin = -10.;
  outerConfig6.zMax = 10.;

  // check if first volume wraps around the inner volume (wrapping in z)
  BOOST_CHECK(outerConfig1.wraps(innerConfig));
  // check if second volume wraps around the inner volume (wrapping in z)
  BOOST_CHECK(outerConfig2.wraps(innerConfig));
  // check if third volume wraps around the inner volume (wrapping in r)
  BOOST_CHECK(outerConfig3.wraps(innerConfig));
  // check if volume at inside the inner volume can not be wrapped
  BOOST_CHECK(!outerConfig4.wraps(innerConfig));
  // check if outside volume can not be wrapped around inside volume
  BOOST_CHECK(!innerConfig.wraps(outerConfig3));
  // check if outside volume contains inside volume
  BOOST_CHECK(outerConfig5.wraps(innerConfig));
  // check if inside volume is not contained by outside volume
  BOOST_CHECK(!innerConfig.wraps(outerConfig5));
  // check if outside volume wraps around the inside volume
  BOOST_CHECK(outerConfig6.wraps(innerConfig));
}

/// Unit test for testing the contains(), containsInR() and containsInZ()
/// function of the CylinderVolumeBuilder
BOOST_DATA_TEST_CASE(
    CylinderVolumeBuilder_containes,
    bdata::random((bdata::engine = std::mt19937(), bdata::seed = 1,
                   bdata::distribution =
                       std::uniform_real_distribution<double>(-11., -15.))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 2,
                       bdata::distribution =
                           std::uniform_real_distribution<double>(11., 15.))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 3,
                       bdata::distribution =
                           std::uniform_real_distribution<double>(-10., 10.))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 4,
                       bdata::distribution =
                           std::uniform_real_distribution<double>(0., 4.))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 5,
                       bdata::distribution =
                           std::uniform_real_distribution<double>(10., 15.))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 6,
                       bdata::distribution =
                           std::uniform_real_distribution<double>(10., 15.))) ^
        bdata::xrange(100),
    left, right, central, inner, outer, length, index) {
  (void)index;
  // inner volume
  VolumeConfig innerConfig;
  innerConfig.rMin = 0.;
  innerConfig.rMax = 10.;
  innerConfig.zMin = -10.;
  innerConfig.zMax = 10.;

  // volume to the left of the inner volume
  VolumeConfig outerConfig1;
  outerConfig1.rMin = inner;
  outerConfig1.rMax = inner + 5.;
  outerConfig1.zMin = left - 5.;
  outerConfig1.zMax = left;

  // volume to the right of the inner volume
  VolumeConfig outerConfig2;
  outerConfig2.rMin = inner;
  outerConfig2.rMax = inner + 5.;
  outerConfig2.zMin = right;
  outerConfig2.zMax = right + 5.;

  // volume around the inner volume in r
  VolumeConfig outerConfig3;
  outerConfig3.rMin = outer;
  outerConfig3.rMax = outer + 5.;
  outerConfig3.zMin = central - 5.;
  outerConfig3.zMax = central + 5.;

  // volume inside the inner volume
  VolumeConfig outerConfig4;
  outerConfig4.rMin = inner;
  outerConfig4.rMax = inner + 5.;
  outerConfig4.zMin = central - 5.;
  outerConfig4.zMax = central + 5.;

  // volume around the inner volume config
  VolumeConfig outerConfig5;
  outerConfig5.rMin = outer;
  outerConfig5.rMax = outer + 5.;
  outerConfig5.zMin = -length;
  outerConfig5.zMax = length;

  // volume around inner volume with same z boundaries
  VolumeConfig outerConfig6;
  outerConfig6.rMin = outer;
  outerConfig6.rMax = outer + 5.;
  outerConfig6.zMin = -10.;
  outerConfig6.zMax = 10.;

  // volume inside the inner volume config in z
  VolumeConfig innerConfig1;
  innerConfig1.rMin = outer;
  innerConfig1.rMax = outer + 5.;
  innerConfig1.zMin = inner - 5.;
  innerConfig1.zMax = inner + 5.;

  // check if first volume wraps around the inner volume (wrapping in z)
  BOOST_CHECK(!outerConfig1.contains(innerConfig));
  // check if second volume wraps around the inner volume (wrapping in z)
  BOOST_CHECK(!outerConfig2.contains(innerConfig));
  // check if volume at inside the inner volume can not be wrapped
  BOOST_CHECK(!outerConfig4.contains(innerConfig));
  // check if outside volume can not be wrapped around inside volume
  BOOST_CHECK(!innerConfig.contains(outerConfig3));
  // check if outside volume contains inside volume
  BOOST_CHECK(outerConfig5.contains(innerConfig));
  // check if inside volume is not contained by outside volume
  BOOST_CHECK(!innerConfig.contains(outerConfig5));
  // check if inside volume is not contained by outside volume
  BOOST_CHECK(!outerConfig6.contains(innerConfig));

  // containment checks in r and z for volumes which either contain in r or z
  BOOST_CHECK(innerConfig.containsInZ(innerConfig1));
  BOOST_CHECK(!innerConfig.containsInR(innerConfig1));
  BOOST_CHECK(innerConfig1.containsInR(innerConfig));
  BOOST_CHECK(!innerConfig1.containsInZ(innerConfig));
}

/// Unit test for testing the coverlapsInR()
/// function of the CylinderVolumeBuilder
BOOST_DATA_TEST_CASE(
    CylinderVolumeBuilder_overlapsInR,
    bdata::random((
        bdata::engine = std::mt19937(), bdata::seed = 1,
        bdata::distribution = std::uniform_real_distribution<double>(0., 4.))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 2,
                       bdata::distribution =
                           std::uniform_real_distribution<double>(11., 15.))) ^
        bdata::xrange(100),
    inner, outer, index) {
  (void)index;
  // reference volume
  VolumeConfig Config0;
  Config0.rMin = 5.;
  Config0.rMax = 10.;
  Config0.zMin = -10.;
  Config0.zMax = 10.;
  Config0.present = true;

  // volume inside volume0
  VolumeConfig Config1;
  Config1.rMin = 0.;
  Config1.rMax = inner;
  Config1.zMin = -10.;
  Config1.zMax = 10.;

  // volume outside volume0
  VolumeConfig Config2;
  Config2.rMin = outer;
  Config2.rMax = outer + 5.;
  Config2.zMin = -10.;
  Config2.zMax = 10.;

  // volume overlapping with rMin
  VolumeConfig Config3;
  Config3.rMin = inner + 5;
  Config3.rMax = outer + 5.;
  Config3.zMin = -10.;
  Config3.zMax = 10.;

  // volume overlapping with rMax
  VolumeConfig Config4;
  Config4.rMin = inner;
  Config4.rMax = inner + 5.;
  Config4.zMin = -10.;
  Config4.zMax = 10.;

  // volume overlapping with rMin and rMax
  VolumeConfig Config5;
  Config5.rMin = 5.;
  Config5.rMax = inner + 5.;
  Config5.zMin = -10.;
  Config5.zMax = 10.;

  // volume does not overlap with volume completely inside
  BOOST_CHECK(!Config0.overlapsInR(Config1));
  // volume does not overlap with volume completely outside
  BOOST_CHECK(!Config0.overlapsInR(Config2));
  // volume overlaps with rMin
  BOOST_CHECK(Config0.overlapsInR(Config3));
  // volume overlaps with rMax
  BOOST_CHECK(Config0.overlapsInR(Config4));
  // volume overlaps with rMin and rMax
  BOOST_CHECK(Config0.overlapsInR(Config5));
}

/// Unit test for testing the coverlapsInZ()
/// function of the CylinderVolumeBuilder
BOOST_DATA_TEST_CASE(
    CylinderVolumeBuilder_overlapsInZ,
    bdata::random((bdata::engine = std::mt19937(), bdata::seed = 1,
                   bdata::distribution =
                       std::uniform_real_distribution<double>(-11., -15.))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 2,
                       bdata::distribution =
                           std::uniform_real_distribution<double>(11., 15.))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 3,
                       bdata::distribution =
                           std::uniform_real_distribution<double>(0., 4.))) ^
        bdata::xrange(100),
    left, right, inner, index) {
  (void)index;
  // inner volume
  VolumeConfig Config0;
  Config0.rMin = 0.;
  Config0.rMax = 10.;
  Config0.zMin = -10.;
  Config0.zMax = 10.;
  Config0.present = true;

  // volume to the left of volume0
  VolumeConfig Config1;
  Config1.rMin = 10.;
  Config1.rMax = 20.;
  Config1.zMin = left - 5.;
  Config1.zMax = left;

  // volume to the right of volume0
  VolumeConfig Config2;
  Config2.rMin = 10.;
  Config2.rMax = 20.;
  Config2.zMin = right;
  Config2.zMax = right + 5.;

  // volume around volume0 with same z boundaries
  VolumeConfig Config3;
  Config3.rMin = 10.;
  Config3.rMax = 20.;
  Config3.zMin = -10.;
  Config3.zMax = 10.;

  // volume inside volume0 config in z
  VolumeConfig Config4;
  Config4.rMin = 10.;
  Config4.rMax = 20.;
  Config4.zMin = inner - 5.;
  Config4.zMax = inner + 5.;

  // volume around volume0 config in z
  VolumeConfig Config5;
  Config5.rMin = 10.;
  Config5.rMax = 20.;
  Config5.zMin = left;
  Config5.zMax = right;

  // volume overlapping on the left
  VolumeConfig Config6;
  Config6.rMin = 10.;
  Config6.rMax = 20.;
  Config6.zMin = left;
  Config6.zMax = left + 10.;

  // volume overlapping on the right
  VolumeConfig Config7;
  Config7.rMin = 10.;
  Config7.rMax = 20.;
  Config7.zMin = right - 10.;
  Config7.zMax = right;

  // volume to the right and left do not overlap
  BOOST_CHECK(!Config0.overlapsInZ(Config1));
  BOOST_CHECK(!Config0.overlapsInZ(Config2));
  // volume with same boundaries overlaps
  BOOST_CHECK(Config0.overlapsInZ(Config3));
  // inside volume overlaps
  BOOST_CHECK(Config0.overlapsInZ(Config4));
  // volume around overlaps
  BOOST_CHECK(Config0.overlapsInZ(Config5));
  // volume overlaps on the sides
  BOOST_CHECK(Config0.overlapsInZ(Config6));
  BOOST_CHECK(Config0.overlapsInZ(Config7));
}

}  // namespace Acts::Test
