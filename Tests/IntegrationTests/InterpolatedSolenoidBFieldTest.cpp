// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Units.hpp"
#include "Acts/MagneticField/BFieldMapUtils.hpp"
#include "Acts/MagneticField/InterpolatedBFieldMap.hpp"
#include "Acts/MagneticField/SolenoidBField.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/Axis.hpp"
#include "Acts/Utilities/Grid.hpp"

#include <cmath>
#include <fstream>
#include <iostream>
#include <numbers>
#include <random>
#include <type_traits>

using namespace Acts::UnitLiterals;

namespace bdata = boost::unit_test::data;

namespace Acts::IntegrationTest {

const double L = 5.8_m;
const double R = (2.56 + 2.46) * 0.5 * 0.5_m;
const std::size_t nCoils = 1154;
const double bMagCenter = 2_T;
const std::size_t nBinsR = 150;
const std::size_t nBinsZ = 200;

auto makeFieldMap(const SolenoidBField& field) {
  std::ofstream ostr("solenoidmap.csv");
  ostr << "i;j;r;z;B_r;B_z" << std::endl;

  double rMin = 0;
  double rMax = R * 2.;
  double zMin = 2 * (-L / 2.);
  double zMax = 2 * (L / 2.);

  std::cout << "rMin = " << rMin << std::endl;
  std::cout << "rMax = " << rMax << std::endl;
  std::cout << "zMin = " << zMin << std::endl;
  std::cout << "zMax = " << zMax << std::endl;

  auto map =
      solenoidFieldMap({rMin, rMax}, {zMin, zMax}, {nBinsR, nBinsZ}, field);
  // I know this is the correct grid type
  const auto& grid = map.getGrid();
  using Grid_t = std::decay_t<decltype(grid)>;
  using index_t = Grid_t::index_t;
  using point_t = Grid_t::point_t;

  for (std::size_t i = 0; i <= nBinsR + 1; i++) {
    for (std::size_t j = 0; j <= nBinsZ + 1; j++) {
      // std::cout << "(i,j) = " << i << "," << j << std::endl;
      index_t index({i, j});
      if (i == 0 || j == 0 || i == nBinsR + 1 || j == nBinsZ + 1) {
        // under or overflow bin
      } else {
        point_t lowerLeft = grid.lowerLeftBinEdge(index);
        Vector2 B = grid.atLocalBins(index);
        ostr << i << ";" << j << ";" << lowerLeft[0] << ";" << lowerLeft[1];
        ostr << ";" << B[0] << ";" << B[1] << std::endl;
      }
    }
  }

  return map;
}

Acts::SolenoidBField bSolenoidField({R, L, nCoils, bMagCenter});
auto bFieldMap = makeFieldMap(bSolenoidField);
auto bCache = bFieldMap.makeCache(Acts::MagneticFieldContext{});

struct StreamWrapper {
  StreamWrapper(std::ofstream ofstr) : m_ofstr(std::move(ofstr)) {
    m_ofstr << "x;y;z;B_x;B_y;B_z;Bm_x;Bm_y;Bm_z" << std::endl;
  }

  std::ofstream m_ofstr;
};

StreamWrapper valid(std::ofstream("magfield_lookup.csv"));

const int ntests = 10000;
BOOST_DATA_TEST_CASE(
    solenoid_interpolated_bfield_comparison,
    bdata::random((bdata::engine = std::mt19937(), bdata::seed = 1,
                   bdata::distribution = std::uniform_real_distribution<double>(
                       1.5 * (-L / 2.), 1.5 * L / 2.))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 2,
                       bdata::distribution =
                           std::uniform_real_distribution<double>(0,
                                                                  R * 1.5))) ^
        bdata::random(
            (bdata::engine = std::mt19937(), bdata::seed = 3,
             bdata::distribution = std::uniform_real_distribution<double>(
                 -std::numbers::pi, std::numbers::pi))) ^
        bdata::xrange(ntests),
    z, r, phi, index) {
  if (index % 1000 == 0) {
    std::cout << index << std::endl;
  }

  Vector3 pos(r * std::cos(phi), r * std::sin(phi), z);
  Vector3 B = bSolenoidField.getField(pos) / Acts::UnitConstants::T;
  Vector3 Bm = bFieldMap.getField(pos, bCache).value() / Acts::UnitConstants::T;

  // test less than 5% deviation
  if (std::abs(r - R) > 10 && (std::abs(z) < L / 3. || r > 20)) {
    // only if more than 10mm away from coil for all z
    // only if not close to r=0 for large z
    CHECK_CLOSE_REL(Bm.norm(), B.norm(), 0.05);
  }

  std::ofstream& ofstr = valid.m_ofstr;
  ofstr << pos.x() << ";" << pos.y() << ";" << pos.z() << ";";
  ofstr << B.x() << ";" << B.y() << ";" << B.z() << ";";
  ofstr << Bm.x() << ";" << Bm.y() << ";" << Bm.z() << std::endl;
}

}  // namespace Acts::IntegrationTest
