// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/TransformationHelpers.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Surfaces/CurvilinearSurface.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/PerigeeSurface.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/TrackFitting/detail/GsfComponentMerging.hpp"
#include "Acts/Utilities/Intersection.hpp"
#include "Acts/Utilities/Result.hpp"
#include "Acts/Utilities/detail/periodic.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <memory>
#include <random>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

#include <Eigen/Eigenvalues>

#define CHECK_CLOSE_MATRIX(a, b, t) \
  BOOST_CHECK(((a - b).array().abs() < t).all())

using namespace Acts;
using namespace Acts::UnitLiterals;

// Describes a component of a D-dimensional gaussian component
template <int D>
struct DummyComponent {
  Acts::ActsScalar weight = 0;
  Acts::ActsVector<D> boundPars;
  Acts::ActsSquareMatrix<D> boundCov;
};

// A Multivariate distribution object working in the same way as the
// distributions in the standard library
template <typename T, int D>
class MultivariateNormalDistribution {
 public:
  using Vector = Eigen::Matrix<T, D, 1>;
  using Matrix = Eigen::Matrix<T, D, D>;

 private:
  Vector m_mean;
  Matrix m_transform;

 public:
  MultivariateNormalDistribution(Vector const &mean, Matrix const &boundCov)
      : m_mean(mean) {
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> eigenSolver(boundCov);
    m_transform = eigenSolver.eigenvectors() *
                  eigenSolver.eigenvalues().cwiseSqrt().asDiagonal();
  }

  template <typename generator_t>
  Vector operator()(generator_t &gen) const {
    std::normal_distribution<T> normal;
    return m_mean +
           m_transform * Vector{}.unaryExpr([&](auto) { return normal(gen); });
  }
};

// Sample data from a multi-component multivariate distribution
template <int D>
auto sampleFromMultivariate(const std::vector<DummyComponent<D>> &cmps,
                            std::size_t n_samples, std::mt19937 &gen) {
  using MultiNormal = MultivariateNormalDistribution<double, D>;

  std::vector<MultiNormal> dists;
  std::vector<double> weights;
  for (const auto &cmp : cmps) {
    dists.push_back(MultiNormal(cmp.boundPars, cmp.boundCov));
    weights.push_back(cmp.weight);
  }

  std::discrete_distribution choice(weights.begin(), weights.end());

  auto sample = [&]() {
    const auto n = choice(gen);
    return dists[n](gen);
  };

  std::vector<ActsVector<D>> samples(n_samples);
  std::generate(samples.begin(), samples.end(), sample);

  return samples;
}

// Simple arithmetic mean computation
template <int D>
auto mean(const std::vector<ActsVector<D>> &samples) -> ActsVector<D> {
  ActsVector<D> mean = ActsVector<D>::Zero();

  for (const auto &x : samples) {
    mean += x;
  }

  return mean / samples.size();
}

// A method to compute the circular mean, since the normal arithmetic mean
// doesn't work for angles in general
template <int D>
auto circularMean(const std::vector<ActsVector<D>> &samples) -> ActsVector<D> {
  ActsVector<D> x = ActsVector<D>::Zero();
  ActsVector<D> y = ActsVector<D>::Zero();

  for (const auto &s : samples) {
    for (int i = 0; i < D; ++i) {
      x[i] += std::cos(s[i]);
      y[i] += std::sin(s[i]);
    }
  }

  ActsVector<D> mean = ActsVector<D>::Zero();

  for (int i = 0; i < D; ++i) {
    mean[i] = std::atan2(y[i], x[i]);
  }

  return mean;
}

// This general boundCovariance estimator can be equipped with a custom
// subtraction object to enable circular behaviour
template <int D, typename subtract_t = std::minus<ActsVector<D>>>
auto boundCov(const std::vector<ActsVector<D>> &samples,
              const ActsVector<D> &mu,
              const subtract_t &sub = subtract_t{}) -> ActsSquareMatrix<D> {
  ActsSquareMatrix<D> boundCov = ActsSquareMatrix<D>::Zero();

  for (const auto &smpl : samples) {
    boundCov += sub(smpl, mu) * sub(smpl, mu).transpose();
  }

  return boundCov / samples.size();
}

