// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Tests/CommonHelpers/BenchmarkTools.hpp"
#include "Acts/Utilities/BoundingBox.hpp"
#include "Acts/Utilities/Frustum.hpp"
#include "Acts/Utilities/Ray.hpp"

#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <map>
#include <random>
#include <vector>

using namespace Acts;

struct O {};
using Box = Acts::AxisAlignedBoundingBox<O, float, 3>;
using VertexType = Box::VertexType;
using vertex_array_type = Box::vertex_array_type;
using value_type = Box::value_type;
using Frustum3 = Frustum<float, 3, 4>;
using Ray3 = Ray<float, 3>;

int main(int /*argc*/, char** /*argv[]*/) {
  std::size_t n = 1000;

  std::mt19937 rng(42);
  std::uniform_real_distribution<float> dir(0, 1);
  std::uniform_real_distribution<float> loc(-10, 10);
  std::uniform_real_distribution<float> ang(M_PI / 10., M_PI / 4.);

  Box testBox{nullptr, {0, 0, 0}, Box::Size{{1, 2, 3}}};

  std::cout << "\n==== RAY ====\n" << std::endl;

  std::map<std::string, bool (*)(const Box&, const Ray3&)> rayVariants;

  rayVariants["Nominal"] = [](const auto& box, const auto& ray) -> bool {
    return box.intersect(ray);
  };

  rayVariants["Incl. div., unroll"] = [](const Box& box,
                                         const Ray<float, 3>& ray) {
    const VertexType& origin = ray.origin();

    const vertex_array_type& d = ray.dir();

    double tmin = -INFINITY, tmax = INFINITY;
    if (d.x() != 0.0) {
      double tx1 = (box.min().x() - origin.x()) / d.x();
      double tx2 = (box.max().x() - origin.x()) / d.x();

      tmin = std::max(tmin, std::min(tx1, tx2));
      tmax = std::min(tmax, std::max(tx1, tx2));
    }

    if (d.y() != 0.0) {
      double ty1 = (box.min().y() - origin.y()) / d.y();
      double ty2 = (box.max().y() - origin.y()) / d.y();

      tmin = std::max(tmin, std::min(ty1, ty2));
      tmax = std::min(tmax, std::max(ty1, ty2));
    }

    if (d.z() != 0.0) {
      double tz1 = (box.min().z() - origin.z()) / d.z();
      double tz2 = (box.max().z() - origin.z()) / d.z();

      tmin = std::max(tmin, std::min(tz1, tz2));
      tmax = std::min(tmax, std::max(tz1, tz2));
    }

    return tmax > tmin && tmax > 0.0;
  };

  rayVariants["Incl. div., loop"] = [](const Box& box,
                                       const Ray<float, 3>& ray) {
    const VertexType& origin = ray.origin();

    const vertex_array_type& d = ray.dir();

    double tmin = -INFINITY, tmax = INFINITY;

    for (std::size_t i = 0; i < 3; i++) {
      if (d[i] == 0.0) {
        continue;
      }
      double t1 = (box.min()[i] - origin[i]) / d[i];
      double t2 = (box.max()[i] - origin[i]) / d[i];
      tmin = std::max(tmin, std::min(t1, t2));
      tmax = std::min(tmax, std::max(t1, t2));
    }

    return tmax > tmin && tmax > 0.0;
  };

  rayVariants["Incl. div., min/max alt., unroll"] =
      [](const Box& box, const Ray<float, 3>& ray) {
        const VertexType& origin = ray.origin();
        const vertex_array_type& d = ray.dir();

        double tx1 = (box.min().x() - origin.x()) / d.x();
        double tx2 = (box.max().x() - origin.x()) / d.x();
        double tmin = std::min(tx1, tx2);
        double tmax = std::max(tx1, tx2);

        double ty1 = (box.min().y() - origin.y()) / d.y();
        double ty2 = (box.max().y() - origin.y()) / d.y();
        tmin = std::max(tmin, std::min(ty1, ty2));
        tmax = std::min(tmax, std::max(ty1, ty2));

        double tz1 = (box.min().z() - origin.z()) / d.z();
        double tz2 = (box.max().z() - origin.z()) / d.z();
        tmin = std::max(tmin, std::min(tz1, tz2));
        tmax = std::min(tmax, std::max(tz1, tz2));

        return tmax > tmin && tmax > 0.0;
      };

  rayVariants["No div., min/max alt, unroll"] = [](const Box& box,
                                                   const Ray<float, 3>& ray) {
    const VertexType& origin = ray.origin();
    const vertex_array_type& id = ray.idir();

    double tx1 = (box.min().x() - origin.x()) * id.x();
    double tx2 = (box.max().x() - origin.x()) * id.x();
    double tmin = std::min(tx1, tx2);
    double tmax = std::max(tx1, tx2);

    double ty1 = (box.min().y() - origin.y()) * id.y();
    double ty2 = (box.max().y() - origin.y()) * id.y();
    tmin = std::max(tmin, std::min(ty1, ty2));
    tmax = std::min(tmax, std::max(ty1, ty2));

    double tz1 = (box.min().z() - origin.z()) * id.z();
    double tz2 = (box.max().z() - origin.z()) * id.z();
    tmin = std::max(tmin, std::min(tz1, tz2));
    tmax = std::min(tmax, std::max(tz1, tz2));

    return tmax > tmin && tmax > 0.0;
  };

  rayVariants["No div., min/max orig, loop"] = [](const Box& box,
                                                  const Ray<float, 3>& ray) {
    const VertexType& origin = ray.origin();
    const vertex_array_type& id = ray.idir();
    double tmin = -INFINITY, tmax = INFINITY;

    for (std::size_t i = 0; i < 3; i++) {
      double t1 = (box.min()[i] - origin[i]) * id[i];
      double t2 = (box.max()[i] - origin[i]) * id[i];
      tmin = std::max(tmin, std::min(t1, t2));
      tmax = std::min(tmax, std::max(t1, t2));
    }

    return tmax > tmin && tmax > 0.0;
  };

  using Vector3F = Eigen::Matrix<float, 3, 1>;

  std::vector<Ray3> rays{n, Ray3{Vector3F{0, 0, 0}, Vector3F{1, 0, 0}}};
  std::generate(rays.begin(), rays.end(), [&]() {
    const Vector3F d{dir(rng), dir(rng), dir(rng)};
    const Vector3F l{loc(rng), loc(rng), loc(rng)};
    return Ray3{l, d.normalized()};
  });

  std::cout << "Make sure ray implementations are identical" << std::endl;
  for (const auto& ray : rays) {
    std::vector<std::pair<std::string, bool>> results;

    std::transform(rayVariants.begin(), rayVariants.end(),
                   std::back_inserter(results),
                   [&](const auto& p) -> decltype(results)::value_type {
                     const auto& [name, func] = p;
                     return {name, func(testBox, ray)};
                   });

    bool all = std::all_of(results.begin(), results.end(),
                           [](const auto& r) { return r.second; });
    bool none = std::none_of(results.begin(), results.end(),
                             [](const auto& r) { return r.second; });

    if (!all && !none) {
      std::cerr << "Discrepancy: " << std::endl;
      for (const auto& [name, result] : results) {
        std::cerr << " - " << name << ": " << result << std::endl;
      }

      testBox.toStream(std::cerr);
      std::cerr << std::endl;
      std::cerr << "Ray: [" << ray.origin().transpose() << "], ["
                << ray.dir().transpose() << "]" << std::endl;
      return -1;
    }
  }
  std::cout << "Seems ok" << std::endl;

  std::cout << "Run benchmarks: " << std::endl;
  for (const auto& p : rayVariants) {
    // can't capture structured binding, so pair access it is.
    std::cout << "- Benchmarking variant: '" << p.first << "'" << std::endl;
    auto bench_result = Acts::Test::microBenchmark(
        [&](const auto& ray) { return p.second(testBox, ray); }, rays);
    std::cout << "  " << bench_result << std::endl;
  }

  std::cout << "\n==== FRUSTUM ====\n" << std::endl;

  std::map<std::string, bool (*)(const Box&, const Frustum3&)> frustumVariants;

  frustumVariants["Nominal"] = [](const auto& box,
                                  const auto& frustum) -> bool {
    return box.intersect(frustum);
  };

  frustumVariants["Manual constexpr loop unroll, early ret."] =
      [](const Box& box, const Frustum3& fr) {
        constexpr std::size_t sides = 4;  // yes this is pointless, I just want
                                          // to kind of match the other impl

        const auto& normals = fr.normals();
        const vertex_array_type fr_vmin = box.min() - fr.origin();
        const vertex_array_type fr_vmax = box.max() - fr.origin();

        auto calc = [&](const auto& normal) {
          return (normal.array() < 0).template cast<value_type>() * fr_vmin +
                 (normal.array() >= 0).template cast<value_type>() * fr_vmax;
        };

        VertexType p_vtx;

        p_vtx = calc(normals[0]);
        if (p_vtx.dot(normals[0]) < 0) {
          return false;
        }

        p_vtx = calc(normals[1]);
        if (p_vtx.dot(normals[1]) < 0) {
          return false;
        }

        p_vtx = calc(normals[2]);
        if (p_vtx.dot(normals[2]) < 0) {
          return false;
        }

        if constexpr (sides > 2) {
          p_vtx = calc(normals[3]);
          if (p_vtx.dot(normals[3]) < 0) {
            return false;
          }
        }

        if constexpr (sides > 3) {
          p_vtx = calc(normals[4]);
          if (p_vtx.dot(normals[4]) < 0) {
            return false;
          }
        }

        if constexpr (sides > 4) {
          for (std::size_t i = 5; i <= fr.sides; i++) {
            const VertexType& normal = normals[i];

            p_vtx = calc(normal);
            if (p_vtx.dot(normal) < 0) {
              return false;
            }
          }
        }

        return true;
      };

  frustumVariants["Nominal, no early ret."] = [](const Box& box,
                                                 const Frustum3& fr) {
    const auto& normals = fr.normals();
    const vertex_array_type fr_vmin = box.min() - fr.origin();
    const vertex_array_type fr_vmax = box.max() - fr.origin();

    VertexType p_vtx;
    bool result = true;
    for (std::size_t i = 0; i < fr.sides + 1; i++) {
      const VertexType& normal = normals[i];

      p_vtx = (normal.array() < 0).template cast<value_type>() * fr_vmin +
              (normal.array() >= 0).template cast<value_type>() * fr_vmax;

      result = result && (p_vtx.dot(normal) >= 0);
    }
    return result;
  };

  frustumVariants["Manual constexpr unroll, early ret."] =
      [](const Box& box, const Frustum3& fr) {
        constexpr std::size_t sides = 4;  // yes this is pointless, I just want
                                          // to kind of match the other impl

        const auto& normals = fr.normals();
        const vertex_array_type fr_vmin = box.min() - fr.origin();
        const vertex_array_type fr_vmax = box.max() - fr.origin();

        auto calc = [&](const auto& normal) {
          return (normal.array() < 0).template cast<value_type>() * fr_vmin +
                 (normal.array() >= 0).template cast<value_type>() * fr_vmax;
        };

        VertexType p_vtx;
        bool result = true;

        p_vtx = calc(normals[0]);
        result = result && (p_vtx.dot(normals[0]) >= 0);

        p_vtx = calc(normals[1]);
        result = result && (p_vtx.dot(normals[1]) >= 0);

        p_vtx = calc(normals[2]);
        result = result && (p_vtx.dot(normals[2]) >= 0);

        if constexpr (sides > 2) {
          p_vtx = calc(normals[3]);
          result = result && (p_vtx.dot(normals[3]) >= 0);
        }

        if constexpr (sides > 3) {
          p_vtx = calc(normals[4]);
          result = result && (p_vtx.dot(normals[4]) >= 0);
        }

        if constexpr (sides > 4) {
          for (std::size_t i = 5; i <= fr.sides; i++) {
            const VertexType& normal = normals[i];

            p_vtx = calc(normal);
            result = result && (p_vtx.dot(normal) >= 0);
          }
        }

        return result;
      };

  std::vector<Frustum3> frustums{n, Frustum3{{0, 0, 0}, {1, 0, 0}, M_PI / 2.}};
  std::generate(frustums.begin(), frustums.end(), [&]() {
    const Vector3F d{dir(rng), dir(rng), dir(rng)};
    const Vector3F l{loc(rng), loc(rng), loc(rng)};
    return Frustum3{l, d.normalized(), ang(rng)};
  });

  std::cout << "Make sure frustum implementations are identical" << std::endl;
  for (const auto& fr : frustums) {
    std::vector<std::pair<std::string, bool>> results;

    std::transform(frustumVariants.begin(), frustumVariants.end(),
                   std::back_inserter(results),
                   [&](const auto& p) -> decltype(results)::value_type {
                     const auto& [name, func] = p;
                     return {name, func(testBox, fr)};
                   });

    bool all = std::all_of(results.begin(), results.end(),
                           [](const auto& r) { return r.second; });
    bool none = std::none_of(results.begin(), results.end(),
                             [](const auto& r) { return r.second; });

    if (!all && !none) {
      std::cerr << "Discrepancy: " << std::endl;
      for (const auto& [name, result] : results) {
        std::cerr << " - " << name << ": " << result << std::endl;
      }

      testBox.toStream(std::cerr);
      std::cerr << std::endl;
      std::cerr << "Frustum: [" << fr.origin().transpose() << "], ["
                << fr.dir().transpose() << "]" << std::endl;
      return -1;
    }
  }
  std::cout << "Seems ok" << std::endl;

  std::size_t iters_per_run = 1000;

  std::vector<std::pair<std::string, Frustum3>> testFrusts = {
      {"away", Frustum3{{0, 0, -10}, {0, 0, -1}, M_PI / 4.}},
      {"towards", Frustum3{{0, 0, -10}, {0, 0, 1}, M_PI / 4.}},
      {"left", Frustum3{{0, 0, -10}, {0, 1, 0}, M_PI / 4.}},
      {"right", Frustum3{{0, 0, -10}, {0, -1, 0}, M_PI / 4.}},
      {"up", Frustum3{{0, 0, -10}, {1, 0, 0}, M_PI / 4.}},
      {"down", Frustum3{{0, 0, -10}, {-1, 0, 0}, M_PI / 4.}},
  };

  std::cout << "Run benchmarks: " << std::endl;

  for (const auto& fr_pair : testFrusts) {
    std::cout << "Frustum '" << fr_pair.first << "'" << std::endl;

    for (const auto& p : frustumVariants) {
      // can't capture structured binding, so pair access it is.
      std::cout << "- Benchmarking variant: '" << p.first << "'" << std::endl;
      auto bench_result = Acts::Test::microBenchmark(
          [&]() { return p.second(testBox, fr_pair.second); }, iters_per_run);
      std::cout << "  " << bench_result << std::endl;
    }

    std::cout << std::endl;
  }
  return 0;
}
