// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Plugins/Json/JsonSurfacesReader.hpp"
#include "Acts/Plugins/Json/SurfaceJsonConverter.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Utilities/Zip.hpp"

#include <filesystem>
#include <fstream>
#include <memory>

#include <Eigen/Geometry>
#include <nlohmann/json.hpp>

const std::vector<std::shared_ptr<Acts::Surface>> surfaces = []() {
  std::vector<std::shared_ptr<Acts::Surface>> v;

  for (int i = 0; i < 3; ++i) {
    auto bounds = std::make_shared<Acts::RectangleBounds>(1.0, 1.0);
    Acts::Transform3 transform = Acts::Transform3::Identity();
    transform.translate(Acts::Vector3::Random());
    Acts::Vector3 randomAngles = Acts::Vector3::Random();
    Acts::SquareMatrix3 rotMatrix;
    rotMatrix = Eigen::AngleAxis(randomAngles[0], Acts::Vector3::UnitX()) *
                Eigen::AngleAxis(randomAngles[1], Acts::Vector3::UnitY()) *
                Eigen::AngleAxis(randomAngles[2], Acts::Vector3::UnitZ());
    transform.rotate(rotMatrix);
    v.push_back(
        Acts::Surface::makeShared<Acts::PlaneSurface>(transform, bounds));
  }

  return v;
}();

const std::string filename = "json_surfaces_reader_tests.json";

struct FileFixture {
  FileFixture() {
    nlohmann::json js = nlohmann::json::array();

    for (const auto &s : surfaces) {
      js.push_back(Acts::SurfaceJsonConverter::toJson({}, *s));
    }

    nlohmann::json j;
    j["foo"] = js;

    std::ofstream f(filename);
    f << j.dump(2);
  }

  ~FileFixture() { std::filesystem::remove(filename); }
};

FileFixture fileFixture;

BOOST_AUTO_TEST_CASE(surface_reading_test) {
  auto readBackSurfaces =
      Acts::JsonSurfacesReader::readVector({filename, {"foo"}});

  BOOST_REQUIRE_EQUAL(surfaces.size(), readBackSurfaces.size());
  for (auto [refSurface, surface] : Acts::zip(surfaces, readBackSurfaces)) {
    BOOST_CHECK(
        refSurface->transform({}).isApprox(surface->transform({}), 1.e-4));
    BOOST_CHECK(refSurface->center({}).isApprox(surface->center({}), 1.e-4));
    BOOST_CHECK_EQUAL(refSurface->type(), surface->type());
    BOOST_CHECK_EQUAL(refSurface->bounds().type(), surface->bounds().type());
  }
}

BOOST_AUTO_TEST_CASE(json_detelement_reading_test) {
  auto readBackDetElements =
      Acts::JsonSurfacesReader::readDetectorElements({filename, {"foo"}}, 1.0);

  BOOST_REQUIRE_EQUAL(surfaces.size(), readBackDetElements.size());
  for (auto [refSurface, detElement] :
       Acts::zip(surfaces, readBackDetElements)) {
    auto surface = &detElement->surface();
    BOOST_CHECK(
        refSurface->transform({}).isApprox(surface->transform({}), 1.e-4));
    BOOST_CHECK(refSurface->center({}).isApprox(surface->center({}), 1.e-4));
    BOOST_CHECK_EQUAL(refSurface->type(), surface->type());
    BOOST_CHECK_EQUAL(refSurface->bounds().type(), surface->bounds().type());
  }
}
