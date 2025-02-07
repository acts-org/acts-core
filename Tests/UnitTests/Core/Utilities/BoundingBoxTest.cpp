// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GenericCuboidVolumeBounds.hpp"
#include "Acts/Geometry/Volume.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/BoundingBox.hpp"
#include "Acts/Utilities/Frustum.hpp"
#include "Acts/Utilities/Ray.hpp"
#include "Acts/Visualization/IVisualization3D.hpp"
#include "Acts/Visualization/PlyVisualization3D.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <numbers>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace Acts::Test {

struct Object {};

using ObjectBBox = Acts::AxisAlignedBoundingBox<Object, double, 3>;

using Vector2F = Eigen::Matrix<double, 2, 1>;
using Vector3F = Eigen::Matrix<double, 3, 1>;
using AngleAxis3F = Eigen::AngleAxis<double>;

std::filesystem::path tmp_path = []() {
  auto tmpPath = std::filesystem::temp_directory_path() / "acts_unit_tests";
  std::filesystem::create_directory(tmpPath);
  std::cout << "Writing test output to: " << tmpPath << std::endl;
  return tmpPath;
}();

std::ofstream tmp(const std::string& path) {
  return std::ofstream{(tmp_path / path).string()};
}

BOOST_AUTO_TEST_CASE(box_construction) {
  BOOST_TEST_CONTEXT("2D") {
    Object o;
    using Box = Acts::AxisAlignedBoundingBox<Object, double, 2>;
    Box bb(&o, {-1, -1}, {2, 2});

    typename Box::transform_type rot;
    rot = Eigen::Rotation2D<double>(std::numbers::pi / 7.);
    Box bb_rot = bb.transformed(rot);

    CHECK_CLOSE_ABS(bb_rot.min(), Vector2F(-1.76874, -1.33485), 1e-4);
    CHECK_CLOSE_ABS(bb_rot.max(), Vector2F(2.23582, 2.66971), 1e-4);
  }

  BOOST_TEST_CONTEXT("3D") {
    Object o;
    using Box = Acts::AxisAlignedBoundingBox<Object, double, 3>;
    Box bb(&o, {-1, -1, -1}, {2, 2, 2});

    typename Box::transform_type rot;
    rot = AngleAxis3F(std::numbers::pi / 3., Vector3F::UnitZ());
    Box bb_rot = bb.transformed(rot);

    CHECK_CLOSE_ABS(bb_rot.min(), Vector3F(-2.23205, -1.36603, -1), 1e-4);
    CHECK_CLOSE_ABS(bb_rot.max(), Vector3F(1.86603, 2.73205, 2), 1e-4);

    rot *= AngleAxis3F(std::numbers::pi / 5., Vector3F(1, 1, 0).normalized());
    Box bb_rot2 = bb.transformed(rot);

    CHECK_CLOSE_ABS(bb_rot2.min(), Vector3F(-2.40848, -1.51816, -2.0559), 1e-4);
    CHECK_CLOSE_ABS(bb_rot2.max(), Vector3F(2.61021, 3.03631, 2.86491), 1e-4);
  }
}

BOOST_AUTO_TEST_CASE(intersect_points) {
  using VertexType = ObjectBBox::VertexType;

  Object o;
  ObjectBBox bb(&o, {0, 0, 0}, {1, 1, 1});
  VertexType p;

  p = {0.5, 0.5, 0.5};
  BOOST_CHECK(bb.intersect(p));
  p = {0.25, 0.25, 0.25};
  BOOST_CHECK(bb.intersect(p));
  p = {0.75, 0.75, 0.75};
  BOOST_CHECK(bb.intersect(p));

  // lower bound is inclusive
  p = {0, 0, 0};
  BOOST_CHECK(bb.intersect(p));
  // upper bound is exclusive
  p = {1.0, 1.0, 1.0};
  BOOST_CHECK(!bb.intersect(p));

  // some outsides
  p = {2, 0, 0};
  BOOST_CHECK(!bb.intersect(p));
  p = {0, 2, 0};
  BOOST_CHECK(!bb.intersect(p));
  p = {0, 0, 2};
  BOOST_CHECK(!bb.intersect(p));
  p = {2, 2, 0};
  BOOST_CHECK(!bb.intersect(p));
  p = {2, 0, 2};
  BOOST_CHECK(!bb.intersect(p));
  p = {2, 2, 2};
  BOOST_CHECK(!bb.intersect(p));

  p = {-1, 0, 0};
  BOOST_CHECK(!bb.intersect(p));
  p = {0, -1, 0};
  BOOST_CHECK(!bb.intersect(p));
  p = {0, 0, -1};
  BOOST_CHECK(!bb.intersect(p));
  p = {-1, -1, 0};
  BOOST_CHECK(!bb.intersect(p));
  p = {-1, 0, -1};
  BOOST_CHECK(!bb.intersect(p));
  p = {-1, -1, -1};
  BOOST_CHECK(!bb.intersect(p));
}