// This function computes the mean of a bound gaussian mixture by converting
// them to cartesian coordinates, computing the mean, and converting back to
// bound.
BoundVector meanFromFree(std::vector<DummyComponent<eBoundSize>> cmps,
                         const Surface &surface) {
  // Specially handle LOC0, since the free mean would not be on the surface
  // likely
  if (surface.type() == Surface::Cylinder) {
    auto x = 0.0, y = 0.0;
    const auto r = surface.bounds().values()[CylinderBounds::eR];

    for (const auto &cmp : cmps) {
      x += cmp.weight * std::cos(cmp.boundPars[eBoundLoc0] / r);
      y += cmp.weight * std::sin(cmp.boundPars[eBoundLoc0] / r);
    }

    for (auto &cmp : cmps) {
      cmp.boundPars[eBoundLoc0] = std::atan2(y, x) * r;
    }
  }

  if (surface.type() == Surface::Cone) {
    throw std::runtime_error("Cone surface not supported");
  }

  FreeVector mean = FreeVector::Zero();

  for (const auto &cmp : cmps) {
    mean += cmp.weight * transformBoundToFreeParameters(
                             surface, GeometryContext{}, cmp.boundPars);
  }

  mean.segment<3>(eFreeDir0).normalize();

  // Project the position on the surface.
  // This is mainly necessary for the perigee surface, where
  // the mean might not fulfill the perigee condition.
  Vector3 position = mean.head<3>();
  Vector3 direction = mean.segment<3>(eFreeDir0);
  auto intersection = surface
                          .intersect(GeometryContext{}, position, direction,
                                     BoundaryTolerance::Infinite())
                          .closest();
  mean.head<3>() = intersection.position();

  return *transformFreeToBoundParameters(mean, surface, GeometryContext{});
}

// Typedef to describe local positions of 4 components
using LocPosArray = std::array<std::pair<double, double>, 4>;

// Test the combination for a surface type. The local positions are given from
// the outside since their meaning differs between surface types
template <typename angle_description_t>
void test_surface(const Surface &surface, const angle_description_t &desc,
                  const LocPosArray &loc_pos, double expectedError) {
  const auto proj = std::identity{};

  for (auto phi : {-175_degree, 0_degree, 175_degree}) {
    for (auto theta : {5_degree, 90_degree, 175_degree}) {
      // Go create mixture with 4 cmps
      std::vector<DummyComponent<eBoundSize>> cmps;

      auto p_it = loc_pos.begin();

      for (auto dphi : {-10_degree, 10_degree}) {
        for (auto dtheta : {-5_degree, 5_degree}) {
          DummyComponent<eBoundSize> a;
          a.weight = 1. / 4.;
          a.boundPars = BoundVector::Ones();
          a.boundPars[eBoundLoc0] *= p_it->first;
          a.boundPars[eBoundLoc1] *= p_it->second;
          a.boundPars[eBoundPhi] =
              detail::wrap_periodic(phi + dphi, -M_PI, 2 * M_PI);
          a.boundPars[eBoundTheta] = theta + dtheta;

          // We don't look at covariance in this test
          a.boundCov = BoundSquareMatrix::Zero();

          cmps.push_back(a);
          ++p_it;
        }
      }

      const auto [mean_approx, cov_approx] =
          detail::gaussianMixtureMeanCov(cmps, proj, desc);

      const auto mean_ref = meanFromFree(cmps, surface);

      CHECK_CLOSE_MATRIX(mean_approx, mean_ref, expectedError);
    }
  }
}

BOOST_AUTO_TEST_CASE(test_with_data) {
  std::mt19937 gen(42);
  std::vector<DummyComponent<2>> cmps(2);

  cmps[0].boundPars << 1.0, 1.0;
  cmps[0].boundCov << 1.0, 0.0, 0.0, 1.0;
  cmps[0].weight = 0.5;

  cmps[1].boundPars << -2.0, -2.0;
  cmps[1].boundCov << 1.0, 1.0, 1.0, 2.0;
  cmps[1].weight = 0.5;

  const auto samples = sampleFromMultivariate(cmps, 10000, gen);
  const auto mean_data = mean(samples);
  const auto boundCov_data = boundCov(samples, mean_data);

  const auto [mean_test, boundCov_test] =
      detail::gaussianMixtureMeanCov(cmps, std::identity{}, std::tuple<>{});

  CHECK_CLOSE_MATRIX(mean_data, mean_test, 1.e-1);
  CHECK_CLOSE_MATRIX(boundCov_data, boundCov_test, 1.e-1);
}

