// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/ConeVolumeBounds.hpp"
#include "Acts/Geometry/CuboidVolumeBounds.hpp"
#include "Acts/Geometry/CutoutCylinderVolumeBounds.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/GenericCuboidVolumeBounds.hpp"
#include "Acts/Geometry/TrapezoidVolumeBounds.hpp"
#include "Acts/Plugins/Json/VolumeBoundsJsonConverter.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <memory>
#include <numbers>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

using namespace Acts;

BOOST_AUTO_TEST_SUITE(VolumeBoundsJsonConversion)

BOOST_AUTO_TEST_CASE(Cuboid) {
  std::ofstream out("CuboidVolumeBounds.json");

  auto cuboidRef = std::make_shared<const CuboidVolumeBounds>(2., 4., 6.);
  nlohmann::json cuboidOut = VolumeBoundsJsonConverter::toJson(*cuboidRef);
  out << cuboidOut.dump(2);
  out.close();

  // Read in json file
  auto in = std::ifstream("CuboidVolumeBounds.json",
                          std::ifstream::in | std::ifstream::binary);
  BOOST_CHECK(in.good());
  nlohmann::json cuboidIn;
  in >> cuboidIn;
  in.close();

  auto cuboidTest =
      VolumeBoundsJsonConverter::fromJson<CuboidVolumeBounds>(cuboidIn);
  BOOST_CHECK(cuboidRef->values() == cuboidTest->values());
}

BOOST_AUTO_TEST_CASE(Cylinder) {
  std::ofstream out("CylinderVolumeBounds.json");

  auto cylinderRef = std::make_shared<const CylinderVolumeBounds>(
      10., 20., 30., std::numbers::pi / 4., 0);
  nlohmann::json cylinderOut = VolumeBoundsJsonConverter::toJson(*cylinderRef);
  out << cylinderOut.dump(2);
  out.close();

  // Read in json file
  auto in = std::ifstream("CylinderVolumeBounds.json",
                          std::ifstream::in | std::ifstream::binary);
  BOOST_CHECK(in.good());
  nlohmann::json cylinderIn;
  in >> cylinderIn;
  in.close();

  auto cylinderTest =
      VolumeBoundsJsonConverter::fromJson<CylinderVolumeBounds>(cylinderIn);
  BOOST_CHECK(cylinderRef->values() == cylinderTest->values());
}

BOOST_AUTO_TEST_CASE(Cone) {
  std::ofstream out("ConeVolumeBounds.json");

  auto coneRef = std::make_shared<const ConeVolumeBounds>(
      0., 0., 0.45, 0.050, 0.050, 0., std::numbers::pi);
  nlohmann::json coneOut = VolumeBoundsJsonConverter::toJson(*coneRef);
  out << coneOut.dump(2);
  out.close();

  // Read in json file
  auto in = std::ifstream("ConeVolumeBounds.json",
                          std::ifstream::in | std::ifstream::binary);
  BOOST_CHECK(in.good());
  nlohmann::json coneIn;
  in >> coneIn;
  in.close();

  auto coneTest = VolumeBoundsJsonConverter::fromJson<ConeVolumeBounds>(coneIn);
  BOOST_CHECK(coneRef->values() == coneTest->values());
}

BOOST_AUTO_TEST_CASE(CutoutCylinder) {
  std::ofstream out("CutoutCylinderVolumeBounds.json");

  auto cutoutCylinderRef =
      std::make_shared<const CutoutCylinderVolumeBounds>(5, 10, 15, 30, 25);
  nlohmann::json cutoutCylinderOut =
      VolumeBoundsJsonConverter::toJson(*cutoutCylinderRef);
  out << cutoutCylinderOut.dump(2);
  out.close();

  // Read in json file
  auto in = std::ifstream("CutoutCylinderVolumeBounds.json",
                          std::ifstream::in | std::ifstream::binary);
  BOOST_CHECK(in.good());
  nlohmann::json cutoutCylinderIn;
  in >> cutoutCylinderIn;
  in.close();

  auto cutoutCylinderTest =
      VolumeBoundsJsonConverter::fromJson<CutoutCylinderVolumeBounds>(
          cutoutCylinderIn);
  BOOST_CHECK(cutoutCylinderRef->values() == cutoutCylinderTest->values());
}

BOOST_AUTO_TEST_CASE(GenericCuboid) {
  std::ofstream out("GenericCuboidVolumeBounds.json");
  std::array<Vector3, 8> vertices;
  vertices = {{{0, 0, 0},
               {2, 0, 0},
               {2, 1, 0},
               {0, 1, 0},
               {0, 0, 1},
               {2, 0, 1},
               {2, 1, 1},
               {0, 1, 1}}};

  auto genericCuboidRef =
      std::make_shared<const GenericCuboidVolumeBounds>(vertices);
  nlohmann::json genericCuboidOut =
      VolumeBoundsJsonConverter::toJson(*genericCuboidRef);
  out << genericCuboidOut.dump(2);
  out.close();

  // Read in json file
  auto in = std::ifstream("GenericCuboidVolumeBounds.json",
                          std::ifstream::in | std::ifstream::binary);
  BOOST_CHECK(in.good());
  nlohmann::json genericCuboidIn;
  in >> genericCuboidIn;
  in.close();

  auto genericCuboidTest = VolumeBoundsJsonConverter::fromJson(genericCuboidIn);
  BOOST_CHECK(genericCuboidRef->values() == genericCuboidTest->values());
}

BOOST_AUTO_TEST_CASE(Trapezoid) {
  std::ofstream out("TrapezoidVolumeBounds.json");

  auto trapezoidRef =
      std::make_shared<const TrapezoidVolumeBounds>(2., 4., 6., 8.);
  nlohmann::json trapezoidOut =
      VolumeBoundsJsonConverter::toJson(*trapezoidRef);
  out << trapezoidOut.dump(2);
  out.close();

  // Read in json file
  auto in = std::ifstream("TrapezoidVolumeBounds.json",
                          std::ifstream::in | std::ifstream::binary);
  BOOST_CHECK(in.good());
  nlohmann::json trapezoidIn;
  in >> trapezoidIn;
  in.close();

  auto trapezoidTest =
      VolumeBoundsJsonConverter::fromJson<TrapezoidVolumeBounds>(trapezoidIn);
  BOOST_CHECK(trapezoidRef->values() == trapezoidTest->values());
}
BOOST_AUTO_TEST_SUITE_END()