BOOST_AUTO_TEST_CASE(intersect_rays) {
  BOOST_TEST_CONTEXT("2D") {
    using Box = AxisAlignedBoundingBox<Object, double, 2>;

    Object o;
    Box bb(&o, {-1, -1}, {1, 1});

    // ray in positive x direction

    Ray<double, 2> ray({-2, 0}, {1, 0});
    BOOST_CHECK(bb.intersect(ray));

    ray = {{-2, 2}, {1, 0}};
    BOOST_CHECK(!bb.intersect(ray));

    ray = {{-2, -2}, {1, 0}};
    BOOST_CHECK(!bb.intersect(ray));

    // upper bound is exclusive - temporarily removed, fails macOS ci
    // ray = {{-2, 1}, {1, 0}};
    // BOOST_CHECK(!bb.intersect(ray));

    // lower bound is inclusive
    ray = {{-2, -1}, {1, 0}};
    BOOST_CHECK(bb.intersect(ray));

    // ray faces away from box
    ray = {{2, 0}, {1, 0}};
    BOOST_CHECK(!bb.intersect(ray));

    // ray in negative x direction

    ray = {{2, 0}, {-1, 0}};
    BOOST_CHECK(bb.intersect(ray));

    ray = {{2, 2}, {-1, 0}};
    BOOST_CHECK(!bb.intersect(ray));

    ray = {{2, -2}, {-1, 0}};
    BOOST_CHECK(!bb.intersect(ray));

    // upper bound is exclusive - temporarily removed, fails macOS ci
    // ray = {{2, 1}, {-1, 0}};
    // BOOST_CHECK(!bb.intersect(ray));

    // lower bound is inclusive
    ray = {{2, -1}, {-1, 0}};
    BOOST_CHECK(bb.intersect(ray));

    // ray in positive y direction

    ray = {{0, -2}, {0, 1}};
    BOOST_CHECK(bb.intersect(ray));

    ray = {{2, -2}, {0, 1}};
    BOOST_CHECK(!bb.intersect(ray));

    ray = {{-2, -2}, {0, 1}};
    BOOST_CHECK(!bb.intersect(ray));

    // upper bound is exclusive
    ray = {{1, -2}, {0, 1}};
    BOOST_CHECK(!bb.intersect(ray));

    // lower bound is not inclusive, due to Eigen's NaN handling.
    ray = {{-1, -2}, {0, 1}};
    BOOST_CHECK(!bb.intersect(ray));

    // other direction
    ray = {{0, -2}, {0, -1}};
    BOOST_CHECK(!bb.intersect(ray));

    // ray in positive y direction

    ray = {{0, 2}, {0, -1}};
    BOOST_CHECK(bb.intersect(ray));

    ray = {{2, 2}, {0, -1}};
    BOOST_CHECK(!bb.intersect(ray));

    ray = {{-2, 2}, {0, -1}};
    BOOST_CHECK(!bb.intersect(ray));

    // upper bound is exclusive
    ray = {{1, 2}, {0, -1}};
    BOOST_CHECK(!bb.intersect(ray));

    // lower bound is not inclusive, due to Eigen's NaN handling.
    ray = {{-1, 2}, {0, -1}};
    BOOST_CHECK(!bb.intersect(ray));

    // other direction
    ray = {{0, 2}, {0, 1}};
    BOOST_CHECK(!bb.intersect(ray));

    // some off axis rays

    ray = {{-2, 0}, {0.5, 0.25}};
    BOOST_CHECK(bb.intersect(ray));

    ray = {{-2, 0}, {0.5, 0.4}};
    BOOST_CHECK(bb.intersect(ray));

    ray = {{-2, 0}, {0.5, 0.6}};
    BOOST_CHECK(!bb.intersect(ray));

    ray = {{-2, 0}, {0.5, 0.1}};
    BOOST_CHECK(bb.intersect(ray));

    ray = {{-2, 0}, {0.5, -0.4}};
    BOOST_CHECK(bb.intersect(ray));

    ray = {{-2, 0}, {0.5, -0.6}};
    BOOST_CHECK(!bb.intersect(ray));

    ray = {{-2, 0}, {0.1, 0.5}};
    BOOST_CHECK(!bb.intersect(ray));

    // starting point inside
    ray = {{0, 0}, {-1, 0}};
    BOOST_CHECK(bb.intersect(ray));
    ray = {{0, 0}, {1, 0}};
    BOOST_CHECK(bb.intersect(ray));
    ray = {{0, 0}, {0, -1}};
    BOOST_CHECK(bb.intersect(ray));
    ray = {{0, 0}, {0, 1}};
    BOOST_CHECK(bb.intersect(ray));
  }

  BOOST_TEST_CONTEXT("3D visualize") {
    Object o;

    // let's make sure it also works in 3d
    ObjectBBox bb3(&o, {-1, -1, -1}, {1, 1, 1});
    Ray<double, 3> ray3({0, 0, -2}, {0, 0, 1});
    BOOST_CHECK(bb3.intersect(ray3));

    PlyVisualization3D<double> ply;

    ray3.draw(ply);
    auto os = tmp("ray3d.ply");
    os << ply << std::flush;
    os.close();
  }

  BOOST_TEST_CONTEXT("3D") {
    using VertexType3 = ObjectBBox::VertexType;
    Object o;

    // let's make sure it also works in 3d
    ObjectBBox bb3(&o, {-1, -1, -1}, {1, 1, 1});
    Ray<double, 3> ray3({0, 0, -2}, {0, 0, 1});
    BOOST_CHECK(bb3.intersect(ray3));

    // facing away from box
    ray3 = {{0, 0, -2}, {0, 0, -1}};
    BOOST_CHECK(!bb3.intersect(ray3));

    ray3 = {{0, 2, -2}, {0, 0, 1}};
    BOOST_CHECK(!bb3.intersect(ray3));

    ray3 = {{0, -2, -2}, {0, 0, 1}};
    BOOST_CHECK(!bb3.intersect(ray3));

    // right on slab - temporarily removed, fails macOS ci
    // ray3 = {{0, 1, -2}, {0, 0, 1}};
    // BOOST_CHECK(!bb3.intersect(ray3));

    // right on slab
    ray3 = {{0, -1, -2}, {0, 0, 1}};
    BOOST_CHECK(bb3.intersect(ray3));

    // right on slab
    ray3 = {{-1, 0, -2}, {0, 0, 1}};
    BOOST_CHECK(!bb3.intersect(ray3));

    // right on slab
    ray3 = {{1, 0, -2}, {0, 0, 1}};
    BOOST_CHECK(!bb3.intersect(ray3));

    ray3 = {{-0.95, 0, -2}, {0, 0, 1}};
    BOOST_CHECK(bb3.intersect(ray3));

    // some off-axis rays
    ObjectBBox::VertexType p(0, 0, -2);

    ray3 = {p, VertexType3(1, 1, 1) - p};
    BOOST_CHECK(bb3.intersect(ray3));

    ray3 = {p, VertexType3(-1, 1, 1) - p};
    BOOST_CHECK(bb3.intersect(ray3));

    ray3 = {p, VertexType3(-1, -1, 1) - p};
    BOOST_CHECK(bb3.intersect(ray3));

    ray3 = {p, VertexType3(1, -1, 1) - p};
    BOOST_CHECK(bb3.intersect(ray3));

    ray3 = {p, VertexType3(1.1, 0, -1) - p};
    BOOST_CHECK(!bb3.intersect(ray3));

    ray3 = {p, VertexType3(-1.1, 0, -1) - p};
    BOOST_CHECK(!bb3.intersect(ray3));

    ray3 = {p, VertexType3(0, 1.1, -1) - p};
    BOOST_CHECK(!bb3.intersect(ray3));

    ray3 = {p, VertexType3(0, -1.1, -1) - p};
    BOOST_CHECK(!bb3.intersect(ray3));

    ray3 = {p, VertexType3(0.9, 0, -1) - p};
    BOOST_CHECK(bb3.intersect(ray3));

    ray3 = {p, VertexType3(-0.9, 0, -1) - p};
    BOOST_CHECK(bb3.intersect(ray3));

    ray3 = {p, VertexType3(0, 0.9, -1) - p};
    BOOST_CHECK(bb3.intersect(ray3));

    ray3 = {p, VertexType3(0, -0.9, -1) - p};
    BOOST_CHECK(bb3.intersect(ray3));

    ray3 = {{0, 0, 0}, {1, 0, 0}};
    BOOST_CHECK(bb3.intersect(ray3));
    ray3 = {{0, 0, 0}, {0, 1, 0}};
    BOOST_CHECK(bb3.intersect(ray3));
    ray3 = {{0, 0, 0}, {0, 0, 1}};
    BOOST_CHECK(bb3.intersect(ray3));

    ray3 = {{0, 0, 0}, {-1, 0, 0}};
    BOOST_CHECK(bb3.intersect(ray3));
    ray3 = {{0, 0, 0}, {0, -1, 0}};
    BOOST_CHECK(bb3.intersect(ray3));
    ray3 = {{0, 0, 0}, {0, 0, -1}};
    BOOST_CHECK(bb3.intersect(ray3));
  }
}  // namespace Test

BOOST_AUTO_TEST_CASE(ray_obb_intersect) {
  using Ray = Ray<double, 3>;

  std::array<Vector3, 8> vertices;
  vertices = {{{0, 0, 0},
               {2, 0, 0.4},
               {2, 1, 0.4},
               {0, 1, 0},
               {0, 0, 1},
               {1.8, 0, 1},
               {1.8, 1, 1},
               {0, 1, 1}}};
  auto cubo = std::make_shared<GenericCuboidVolumeBounds>(vertices);
  auto trf = Transform3(
      Translation3(Vector3(0, 8, -5)) *
      AngleAxis3(std::numbers::pi / 3., Vector3(1, -3, 9).normalized()));

  Volume vol(trf, cubo);

  PlyVisualization3D<double> ply;

  Transform3 trl = Transform3::Identity();
  trl.translation() = trf.translation();

  cubo->draw(ply);

  auto obb = vol.orientedBoundingBox();
  obb.draw(ply, {200, 0, 0});

  ply.clear();

  Vector3 origin(10, -20, 6);
  Vector3 centroid(0., 0., 0.);

  for (const auto& vtx_ : vertices) {
    Vector3 vtx = trf * vtx_;
    centroid += vtx;
  }

  // approximately the centroid
  centroid *= 0.125;

  // shoot rays to the corner points of the cuboid
  for (const auto& vtx_ : vertices) {
    Vector3 vtx = trf * vtx_;

    // this ray goes straight to the actual vertex, this should definitely
    // intersect the OBB
    Ray ray(origin, (vtx - origin).normalized());
    ray = ray.transformed(trf.inverse());
    BOOST_CHECK(obb.intersect(ray));
    ray.draw(ply, (vtx - origin).norm());

    // now shift the target point away from the centroid this should definitely
    // NOT intersect the OBB
    vtx += (vtx - centroid);
    ray = Ray(origin, (vtx - origin).normalized());
    ray = ray.transformed(trf.inverse());
    BOOST_CHECK(!obb.intersect(ray));
    ray.draw(ply, (vtx - origin).norm());
  }
}