BOOST_AUTO_TEST_CASE(test_with_data_circular) {
  std::mt19937 gen(42);
  std::vector<DummyComponent<2>> cmps(2);

  cmps[0].boundPars << 175_degree, 5_degree;
  cmps[0].boundCov << 20_degree, 0.0, 0.0, 20_degree;
  cmps[0].weight = 0.5;

  cmps[1].boundPars << -175_degree, -5_degree;
  cmps[1].boundCov << 20_degree, 20_degree, 20_degree, 40_degree;
  cmps[1].weight = 0.5;

  const auto samples = sampleFromMultivariate(cmps, 10000, gen);
  const auto mean_data = circularMean(samples);
  const auto boundCov_data = boundCov(samples, mean_data, [](auto a, auto b) {
    Vector2 res = Vector2::Zero();
    for (int i = 0; i < 2; ++i) {
      res[i] = detail::difference_periodic(a[i], b[i], 2 * M_PI);
    }
    return res;
  });

  using detail::CyclicAngle;
  const auto d = std::tuple<CyclicAngle<eBoundLoc0>, CyclicAngle<eBoundLoc1>>{};
  const auto [mean_test, boundCov_test] =
      detail::gaussianMixtureMeanCov(cmps, std::identity{}, d);

  BOOST_CHECK(std::abs(detail::difference_periodic(mean_data[0], mean_test[0],
                                                   2 * M_PI)) < 1.e-1);
  BOOST_CHECK(std::abs(detail::difference_periodic(mean_data[1], mean_test[1],
                                                   2 * M_PI)) < 1.e-1);
  CHECK_CLOSE_MATRIX(boundCov_data, boundCov_test, 1.e-1);
}

BOOST_AUTO_TEST_CASE(test_plane_surface) {
  const auto desc = detail::AngleDescription<Surface::Plane>::Desc{};

  const auto surface =
      CurvilinearSurface(Vector3{0, 0, 0}, Vector3{1, 0, 0}).planeSurface();

  const LocPosArray p{{{1, 1}, {1, -1}, {-1, 1}, {-1, -1}}};

  test_surface(*surface, desc, p, 1.e-2);
}

BOOST_AUTO_TEST_CASE(test_cylinder_surface) {
  const Transform3 trafo = Transform3::Identity();
  const double r = 2;
  const double halfz = 100;

  const auto surface = Surface::makeShared<CylinderSurface>(trafo, r, halfz);

  const double z1 = -1, z2 = 1;
  const double phi1 = 178_degree, phi2 = -176_degree;

  const LocPosArray p{
      {{r * phi1, z1}, {r * phi1, -z2}, {r * phi2, z1}, {r * phi2, z2}}};

  auto desc = detail::AngleDescription<Surface::Cylinder>::Desc{};
  std::get<0>(desc).constant = r;

  test_surface(*surface, desc, p, 1.e-2);
}

BOOST_AUTO_TEST_CASE(test_disc_surface) {
  const Transform3 trafo = Transform3::Identity();
  const auto radius = 1;

  const auto surface = Surface::makeShared<DiscSurface>(trafo, 0.0, radius);

  const double r1 = 0.4, r2 = 0.8;
  const double phi1 = -178_degree, phi2 = 176_degree;

  const LocPosArray p{{{r1, phi1}, {r2, phi2}, {r1, phi2}, {r2, phi1}}};

  const auto desc = detail::AngleDescription<Surface::Disc>::Desc{};

  test_surface(*surface, desc, p, 1.e-2);
}

BOOST_AUTO_TEST_CASE(test_perigee_surface) {
  const auto desc = detail::AngleDescription<Surface::Plane>::Desc{};

  const auto surface = Surface::makeShared<PerigeeSurface>(Vector3{0, 0, 0});

  const auto z = 5;
  const auto d = 1;

  const LocPosArray p{{{d, z}, {d, -z}, {2 * d, z}, {2 * d, -z}}};

  // Here we expect a very bad approximation
  test_surface(*surface, desc, p, 1.1);
}