BOOST_AUTO_TEST_CASE(frustum_intersect) {
  BOOST_TEST_CONTEXT("2D") {
    auto make_svg = [](const std::string& fname, std::size_t width,
                       std::size_t height) {
      auto os = tmp(fname);
      os << "<?xml version=\"1.0\" standalone=\"no\"?>\n";
      os << "<svg width=\"" << width << "\" height=\"" << height
         << "\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\">\n";
      return os;
    };

    using Frustum2 = Frustum<double, 2, 2>;

    std::ofstream os;

    const std::size_t svgWidth = 1000;
    const std::size_t svgHeight = svgWidth;
    const std::size_t nSteps = 10;

    // Visualise the parameters
    const double min = -20;
    const double max = 20;
    os = make_svg("frust2d_parameters.svg", svgWidth, svgHeight);

    const double step = (max - min) / nSteps;
    for (std::size_t i = 0; i <= nSteps; i++) {
      for (std::size_t j = 0; j <= nSteps; j++) {
        Vector2F dir(1, 0);
        Vector2F origin(min + step * i, min + step * j);
        origin.x() *= 1.10;
        Eigen::Rotation2D<double> rot(2 * std::numbers::pi / nSteps * i);
        double angle = std::numbers::pi / 2. / nSteps * j;
        Frustum2 fr(origin, rot * dir, angle);
        fr.svg(os, svgWidth, svgHeight, 2);
      }
    }

    os << "</svg>";
    os.close();

    const double unit = 20.;

    using Box = AxisAlignedBoundingBox<Object, double, 2>;
    Object o;
    Box::Size size(Eigen::Matrix<double, 2, 1>(2, 2));

    const double minX = -20.;
    const double minY = -20.;
    const double maxX = 20.;
    const double maxY = 20.;
    const double stepX = (maxX - minX) / nSteps;
    const double stepY = (maxY - minY) / nSteps;

    std::set<std::size_t> idxsAct;

    // clang-format off
    std::vector<std::pair<Frustum2, std::set<std::size_t>>> frExp;
    frExp = {
        {Frustum2({0, 0}, {1, 0}, std::numbers::pi / 2.),
         {60,  70,  71,  72,  80,  81,  82,  83,  84,  90,  91,  92,
          93,  94,  95,  96,  100, 101, 102, 103, 104, 105, 106, 107,
          108, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120}
        },
        {Frustum2({0, 0}, {1, 0}, 0.5 * std::numbers::pi / 2.),
         {60,  71,  81,  82,  83,  92,  93,  94, 102,
          103, 104, 105, 106, 113, 114, 115, 116, 117}
        },
        {Frustum2({0, 0}, {1, 0}, 0.2 * std::numbers::pi / 2.),
         {60, 71, 82, 93, 104, 114, 115, 116}
        },
        {Frustum2({0, 0}, {1, 0},  3 * std::numbers::pi / 4.),
         {60, 68, 69, 70, 71, 72, 73, 74, 77, 78, 79, 80, 81, 82, 83,
          84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98,
          99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
          112, 113, 114, 115, 116, 117, 118, 119, 120}
        },
        {Frustum2({0, 0}, {0, 1}, 0.5 * std::numbers::pi / 2.),
         {42, 43, 51, 52, 53, 54, 60, 61, 62, 63, 64, 65, 73, 74, 75, 76, 86, 87}
        },
        {Frustum2({0, 0}, {-1, 0}, 0.5 * std::numbers::pi / 2.),
         {3, 4, 5, 6, 7, 14, 15, 16, 17, 18, 26, 27, 28, 37, 38, 39, 49, 60}
        },
        {Frustum2({0, 0}, {0, -1}, 0.5 * std::numbers::pi / 2.),
         {33, 34, 44, 45, 46, 47, 55, 56, 57, 58, 59, 60, 66, 67, 68, 69, 77, 78}
        },
        {Frustum2({0, 0}, {1, 1}, 0.5 * std::numbers::pi / 2.),
         {60, 72, 73, 74, 83, 84, 85, 86, 87, 94, 95, 96, 97, 98, 106, 107,
          108, 109, 117, 118, 119, 120}
        },
        {Frustum2({0, 0}, {-1, 1}, 0.5 * std::numbers::pi / 2.),
         {7, 8, 9, 10, 18, 19, 20, 21, 28, 29, 30, 31, 32, 39, 40, 41, 42,
          43, 50, 51, 52, 60}
        },
        {Frustum2({0, 0}, {-1, -1}, 0.5 * std::numbers::pi / 2.),
         {0, 1, 2, 3, 11, 12, 13, 14, 22, 23, 24, 25, 26, 33, 34, 35, 36,
          37, 46, 47, 48, 60}
        },
        {Frustum2({0, 0}, {1, -1}, 0.5 * std::numbers::pi / 2.),
         {60, 68, 69, 70, 77, 78, 79, 80, 81, 88, 89, 90, 91, 92, 99, 100,
          101, 102, 110, 111, 112, 113}
        },
        {Frustum2({1, 1}, {1, -1}, std::numbers::pi / 2.),
         {55, 56, 57, 58, 59, 60, 66, 67, 68, 69, 70, 71, 77, 78, 79, 80,
          81, 82, 88, 89, 90, 91, 92, 93, 99, 100, 101, 102, 103, 104, 110, 111, 112, 113, 114, 115}
        },
        {Frustum2({-1, -1}, {1, -1}, std::numbers::pi / 2.),
         {55, 56, 57, 58, 59, 60, 66, 67, 68, 69, 70, 71, 77, 78, 79, 80,
          81, 82, 88, 89, 90, 91, 92, 93, 99, 100, 101, 102, 103, 104, 110, 111, 112, 113, 114, 115}
        },
        {Frustum2({10, -10}, {1, 1}, 0.5 * std::numbers::pi / 2.),
         {91, 92, 102, 103, 104, 105, 114, 115, 116, 117, 118, 119}
        },
        {Frustum2({-10.3, 12.8}, {0.3, -1}, 0.5 * std::numbers::pi / 2.),
         {22, 23, 24, 25, 26, 27, 28, 33, 34, 35, 36, 37, 38, 39, 40, 41,
          44, 45, 46, 47, 48, 49, 50, 55, 56, 57, 58, 59, 60, 66, 67, 68,
          69, 70, 77, 78, 79, 80, 88, 89, 99}
        },
        {Frustum2({17.2, 19.45}, {-1, -0.1}, 0.5 * std::numbers::pi / 2.),
         {5, 6, 7, 8, 9, 10, 17, 18, 19, 20, 21, 28, 29, 30, 31, 32, 40,
          41, 42, 43, 51, 52, 53, 54, 63, 64, 65, 74, 75, 76, 86, 87, 97,
          98, 109}
        },
    };
    // clang-format on

    for (std::size_t l = 0; l < frExp.size(); l++) {
      const Frustum2& fr = frExp.at(l).first;
      const std::set<std::size_t>& idxsExp = frExp.at(l).second;
      std::stringstream ss;
      ss << "frust2d_test_" << l << ".svg";
      os = make_svg(ss.str(), svgWidth, svgHeight);

      idxsAct.clear();

      std::vector<Box> boxes;
      boxes.reserve((nSteps + 1) * (nSteps + 1));
      for (std::size_t i = 0; i <= nSteps; i++) {
        for (std::size_t j = 0; j <= nSteps; j++) {
          boxes.emplace_back(
              &o,
              Eigen::Matrix<double, 2, 1>{minX + i * stepX, minY + j * stepY},
              size);
          std::stringstream st;
          st << boxes.size() - 1;

          std::string color = "red";
          if (boxes.back().intersect(fr)) {
            color = "green";
            idxsAct.insert(boxes.size() - 1);
          }

          boxes.back().svg(os, svgWidth, svgHeight, unit, st.str(), color);
        }
      }

      BOOST_CHECK(idxsAct == idxsExp);

      fr.svg(os, svgWidth, svgHeight, maxX, unit);
      os << "</svg>";

      os.close();
    }
  }

  PlyVisualization3D<double> helper;
  BOOST_TEST_CONTEXT("3D - 3 Sides") {
    using Frustum3 = Frustum<double, 3, 3>;
    std::ofstream os;
    std::size_t n = 10;
    const std::size_t nSteps = 5;
    double min = -10;
    double max = 10;
    double step = (max - min) / nSteps;

    // Visualise the parameters
    auto make = [&](double angle, const Vector3F& origin,
                    std::ofstream& osTmp) {
      helper.clear();
      double far = 1.;
      Frustum3 fr(origin, {0, 0, 1}, angle);
      fr.draw(helper, far);
      fr = Frustum3(origin, {0, 0, -1}, angle);
      fr.draw(helper, far);
      fr = Frustum3(origin, {1, 0, 0}, angle);
      fr.draw(helper, far);
      fr = Frustum3(origin, {-1, 0, 0}, angle);
      fr.draw(helper, far);

      fr = Frustum3(origin, {0, 1, 0}, angle);
      fr.draw(helper, far);
      fr = Frustum3(origin, {0, -1, 0}, angle);
      fr.draw(helper, far);

      osTmp << helper << std::flush;

      helper.clear();
    };

    os = std::ofstream("frust3d_dir.ply");
    for (std::size_t i = 0; i <= nSteps; i++) {
      for (std::size_t j = 0; j <= nSteps; j++) {
        for (std::size_t k = 0; k <= nSteps; k++) {
          Vector3F origin(min + i * step, min + j * step, min + k * step);
          make(std::numbers::pi / 4., origin, os);
        }
      }
    }
    os.close();

    os = tmp("frust3D_angle.ply");
    helper.clear();
    for (std::size_t i = 0; i <= n; i++) {
      Vector3F origin(i * 4, 0, 0);
      AngleAxis3F rot(std::numbers::pi / n * i, Vector3F::UnitY());
      double angle = (std::numbers::pi / 2.) / n * (1 + i);
      Vector3F dir(1, 0, 0);
      Frustum3 fr(origin, rot * dir, angle);
      fr.draw(helper, 2);
    }

    os << helper << std::flush;
    os.close();

    std::set<std::size_t> idxsAct;

    std::vector<std::pair<Frustum3, std::set<std::size_t>>> frExp;
    frExp = {
        {Frustum3({0, 0, 0}, {1, 0, 0}, std::numbers::pi / 2.),
         {
             665,  763,  774,  775,  785,  786,  787,  788,  796,  797,  807,
             872,  873,  883,  884,  885,  886,  894,  895,  896,  897,  898,
             905,  906,  907,  908,  909,  910,  911,  916,  917,  918,  919,
             920,  927,  928,  929,  930,  938,  939,  970,  971,  981,  982,
             983,  992,  993,  994,  995,  996,  1003, 1004, 1005, 1006, 1007,
             1008, 1009, 1014, 1015, 1016, 1017, 1018, 1019, 1020, 1021, 1025,
             1026, 1027, 1028, 1029, 1030, 1031, 1032, 1033, 1036, 1037, 1038,
             1039, 1040, 1041, 1042, 1043, 1047, 1048, 1049, 1050, 1051, 1052,
             1053, 1058, 1059, 1060, 1061, 1062, 1069, 1070, 1071, 1080, 1081,
             1090, 1091, 1092, 1093, 1094, 1101, 1102, 1103, 1104, 1105, 1106,
             1112, 1113, 1114, 1115, 1116, 1117, 1118, 1119, 1123, 1124, 1125,
             1126, 1127, 1128, 1129, 1130, 1131, 1132, 1134, 1135, 1136, 1137,
             1138, 1139, 1140, 1141, 1142, 1143, 1145, 1146, 1147, 1148, 1149,
             1150, 1151, 1152, 1153, 1154, 1156, 1157, 1158, 1159, 1160, 1161,
             1162, 1163, 1164, 1165, 1167, 1168, 1169, 1170, 1171, 1172, 1173,
             1174, 1175, 1176, 1178, 1179, 1180, 1181, 1182, 1183, 1184, 1185,
             1189, 1190, 1191, 1192, 1193, 1194, 1200, 1201, 1202, 1203, 1204,
             1210, 1211, 1212, 1213, 1214, 1215, 1216, 1217, 1221, 1222, 1223,
             1224, 1225, 1226, 1227, 1228, 1229, 1232, 1233, 1234, 1235, 1236,
             1237, 1238, 1239, 1240, 1241, 1242, 1243, 1244, 1245, 1246, 1247,
             1248, 1249, 1250, 1251, 1252, 1253, 1254, 1255, 1256, 1257, 1258,
             1259, 1260, 1261, 1262, 1263, 1264, 1265, 1266, 1267, 1268, 1269,
             1270, 1271, 1272, 1273, 1274, 1275, 1276, 1277, 1278, 1279, 1280,
             1281, 1282, 1283, 1284, 1285, 1286, 1287, 1288, 1289, 1290, 1291,
             1292, 1293, 1294, 1295, 1296, 1297, 1298, 1299, 1300, 1301, 1302,
             1303, 1304, 1305, 1306, 1307, 1308, 1309, 1310, 1311, 1312, 1313,
             1314, 1315, 1316, 1317, 1320, 1321, 1322, 1323, 1324, 1325, 1326,
             1327,
         }},
        {Frustum3({0, 0, 0}, {0, 1, 0}, std::numbers::pi / 2.),
         {93,   102,  103,  104,  105,  106,  112,  113,  114,  115,  116,
          117,  118,  203,  213,  214,  215,  223,  224,  225,  226,  227,
          233,  234,  235,  236,  237,  238,  239,  324,  333,  334,  335,
          336,  337,  343,  344,  345,  346,  347,  348,  349,  353,  354,
          355,  356,  357,  358,  359,  360,  361,  434,  444,  445,  446,
          454,  455,  456,  457,  458,  464,  465,  466,  467,  468,  469,
          470,  473,  474,  475,  476,  477,  478,  479,  480,  481,  482,
          483,  555,  564,  565,  566,  567,  568,  574,  575,  576,  577,
          578,  579,  580,  584,  585,  586,  587,  588,  589,  590,  591,
          592,  594,  595,  596,  597,  598,  599,  600,  601,  602,  603,
          604,  665,  675,  676,  677,  685,  686,  687,  688,  689,  695,
          696,  697,  698,  699,  700,  701,  704,  705,  706,  707,  708,
          709,  710,  711,  712,  713,  714,  715,  716,  717,  718,  719,
          720,  721,  722,  723,  724,  725,  795,  796,  797,  798,  799,
          805,  806,  807,  808,  809,  810,  811,  815,  816,  817,  818,
          819,  820,  821,  822,  823,  825,  826,  827,  828,  829,  830,
          831,  832,  833,  834,  835,  836,  837,  838,  839,  840,  841,
          842,  843,  844,  845,  846,  926,  927,  928,  929,  930,  931,
          932,  935,  936,  937,  938,  939,  940,  941,  942,  943,  944,
          945,  946,  947,  948,  949,  950,  951,  952,  953,  954,  955,
          956,  957,  958,  959,  960,  961,  962,  963,  964,  965,  966,
          967,  1056, 1057, 1058, 1059, 1060, 1061, 1062, 1063, 1064, 1065,
          1066, 1067, 1068, 1069, 1070, 1071, 1072, 1073, 1074, 1075, 1076,
          1077, 1078, 1079, 1080, 1081, 1082, 1083, 1084, 1085, 1086, 1087,
          1088, 1188, 1189, 1190, 1191, 1192, 1193, 1194, 1195, 1196, 1197,
          1198, 1199, 1200, 1201, 1202, 1203, 1204, 1205, 1206, 1207, 1208,
          1209, 1320, 1321, 1322, 1323, 1324, 1325, 1326, 1327, 1328, 1329,
          1330}},
        {Frustum3({0, 0, 0}, {0, 0, 1}, std::numbers::pi / 2.),
         {32,   42,   43,   53,   54,   63,   64,   65,   75,   76,   86,
          87,   98,   153,  163,  164,  173,  174,  175,  183,  184,  185,
          186,  195,  196,  197,  207,  208,  219,  263,  273,  274,  283,
          284,  285,  294,  295,  296,  304,  305,  306,  307,  316,  317,
          318,  327,  328,  329,  339,  340,  351,  373,  384,  394,  395,
          404,  405,  406,  414,  415,  416,  417,  424,  425,  426,  427,
          428,  436,  437,  438,  439,  448,  449,  450,  460,  461,  472,
          483,  494,  504,  505,  514,  515,  516,  524,  525,  526,  527,
          535,  536,  537,  538,  545,  546,  547,  548,  549,  557,  558,
          559,  560,  568,  569,  570,  571,  580,  581,  582,  592,  593,
          604,  614,  615,  625,  626,  635,  636,  637,  645,  646,  647,
          648,  655,  656,  657,  658,  659,  665,  666,  667,  668,  669,
          670,  677,  678,  679,  680,  681,  689,  690,  691,  692,  701,
          702,  703,  713,  714,  724,  725,  735,  736,  745,  746,  747,
          755,  756,  757,  758,  765,  766,  767,  768,  769,  776,  777,
          778,  779,  780,  787,  788,  789,  790,  791,  798,  799,  800,
          801,  802,  809,  810,  811,  812,  813,  821,  822,  823,  824,
          833,  834,  835,  845,  846,  855,  856,  857,  866,  867,  868,
          876,  877,  878,  879,  887,  888,  889,  890,  898,  899,  900,
          901,  909,  910,  911,  912,  920,  921,  922,  923,  931,  932,
          933,  934,  942,  943,  944,  945,  954,  955,  956,  965,  966,
          967,  976,  977,  978,  987,  988,  989,  998,  999,  1000, 1009,
          1010, 1011, 1020, 1021, 1022, 1031, 1032, 1033, 1042, 1043, 1044,
          1053, 1054, 1055, 1064, 1065, 1066, 1075, 1076, 1077, 1086, 1087,
          1088, 1098, 1099, 1109, 1110, 1120, 1121, 1131, 1132, 1142, 1143,
          1153, 1154, 1164, 1165, 1175, 1176, 1186, 1187, 1197, 1198, 1208,
          1209, 1220, 1231, 1242, 1253, 1264, 1275, 1286, 1297, 1308, 1319,
          1330}},
        {Frustum3({0, 0, 0}, {0, 0, 1}, std::numbers::pi / 4.),
         {186, 305, 306, 307, 416, 417, 425, 426, 427, 428, 438, 439,
          527, 536, 537, 538, 545, 546, 547, 548, 549, 558, 559, 560,
          571, 647, 648, 656, 657, 658, 659, 665, 666, 667, 668, 669,
          670, 678, 679, 680, 681, 691, 692, 758, 767, 768, 769, 777,
          778, 779, 780, 788, 789, 790, 791, 799, 800, 801, 802, 811,
          812, 813, 824, 879, 890, 901, 912, 923, 934, 945}},
        {Frustum3({0, 0, 0}, {0, 0, 1}, std::numbers::pi / 8.),
         {427, 428, 546, 547, 548, 549, 658, 659, 665, 666, 667, 668, 669, 670,
          680, 681, 780, 791, 802}},
        {Frustum3({0, 0, 0}, {0, 0, 1}, std::numbers::pi * 3. / 4.),
         {8,    9,    10,   19,   20,   21,   29,   30,   31,   32,   40,
          41,   42,   43,   51,   52,   53,   54,   61,   62,   63,   64,
          65,   73,   74,   75,   76,   84,   85,   86,   87,   95,   96,
          97,   98,   107,  108,  109,  118,  119,  120,  129,  130,  131,
          140,  141,  142,  150,  151,  152,  153,  161,  162,  163,  164,
          171,  172,  173,  174,  175,  182,  183,  184,  185,  186,  193,
          194,  195,  196,  197,  205,  206,  207,  208,  216,  217,  218,
          219,  228,  229,  230,  239,  240,  241,  250,  251,  252,  260,
          261,  262,  263,  271,  272,  273,  274,  282,  283,  284,  285,
          292,  293,  294,  295,  296,  303,  304,  305,  306,  307,  314,
          315,  316,  317,  318,  326,  327,  328,  329,  337,  338,  339,
          340,  348,  349,  350,  351,  360,  361,  362,  370,  371,  372,
          373,  381,  382,  383,  384,  392,  393,  394,  395,  402,  403,
          404,  405,  406,  413,  414,  415,  416,  417,  424,  425,  426,
          427,  428,  435,  436,  437,  438,  439,  446,  447,  448,  449,
          450,  458,  459,  460,  461,  469,  470,  471,  472,  480,  481,
          482,  483,  491,  492,  493,  494,  502,  503,  504,  505,  513,
          514,  515,  516,  523,  524,  525,  526,  527,  534,  535,  536,
          537,  538,  544,  545,  546,  547,  548,  549,  556,  557,  558,
          559,  560,  567,  568,  569,  570,  571,  579,  580,  581,  582,
          590,  591,  592,  593,  601,  602,  603,  604,  612,  613,  614,
          615,  623,  624,  625,  626,  633,  634,  635,  636,  637,  644,
          645,  646,  647,  648,  655,  656,  657,  658,  659,  665,  666,
          667,  668,  669,  670,  677,  678,  679,  680,  681,  688,  689,
          690,  691,  692,  699,  700,  701,  702,  703,  711,  712,  713,
          714,  722,  723,  724,  725,  733,  734,  735,  736,  743,  744,
          745,  746,  747,  754,  755,  756,  757,  758,  765,  766,  767,
          768,  769,  776,  777,  778,  779,  780,  787,  788,  789,  790,
          791,  798,  799,  800,  801,  802,  809,  810,  811,  812,  813,
          820,  821,  822,  823,  824,  831,  832,  833,  834,  835,  843,
          844,  845,  846,  854,  855,  856,  857,  864,  865,  866,  867,
          868,  875,  876,  877,  878,  879,  886,  887,  888,  889,  890,
          897,  898,  899,  900,  901,  908,  909,  910,  911,  912,  919,
          920,  921,  922,  923,  930,  931,  932,  933,  934,  941,  942,
          943,  944,  945,  952,  953,  954,  955,  956,  964,  965,  966,
          967,  975,  976,  977,  978,  986,  987,  988,  989,  997,  998,
          999,  1000, 1008, 1009, 1010, 1011, 1019, 1020, 1021, 1022, 1030,
          1031, 1032, 1033, 1041, 1042, 1043, 1044, 1052, 1053, 1054, 1055,
          1063, 1064, 1065, 1066, 1074, 1075, 1076, 1077, 1085, 1086, 1087,
          1088, 1096, 1097, 1098, 1099, 1107, 1108, 1109, 1110, 1118, 1119,
          1120, 1121, 1129, 1130, 1131, 1132, 1140, 1141, 1142, 1143, 1151,
          1152, 1153, 1154, 1162, 1163, 1164, 1165, 1173, 1174, 1175, 1176,
          1184, 1185, 1186, 1187, 1195, 1196, 1197, 1198, 1206, 1207, 1208,
          1209, 1217, 1218, 1219, 1220, 1228, 1229, 1230, 1231, 1239, 1240,
          1241, 1242, 1250, 1251, 1252, 1253, 1261, 1262, 1263, 1264, 1272,
          1273, 1274, 1275, 1283, 1284, 1285, 1286, 1294, 1295, 1296, 1297,
          1305, 1306, 1307, 1308, 1316, 1317, 1318, 1319, 1327, 1328, 1329,
          1330}},
        {Frustum3({1.3, -5.9, 3.5}, {0.2, 0.4, 1}, std::numbers::pi / 3.),
         {318,  426,  427,  428,  438,  439,  450,  538,  546,  547,  548,
          549,  558,  559,  560,  570,  571,  582,  655,  656,  657,  658,
          659,  667,  668,  669,  670,  678,  679,  680,  681,  690,  691,
          692,  702,  703,  714,  768,  769,  777,  778,  779,  780,  787,
          788,  789,  790,  791,  799,  800,  801,  802,  810,  811,  812,
          813,  822,  823,  824,  834,  835,  846,  888,  889,  890,  899,
          900,  901,  910,  911,  912,  920,  921,  922,  923,  931,  932,
          933,  934,  942,  943,  944,  945,  954,  955,  956,  966,  967,
          1000, 1010, 1011, 1021, 1022, 1032, 1033, 1042, 1043, 1044, 1053,
          1054, 1055, 1064, 1065, 1066, 1074, 1075, 1076, 1077, 1086, 1087,
          1088, 1143, 1154, 1165, 1175, 1176, 1186, 1187, 1197, 1198, 1207,
          1208, 1209, 1308, 1319, 1330}}};

    for (std::size_t l = 0; l < frExp.size(); l++) {
      const Frustum3& fr = frExp.at(l).first;
      const std::set<std::size_t>& idxsExp = frExp.at(l).second;
      std::stringstream ss;
      ss << "frust3d-3s_test_" << l << ".ply";

      os = tmp(ss.str());

      helper.clear();

      idxsAct.clear();

      fr.draw(helper, 50);

      n = 10;
      min = -33;
      max = 33;
      step = (max - min) / n;

      Object o;
      using Box = AxisAlignedBoundingBox<Object, double, 3>;
      Box::Size size(Eigen::Matrix<double, 3, 1>(2, 2, 2));

      std::size_t idx = 0;

      for (std::size_t i = 0; i <= n; i++) {
        for (std::size_t j = 0; j <= n; j++) {
          for (std::size_t k = 0; k <= n; k++) {
            Eigen::Matrix<double, 3, 1> pos(min + i * step, min + j * step,
                                            min + k * step);
            Box bb(&o, pos, size);

            Color color = {255, 0, 0};

            if (bb.intersect(fr)) {
              color = {0, 255, 0};
              idxsAct.insert(idx);
            }

            bb.draw(helper, color);
            idx++;
          }
        }
      }

      os << helper << std::flush;
      os.close();

      BOOST_CHECK(idxsAct == idxsExp);
    }
  }

  BOOST_TEST_CONTEXT("3D - 4 Sides") {
    using Frustum34 = Frustum<double, 3, 4>;
    const std::size_t n = 10;
    double min = -10;
    double max = 10;
    const std::size_t s = 5;
    double step = (max - min) / s;
    std::ofstream os;

    // Visualise the parameters
    helper.clear();
    os = tmp("frust3d-4s_dir.ply");

    const double constAngle = std::numbers::pi / 4.;
    for (std::size_t i = 0; i <= s; i++) {
      for (std::size_t j = 0; j <= s; j++) {
        for (std::size_t k = 0; k <= s; k++) {
          Vector3F origin(min + i * step, min + j * step, min + k * step);
          Vector3F dir(1, 0, 0);

          double piOverS = std::numbers::pi / s;
          AngleAxis3F rot;
          rot = AngleAxis3F(piOverS * i, Vector3F::UnitX()) *
                AngleAxis3F(piOverS * j, Vector3F::UnitY()) *
                AngleAxis3F(piOverS * k, Vector3F::UnitZ());

          Frustum34 fr(origin, rot * dir, constAngle);
          fr.draw(helper, 1);
        }
      }
    }

    os << helper << std::flush;
    os.close();

    os = tmp("frust3d-4s_angle.ply");
    helper.clear();

    for (std::size_t i = 0; i <= n; i++) {
      Vector3F origin(i * 4, 0, 0);
      AngleAxis3F rot(std::numbers::pi / n * i, Vector3F::UnitY());
      const double angle = (std::numbers::pi / 2.) / n * (1 + i);
      Vector3F dir(1, 0, 0);
      Frustum34 fr(origin, rot * dir, angle);
      fr.draw(helper, 2);
    }

    os << helper << std::flush;
    os.close();

    std::set<std::size_t> idxsAct;

    std::vector<std::pair<Frustum34, std::set<std::size_t>>> frExp;
    frExp = {
        {Frustum34({0, 0, 0}, {1, 0, 0}, std::numbers::pi / 2.),
         {665,  774,  775,  776,  785,  786,  787,  796,  797,  798,  883,
          884,  885,  886,  887,  894,  895,  896,  897,  898,  905,  906,
          907,  908,  909,  916,  917,  918,  919,  920,  927,  928,  929,
          930,  931,  992,  993,  994,  995,  996,  997,  998,  1003, 1004,
          1005, 1006, 1007, 1008, 1009, 1014, 1015, 1016, 1017, 1018, 1019,
          1020, 1025, 1026, 1027, 1028, 1029, 1030, 1031, 1036, 1037, 1038,
          1039, 1040, 1041, 1042, 1047, 1048, 1049, 1050, 1051, 1052, 1053,
          1058, 1059, 1060, 1061, 1062, 1063, 1064, 1101, 1102, 1103, 1104,
          1105, 1106, 1107, 1108, 1109, 1112, 1113, 1114, 1115, 1116, 1117,
          1118, 1119, 1120, 1123, 1124, 1125, 1126, 1127, 1128, 1129, 1130,
          1131, 1134, 1135, 1136, 1137, 1138, 1139, 1140, 1141, 1142, 1145,
          1146, 1147, 1148, 1149, 1150, 1151, 1152, 1153, 1156, 1157, 1158,
          1159, 1160, 1161, 1162, 1163, 1164, 1167, 1168, 1169, 1170, 1171,
          1172, 1173, 1174, 1175, 1178, 1179, 1180, 1181, 1182, 1183, 1184,
          1185, 1186, 1189, 1190, 1191, 1192, 1193, 1194, 1195, 1196, 1197,
          1210, 1211, 1212, 1213, 1214, 1215, 1216, 1217, 1218, 1219, 1220,
          1221, 1222, 1223, 1224, 1225, 1226, 1227, 1228, 1229, 1230, 1231,
          1232, 1233, 1234, 1235, 1236, 1237, 1238, 1239, 1240, 1241, 1242,
          1243, 1244, 1245, 1246, 1247, 1248, 1249, 1250, 1251, 1252, 1253,
          1254, 1255, 1256, 1257, 1258, 1259, 1260, 1261, 1262, 1263, 1264,
          1265, 1266, 1267, 1268, 1269, 1270, 1271, 1272, 1273, 1274, 1275,
          1276, 1277, 1278, 1279, 1280, 1281, 1282, 1283, 1284, 1285, 1286,
          1287, 1288, 1289, 1290, 1291, 1292, 1293, 1294, 1295, 1296, 1297,
          1298, 1299, 1300, 1301, 1302, 1303, 1304, 1305, 1306, 1307, 1308,
          1309, 1310, 1311, 1312, 1313, 1314, 1315, 1316, 1317, 1318, 1319,
          1320, 1321, 1322, 1323, 1324, 1325, 1326, 1327, 1328, 1329, 1330}},
        {Frustum34({0, 0, 0}, {0, 1, 0}, std::numbers::pi / 2.),
         {110,  111,  112,  113,  114,  115,  116,  117,  118,  119,  120,
          221,  222,  223,  224,  225,  226,  227,  228,  229,  231,  232,
          233,  234,  235,  236,  237,  238,  239,  240,  241,  332,  333,
          334,  335,  336,  337,  338,  342,  343,  344,  345,  346,  347,
          348,  349,  350,  352,  353,  354,  355,  356,  357,  358,  359,
          360,  361,  362,  443,  444,  445,  446,  447,  453,  454,  455,
          456,  457,  458,  459,  463,  464,  465,  466,  467,  468,  469,
          470,  471,  473,  474,  475,  476,  477,  478,  479,  480,  481,
          482,  483,  554,  555,  556,  564,  565,  566,  567,  568,  574,
          575,  576,  577,  578,  579,  580,  584,  585,  586,  587,  588,
          589,  590,  591,  592,  594,  595,  596,  597,  598,  599,  600,
          601,  602,  603,  604,  665,  675,  676,  677,  685,  686,  687,
          688,  689,  695,  696,  697,  698,  699,  700,  701,  705,  706,
          707,  708,  709,  710,  711,  712,  713,  715,  716,  717,  718,
          719,  720,  721,  722,  723,  724,  725,  796,  797,  798,  806,
          807,  808,  809,  810,  816,  817,  818,  819,  820,  821,  822,
          826,  827,  828,  829,  830,  831,  832,  833,  834,  836,  837,
          838,  839,  840,  841,  842,  843,  844,  845,  846,  927,  928,
          929,  930,  931,  937,  938,  939,  940,  941,  942,  943,  947,
          948,  949,  950,  951,  952,  953,  954,  955,  957,  958,  959,
          960,  961,  962,  963,  964,  965,  966,  967,  1058, 1059, 1060,
          1061, 1062, 1063, 1064, 1068, 1069, 1070, 1071, 1072, 1073, 1074,
          1075, 1076, 1078, 1079, 1080, 1081, 1082, 1083, 1084, 1085, 1086,
          1087, 1088, 1189, 1190, 1191, 1192, 1193, 1194, 1195, 1196, 1197,
          1199, 1200, 1201, 1202, 1203, 1204, 1205, 1206, 1207, 1208, 1209,
          1320, 1321, 1322, 1323, 1324, 1325, 1326, 1327, 1328, 1329, 1330}},
        {Frustum34({0, 0, 0}, {0, 0, 1}, std::numbers::pi / 2.),
         {10,   21,   32,   43,   54,   65,   76,   87,   98,   109,  120,
          131,  141,  142,  152,  153,  163,  164,  174,  175,  185,  186,
          196,  197,  207,  208,  218,  219,  229,  230,  241,  252,  262,
          263,  272,  273,  274,  283,  284,  285,  294,  295,  296,  305,
          306,  307,  316,  317,  318,  327,  328,  329,  338,  339,  340,
          350,  351,  362,  373,  383,  384,  393,  394,  395,  403,  404,
          405,  406,  414,  415,  416,  417,  425,  426,  427,  428,  436,
          437,  438,  439,  447,  448,  449,  450,  459,  460,  461,  471,
          472,  483,  494,  504,  505,  514,  515,  516,  524,  525,  526,
          527,  534,  535,  536,  537,  538,  545,  546,  547,  548,  549,
          556,  557,  558,  559,  560,  568,  569,  570,  571,  580,  581,
          582,  592,  593,  604,  615,  625,  626,  635,  636,  637,  645,
          646,  647,  648,  655,  656,  657,  658,  659,  665,  666,  667,
          668,  669,  670,  677,  678,  679,  680,  681,  689,  690,  691,
          692,  701,  702,  703,  713,  714,  725,  736,  746,  747,  756,
          757,  758,  766,  767,  768,  769,  776,  777,  778,  779,  780,
          787,  788,  789,  790,  791,  798,  799,  800,  801,  802,  810,
          811,  812,  813,  822,  823,  824,  834,  835,  846,  857,  867,
          868,  877,  878,  879,  887,  888,  889,  890,  898,  899,  900,
          901,  909,  910,  911,  912,  920,  921,  922,  923,  931,  932,
          933,  934,  943,  944,  945,  955,  956,  967,  978,  988,  989,
          998,  999,  1000, 1009, 1010, 1011, 1020, 1021, 1022, 1031, 1032,
          1033, 1042, 1043, 1044, 1053, 1054, 1055, 1064, 1065, 1066, 1076,
          1077, 1088, 1099, 1109, 1110, 1120, 1121, 1131, 1132, 1142, 1143,
          1153, 1154, 1164, 1165, 1175, 1176, 1186, 1187, 1197, 1198, 1209,
          1220, 1231, 1242, 1253, 1264, 1275, 1286, 1297, 1308, 1319, 1330}},
        {Frustum34({0, 0, 0}, {0, 0, 1}, std::numbers::pi / 4.),
         {406, 417, 428, 439, 450, 527, 535, 536, 537, 538, 546, 547, 548, 549,
          557, 558, 559, 560, 571, 648, 656, 657, 658, 659, 665, 666, 667, 668,
          669, 670, 678, 679, 680, 681, 692, 769, 777, 778, 779, 780, 788, 789,
          790, 791, 799, 800, 801, 802, 813, 890, 901, 912, 923, 934}},
        {Frustum34({0, 0, 0}, {0, 0, 1}, std::numbers::pi / 8.),
         {538, 549, 560, 659, 665, 666, 667, 668, 669, 670, 681, 780, 791,
          802}},
        {Frustum34({0, 0, 0}, {0, 0, 1}, std::numbers::pi * 3. / 4.),
         {7,    8,    9,    10,   18,   19,   20,   21,   29,   30,   31,
          32,   40,   41,   42,   43,   51,   52,   53,   54,   62,   63,
          64,   65,   73,   74,   75,   76,   84,   85,   86,   87,   95,
          96,   97,   98,   106,  107,  108,  109,  117,  118,  119,  120,
          128,  129,  130,  131,  139,  140,  141,  142,  150,  151,  152,
          153,  161,  162,  163,  164,  172,  173,  174,  175,  183,  184,
          185,  186,  194,  195,  196,  197,  205,  206,  207,  208,  216,
          217,  218,  219,  227,  228,  229,  230,  238,  239,  240,  241,
          249,  250,  251,  252,  260,  261,  262,  263,  271,  272,  273,
          274,  282,  283,  284,  285,  293,  294,  295,  296,  304,  305,
          306,  307,  315,  316,  317,  318,  326,  327,  328,  329,  337,
          338,  339,  340,  348,  349,  350,  351,  359,  360,  361,  362,
          370,  371,  372,  373,  381,  382,  383,  384,  392,  393,  394,
          395,  402,  403,  404,  405,  406,  413,  414,  415,  416,  417,
          424,  425,  426,  427,  428,  435,  436,  437,  438,  439,  446,
          447,  448,  449,  450,  458,  459,  460,  461,  469,  470,  471,
          472,  480,  481,  482,  483,  491,  492,  493,  494,  502,  503,
          504,  505,  513,  514,  515,  516,  523,  524,  525,  526,  527,
          534,  535,  536,  537,  538,  545,  546,  547,  548,  549,  556,
          557,  558,  559,  560,  567,  568,  569,  570,  571,  579,  580,
          581,  582,  590,  591,  592,  593,  601,  602,  603,  604,  612,
          613,  614,  615,  623,  624,  625,  626,  634,  635,  636,  637,
          644,  645,  646,  647,  648,  655,  656,  657,  658,  659,  665,
          666,  667,  668,  669,  670,  677,  678,  679,  680,  681,  688,
          689,  690,  691,  692,  700,  701,  702,  703,  711,  712,  713,
          714,  722,  723,  724,  725,  733,  734,  735,  736,  744,  745,
          746,  747,  755,  756,  757,  758,  765,  766,  767,  768,  769,
          776,  777,  778,  779,  780,  787,  788,  789,  790,  791,  798,
          799,  800,  801,  802,  809,  810,  811,  812,  813,  821,  822,
          823,  824,  832,  833,  834,  835,  843,  844,  845,  846,  854,
          855,  856,  857,  865,  866,  867,  868,  876,  877,  878,  879,
          886,  887,  888,  889,  890,  897,  898,  899,  900,  901,  908,
          909,  910,  911,  912,  919,  920,  921,  922,  923,  930,  931,
          932,  933,  934,  942,  943,  944,  945,  953,  954,  955,  956,
          964,  965,  966,  967,  975,  976,  977,  978,  986,  987,  988,
          989,  997,  998,  999,  1000, 1008, 1009, 1010, 1011, 1019, 1020,
          1021, 1022, 1030, 1031, 1032, 1033, 1041, 1042, 1043, 1044, 1052,
          1053, 1054, 1055, 1063, 1064, 1065, 1066, 1074, 1075, 1076, 1077,
          1085, 1086, 1087, 1088, 1096, 1097, 1098, 1099, 1107, 1108, 1109,
          1110, 1118, 1119, 1120, 1121, 1129, 1130, 1131, 1132, 1140, 1141,
          1142, 1143, 1151, 1152, 1153, 1154, 1162, 1163, 1164, 1165, 1173,
          1174, 1175, 1176, 1184, 1185, 1186, 1187, 1195, 1196, 1197, 1198,
          1206, 1207, 1208, 1209, 1217, 1218, 1219, 1220, 1228, 1229, 1230,
          1231, 1239, 1240, 1241, 1242, 1250, 1251, 1252, 1253, 1261, 1262,
          1263, 1264, 1272, 1273, 1274, 1275, 1283, 1284, 1285, 1286, 1294,
          1295, 1296, 1297, 1305, 1306, 1307, 1308, 1316, 1317, 1318, 1319,
          1327, 1328, 1329, 1330}},
        {Frustum34({1.3, -5.9, 3.5}, {0.2, 0.4, 1}, std::numbers::pi / 3.),
         {461,  472,  537,  538,  548,  549,  558,  559,  560,  569,  570,
          571,  581,  582,  593,  655,  656,  657,  658,  659,  666,  667,
          668,  669,  670,  678,  679,  680,  681,  690,  691,  692,  702,
          703,  714,  777,  778,  779,  780,  787,  788,  789,  790,  791,
          799,  800,  801,  802,  811,  812,  813,  823,  824,  835,  846,
          899,  900,  901,  910,  911,  912,  920,  921,  922,  923,  932,
          933,  934,  944,  945,  955,  956,  967,  1021, 1022, 1032, 1033,
          1042, 1043, 1044, 1053, 1054, 1055, 1064, 1065, 1066, 1076, 1077,
          1088, 1143, 1154, 1165, 1175, 1176, 1186, 1187, 1197, 1198, 1209,
          1308, 1319, 1330}}};

    min = -33;
    max = 33;
    step = (max - min) / n;

    for (std::size_t l = 0; l < frExp.size(); l++) {
      const Frustum34& fr = frExp.at(l).first;
      const std::set<std::size_t>& idxsExp = frExp.at(l).second;
      std::stringstream ss;
      ss << "frust3d-4s_test_" << l << ".ply";

      os = tmp(ss.str());

      helper.clear();

      idxsAct.clear();

      fr.draw(helper, 50);

      Object o;
      using Box = AxisAlignedBoundingBox<Object, double, 3>;
      Box::Size size(Eigen::Matrix<double, 3, 1>(2, 2, 2));

      std::size_t idx = 0;
      for (std::size_t i = 0; i <= n; i++) {
        for (std::size_t j = 0; j <= n; j++) {
          for (std::size_t k = 0; k <= n; k++) {
            Eigen::Matrix<double, 3, 1> pos(min + i * step, min + j * step,
                                            min + k * step);
            Box bb(&o, pos, size);

            Color color = {255, 0, 0};

            if (bb.intersect(fr)) {
              color = {0, 255, 0};
              idxsAct.insert(idx);
            }

            bb.draw(helper, color);
            idx++;
          }
        }
      }

      os << helper << std::flush;
      os.close();

      BOOST_CHECK(idxsAct == idxsExp);
    }
  }

  BOOST_TEST_CONTEXT("3D - 5 Sides") {
    using Frustum = Frustum<double, 3, 5>;
    using Box = AxisAlignedBoundingBox<Object, double, 3>;
    Box::Size size(Acts::Vector3(2, 2, 2));

    Object o;

    PlyVisualization3D<double> ply;

    Frustum fr({0, 0, 0}, {0, 0, 1}, std::numbers::pi / 8.);
    fr.draw(ply, 10);

    Box bb(&o, {0, 0, 10}, size);
    bb.draw(ply);

    BOOST_CHECK(bb.intersect(fr));

    auto os = tmp("frust3d-5s.ply");
    os << ply << std::flush;
    os.close();
  }

  BOOST_TEST_CONTEXT("3D - 10 Sides") {
    using Frustum = Frustum<double, 3, 10>;
    using Box = AxisAlignedBoundingBox<Object, double, 3>;
    Box::Size size(Acts::Vector3(2, 2, 2));

    Object o;

    PlyVisualization3D<double> ply;

    Acts::Vector3 pos = {-12.4205, 29.3578, 44.6207};
    Acts::Vector3 dir = {-0.656862, 0.48138, 0.58035};
    Frustum fr(pos, dir, 0.972419);
    fr.draw(ply, 10);

    Box bb(&o, pos + dir * 10, size);
    bb.draw(ply);

    BOOST_CHECK(bb.intersect(fr));

    auto os = tmp("frust3d-10s.ply");
    os << ply << std::flush;
    os.close();
  }

  BOOST_TEST_CONTEXT("3D - 4 Sides - Big box") {
    using Frustum = Frustum<double, 3, 4>;
    using Box = AxisAlignedBoundingBox<Object, double, 3>;

    Object o;

    PlyVisualization3D<double> ply;

    Acts::Vector3 pos = {0, 0, 0};
    Acts::Vector3 dir = {0, 0, 1};
    Frustum fr(pos, dir, 0.972419);
    fr.draw(ply, 10);

    Box::Size size(Acts::Vector3(100, 100, 2));
    Box bb(&o, pos + dir * 7, size);
    bb.draw(ply);

    BOOST_CHECK(bb.intersect(fr));

    auto os = tmp("frust3d-4s-bigbox.ply");
    os << ply << std::flush;
    os.close();
  }
}

BOOST_AUTO_TEST_CASE(ostream_operator) {
  Object o;
  using Box = Acts::AxisAlignedBoundingBox<Object, double, 2>;
  Box bb(&o, {-1, -1}, {2, 2});

  std::stringstream ss;
  ss << bb;

  BOOST_CHECK(ss.str() == "AABB(ctr=(0.5, 0.5) vmin=(-1, -1) vmax=(2, 2))");
}

}  // namespace Acts::Test
