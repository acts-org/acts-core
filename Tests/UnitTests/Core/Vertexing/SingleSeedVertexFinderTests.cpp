// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Vertexing/SingleSeedVertexFinder.hpp"

#include <cmath>
#include <iostream>
#include <random>
#include <vector>

/// @brief SpacePoint definition to be used for the unit tests. Implements all the relevant methods.
struct SpacePoint4SSVFT {
  SpacePoint4SSVFT(double x, double y, double z) : m_x(x), m_y(y), m_z(z) {}
  double m_x;
  double m_y;
  double m_z;
  double x() const { return m_x; }
  double y() const { return m_y; }
  double z() const { return m_z; }
  double r() const { return std::sqrt(m_x * m_x + m_y * m_y); }
};

/// @brief Provides random double number between $from and $to
/// @param gen random number generator
/// @param from lower threshold
/// @param to upper threshold
/// @return random number in [from,to)
double getRndDouble(std::mt19937& gen, double from, double to) {
  return gen() / 4294967296. * (to - from) + from;
}

/// @brief Provides random integer number between $from and $to
/// @param gen random number generator
/// @param from lower threshold
/// @param to upper threshold
/// @return random number in [from,to)
int getRndInt(std::mt19937& gen, int from, int to) {
  return static_cast<int>(gen() / 4294967296. * (to - from) + from);
}

/// @brief Calculates equation of the plane (alpha*x + beta*y + gamma*z + delta = 0), given the three points
/// @param a,b,c The three points
/// @return Parameters of the plane {alpha,beta,gamma,delta}
std::vector<double> makePlaneFromTriplet(SpacePoint4SSVFT aa,
                                         SpacePoint4SSVFT bb,
                                         SpacePoint4SSVFT cc) {
  Acts::Vector3 a{aa.x(), aa.y(), aa.z()};
  Acts::Vector3 b{bb.x(), bb.y(), bb.z()};
  Acts::Vector3 c{cc.x(), cc.y(), cc.z()};

  Acts::Vector3 ba = b - a, ca = c - a;

  Acts::Vector3 abg = ba.cross(ca).normalized();
  double delta = -1. * abg.dot(a);

  // plane (alpha*x + beta*y + gamma*z + delta = 0)
  return {abg[0], abg[1], abg[2], delta};
}

/// @brief Unit test for SingleSeedVertexFinder. Fits a set of the spacepoints with planes and compare the result to the easy-to-calculate expected result
BOOST_AUTO_TEST_CASE(single_seed_vertex_finder_small_planes_test) {
  Acts::SingleSeedVertexFinder<SpacePoint4SSVFT>::Config singleSeedVtxCfg;
  singleSeedVtxCfg.maxPhideviation = 0.2;
  singleSeedVtxCfg.maxXYdeviation = 0.1;
  singleSeedVtxCfg.maxXYZdeviation = 0.1;
  singleSeedVtxCfg.minTheta = 0.5;
  singleSeedVtxCfg.rMinNear = 6.f * Acts::UnitConstants::mm;
  singleSeedVtxCfg.rMaxNear = 14.9f * Acts::UnitConstants::mm;
  singleSeedVtxCfg.rMinMiddle = 15.f * Acts::UnitConstants::mm;
  singleSeedVtxCfg.rMaxMiddle = 24.9f * Acts::UnitConstants::mm;
  singleSeedVtxCfg.rMinFar = 25.f * Acts::UnitConstants::mm;
  singleSeedVtxCfg.rMaxFar = 44.9f * Acts::UnitConstants::mm;
  singleSeedVtxCfg.numPhiSlices = 10;
  singleSeedVtxCfg.useFracPhiSlices = 1.0;
  singleSeedVtxCfg.numZSlices = 10;
  singleSeedVtxCfg.useFracZSlices = 1.0;
  singleSeedVtxCfg.maxAbsZ = 50. * Acts::UnitConstants::mm;
  singleSeedVtxCfg.maxZPosition = 20.f * Acts::UnitConstants::mm;
  singleSeedVtxCfg.maxRPosition = 5.f * Acts::UnitConstants::mm;
  singleSeedVtxCfg.minimalizeWRT = "planes";
  singleSeedVtxCfg.maxIterations = 3;
  singleSeedVtxCfg.removeFraction = 0.3;
  singleSeedVtxCfg.minVtxShift = 0.3f * Acts::UnitConstants::mm;
  Acts::SingleSeedVertexFinder<SpacePoint4SSVFT> SingleSeedVertexFinder(
      singleSeedVtxCfg);

  double vtxX = 1.;
  double vtxY = -1.;
  double vtxZ = 10;

  std::vector<std::vector<double>> positions = {
      {10.8, 0., 7.},      {20.9, 0.7, 4.},
      {30.5, 1.9, 1.},  // plane ((-0.25,-0.25,-0.9), 9.0)
      {2.1, 10.6, 15.2},   {2.7, 19.36666, 19.5},
      {4.5, 34., 25.4},  // plane ((0.8, -0.3, 0.5), -6.1)
      {-6.25, -7.9, 10.5}, {-12.8, -15.08, 11.},
      {-20., -22., 11.5},  // plane ((-0.02,-0.05,-0.98), 9.77)
      {-6., 8., 2.4},      {-12., 15., -3.4},
      {-17., 21., -8.4},  // plane ((0.1,0.5,0.5), -4.6)
      {7.8, 7.8, 16.0},    {14.8, 15., 17.},
      {22.8, 23.3, 18.},  // this will be removed after iteration
      {-5.1, 8.1, -10.},   {-1.3, 9., -10.4},
      {3.1, 10.1, -11.1}};  // this will not form a triplet

  std::vector<SpacePoint4SSVFT> inputSpacepoints;
  for (auto pos : positions) {
    inputSpacepoints.emplace_back(pos[0], pos[1], pos[2]);
  }

  auto t1 = std::chrono::high_resolution_clock::now();
  auto vtx = SingleSeedVertexFinder.findVertex(inputSpacepoints);
  auto t2 = std::chrono::high_resolution_clock::now();

  bool vtxFound = false;
  if (vtx.ok()) {
    std::cout << "Found a vertex in the event in " << (t2 - t1).count() / 1e6
              << " ms at x = " << vtx.value()[0] << "mm, y = " << vtx.value()[1]
              << "mm, z = " << vtx.value()[2] << "mm" << std::endl;
    std::cout << "Truth vertex was at x = " << vtxX << "mm, y = " << vtxY
              << "mm, z = " << vtxZ << "mm" << std::endl;

    if (std::abs(vtxX - vtx.value()[0]) < 1e-3 &&
        std::abs(vtxY - vtx.value()[1]) < 1e-3 &&
        std::abs(vtxZ - vtx.value()[2]) < 1e-3) {
      vtxFound = true;
    }
  } else {
    std::cout << "Not found a vertex in the event after "
              << (t2 - t1).count() / 1e6 << " ms" << std::endl;
    std::cout << "Truth vertex was at x = " << vtxX << "mm, y = " << vtxY
              << "mm, z = " << vtxZ << "mm" << std::endl;
  }

  // check if found vertex has compatible position
  BOOST_CHECK(vtxFound);
}

/// @brief Unit test for SingleSeedVertexFinder. Fits a set of the spacepoints with straigh lines and compare the result to the easy-to-calculate expected result
BOOST_AUTO_TEST_CASE(single_seed_vertex_finder_small_rays_test) {
  Acts::SingleSeedVertexFinder<SpacePoint4SSVFT>::Config singleSeedVtxCfg;
  singleSeedVtxCfg.maxPhideviation = 0.2;
  singleSeedVtxCfg.maxXYdeviation = 0.1;
  singleSeedVtxCfg.maxXYZdeviation = 0.1;
  singleSeedVtxCfg.minTheta = 0.5;
  singleSeedVtxCfg.rMinNear = 6.f * Acts::UnitConstants::mm;
  singleSeedVtxCfg.rMaxNear = 14.9f * Acts::UnitConstants::mm;
  singleSeedVtxCfg.rMinMiddle = 15.f * Acts::UnitConstants::mm;
  singleSeedVtxCfg.rMaxMiddle = 24.9f * Acts::UnitConstants::mm;
  singleSeedVtxCfg.rMinFar = 25.f * Acts::UnitConstants::mm;
  singleSeedVtxCfg.rMaxFar = 44.9f * Acts::UnitConstants::mm;
  singleSeedVtxCfg.numPhiSlices = 10;
  singleSeedVtxCfg.useFracPhiSlices = 1.0;
  singleSeedVtxCfg.numZSlices = 10;
  singleSeedVtxCfg.useFracZSlices = 1.0;
  singleSeedVtxCfg.maxAbsZ = 50. * Acts::UnitConstants::mm;
  singleSeedVtxCfg.maxZPosition = 20.f * Acts::UnitConstants::mm;
  singleSeedVtxCfg.maxRPosition = 5.f * Acts::UnitConstants::mm;
  singleSeedVtxCfg.minimalizeWRT = "rays";
  singleSeedVtxCfg.maxIterations = 1;
  singleSeedVtxCfg.removeFraction = 0.3;
  singleSeedVtxCfg.minVtxShift = 0.3f * Acts::UnitConstants::mm;
  Acts::SingleSeedVertexFinder<SpacePoint4SSVFT> SingleSeedVertexFinder(
      singleSeedVtxCfg);

  double vtxX = 1.;
  double vtxY = -1.;
  double vtxZ = 10;

  std::vector<std::vector<double>> positions = {
      {11., 0., 7.},      {21., 1., 4.},
      {31., 2., 1.},  // this ray is Ok
      {2., 9., 15.},      {3., 19., 20.},
      {4.5, 34., 27.5},  // this ray is Ok
      {-6., -8., 10.5},   {-13., -15., 11.},
      {-20., -22., 11.5},  // this ray is Ok
      {7., 7., 16.0},     {14., 14., 17.},
      {21., 21., 18.},  // this will be removed after iteration
      {-5., 8., -10.},    {-1.5, 9., -10.5},
      {3., 10., -11.}};  // this will not form a triplet

  std::vector<SpacePoint4SSVFT> inputSpacepoints;
  for (auto pos : positions) {
    inputSpacepoints.emplace_back(pos[0], pos[1], pos[2]);
  }

  auto t1 = std::chrono::high_resolution_clock::now();
  auto vtx = SingleSeedVertexFinder.findVertex(inputSpacepoints);
  auto t2 = std::chrono::high_resolution_clock::now();

  bool vtxFound = false;
  if (vtx.ok()) {
    std::cout << "Found a vertex in the event in " << (t2 - t1).count() / 1e6
              << " ms at x = " << vtx.value()[0] << "mm, y = " << vtx.value()[1]
              << "mm, z = " << vtx.value()[2] << "mm" << std::endl;
    std::cout << "Truth vertex was at x = " << vtxX << "mm, y = " << vtxY
              << "mm, z = " << vtxZ << "mm" << std::endl;

    if (std::abs(vtxX - vtx.value()[0]) < 1e-3 &&
        std::abs(vtxY - vtx.value()[1]) < 1e-3 &&
        std::abs(vtxZ - vtx.value()[2]) < 1e-3) {
      vtxFound = true;
    }
  } else {
    std::cout << "Not found a vertex in the event after "
              << (t2 - t1).count() / 1e6 << " ms" << std::endl;
    std::cout << "Truth vertex was at x = " << vtxX << "mm, y = " << vtxY
              << "mm, z = " << vtxZ << "mm" << std::endl;
  }

  // check if found vertex has compatible position
  BOOST_CHECK(vtxFound);
}

/// @brief Unit test for SingleSeedVertexFinder. Generates real-looking sets of the spacepoints, then fits them with planes, finds a vertex, and then verifies the reconstructed vertex is actually near the original one
BOOST_AUTO_TEST_CASE(single_seed_vertex_finder_full_planes_test) {
  Acts::SingleSeedVertexFinder<SpacePoint4SSVFT>::Config singleSeedVtxCfg;
  singleSeedVtxCfg.maxPhideviation = 0.1;
  singleSeedVtxCfg.maxXYdeviation = 0.1;
  singleSeedVtxCfg.maxXYZdeviation = 0.1;
  singleSeedVtxCfg.minTheta = 0.5;
  singleSeedVtxCfg.rMinNear = 8.f * Acts::UnitConstants::mm;
  singleSeedVtxCfg.rMaxNear = 12.f * Acts::UnitConstants::mm;
  singleSeedVtxCfg.rMinMiddle = 18.f * Acts::UnitConstants::mm;
  singleSeedVtxCfg.rMaxMiddle = 22.f * Acts::UnitConstants::mm;
  singleSeedVtxCfg.rMinFar = 28.f * Acts::UnitConstants::mm;
  singleSeedVtxCfg.rMaxFar = 32.f * Acts::UnitConstants::mm;
  singleSeedVtxCfg.numPhiSlices = 60;
  singleSeedVtxCfg.useFracPhiSlices = 0.8;
  singleSeedVtxCfg.numZSlices = 150;
  singleSeedVtxCfg.useFracZSlices = 0.8;
  singleSeedVtxCfg.maxAbsZ = 75. * Acts::UnitConstants::mm;
  singleSeedVtxCfg.maxZPosition = 15.f * Acts::UnitConstants::mm;
  singleSeedVtxCfg.maxRPosition = 2.5f * Acts::UnitConstants::mm;
  singleSeedVtxCfg.minimalizeWRT = "planes";
  singleSeedVtxCfg.maxIterations = 20;
  singleSeedVtxCfg.removeFraction = 0.1;
  singleSeedVtxCfg.minVtxShift = 0.05f * Acts::UnitConstants::mm;
  Acts::SingleSeedVertexFinder<SpacePoint4SSVFT> SingleSeedVertexFinder(
      singleSeedVtxCfg);

  std::mt19937 gen(299792458);

  int vtxFound = 0;
  int nEvents = 5;
  for (int event = 0; event < nEvents; event++) {
    double vtxX = getRndDouble(gen, -1., 1.);
    double vtxY = getRndDouble(gen, -1., 1.);
    double vtxZ = getRndDouble(gen, -10., 10.);

    std::vector<SpacePoint4SSVFT> inputSpacepoints;

    // make straight lines originating from the given vertex
    int nTracks = getRndInt(gen, 200, 400);
    for (int track = 0; track < nTracks; ++track) {
      // initial position of the track
      double posX = vtxX;
      double posY = vtxY;
      double posZ = vtxZ;

      // initial direction of the track
      double dirX = getRndDouble(gen, -1., 1.);
      double dirY = getRndDouble(gen, -1., 1.);
      double dirZ = getRndDouble(gen, -1., 1.);
      // rotation of the track
      double theta = getRndDouble(gen, 0.03, 0.09) *
                     (getRndDouble(gen, -1., 1.) > 0 ? 1 : -1);
      double sgn = std::copysign(1., dirY);

      for (int i = 1; i <= 3; ++i) {
        if (i != 1) {
          // rotate direction, not for the first time
          double dirXtmp = cos(theta) * dirX - sin(theta) * dirY;
          double dirYtmp = sin(theta) * dirX + cos(theta) * dirY;
          dirX = dirXtmp;
          dirY = dirYtmp;
        }

        // double sgn=std::copysign(1.,dirY);
        double dirR2 = dirX * dirX + dirY * dirY;
        double D = posX * (posY + dirY) - posY * (posX + dirX);
        // add some smearing to the layers
        // layers are (9-11), (19-21), and (29-31)
        double r = i * 10. + getRndDouble(gen, -1., 1.);
        // intersection of the layer and the straigh line
        double x1 =
            (D * dirY + sgn * dirX * std::sqrt(r * r * dirR2 - D * D)) / dirR2;
        double y1 =
            (-D * dirX + std::fabs(dirY) * std::sqrt(r * r * dirR2 - D * D)) /
            dirR2;
        // how many units from the vertex to the intersection
        double zDist = std::fabs((x1 - posX) / dirX);

        // position of the new spacepoint
        posX = x1;
        posY = y1;
        // use the same amount of units for distance in Z
        posZ += zDist * dirZ;

        if (i == 3) {
          // move z position, so the vertex will be part of the plane
          auto abgd = makePlaneFromTriplet({vtxX, vtxY, vtxZ},
                                           inputSpacepoints.rbegin()[1],
                                           inputSpacepoints.rbegin()[0]);
          posZ = -1. * (abgd[0] * posX + abgd[1] * posY + abgd[3]) / abgd[2];
        }

        inputSpacepoints.emplace_back(posX, posY, posZ);
      }
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    auto vtx = SingleSeedVertexFinder.findVertex(inputSpacepoints);
    auto t2 = std::chrono::high_resolution_clock::now();

    if (vtx.ok()) {
      std::cout << "Found a vertex in the event in " << (t2 - t1).count() / 1e6
                << " ms at x = " << vtx.value()[0]
                << "mm, y = " << vtx.value()[1] << "mm, z = " << vtx.value()[2]
                << "mm" << std::endl;
      std::cout << "Truth vertex was at x = " << vtxX << "mm, y = " << vtxY
                << "mm, z = " << vtxZ << "mm" << std::endl;
      std::cout << "Difference is in x = " << std::abs(vtxX - vtx.value()[0])
                << "mm, y = " << std::abs(vtxY - vtx.value()[1])
                << "mm, z = " << std::abs(vtxZ - vtx.value()[2]) << "mm"
                << std::endl;
      if (std::abs(vtxX - vtx.value()[0]) < 2.0 &&
          std::abs(vtxY - vtx.value()[1]) < 2.0 &&
          std::abs(vtxZ - vtx.value()[2]) < 0.3) {
        ++vtxFound;
      }
    } else {
      std::cout << "Not found a vertex in the event after "
                << (t2 - t1).count() / 1e6 << " ms" << std::endl;
      std::cout << "Truth vertex was at x = " << vtxX << "mm, y = " << vtxY
                << "mm, z = " << vtxZ << "mm" << std::endl;
    }
  }

  std::cout << "Found " << vtxFound << " out of " << nEvents << " vertices"
            << std::endl;

  // check if all vertices have compatible positions
  BOOST_CHECK_EQUAL(vtxFound, nEvents);
}

/// @brief Unit test for SingleSeedVertexFinder. Generates real-looking sets of the spacepoints, then fits them with rays, finds a vertex, and then verifies the reconstructed vertex is actually near the original one
BOOST_AUTO_TEST_CASE(single_seed_vertex_finder_full_rays_test) {
  Acts::SingleSeedVertexFinder<SpacePoint4SSVFT>::Config singleSeedVtxCfg;
  singleSeedVtxCfg.maxPhideviation = 0.1;
  singleSeedVtxCfg.maxXYdeviation = 0.1;
  singleSeedVtxCfg.maxXYZdeviation = 0.1;
  singleSeedVtxCfg.minTheta = 0.5;
  singleSeedVtxCfg.rMinNear = 8.f * Acts::UnitConstants::mm;
  singleSeedVtxCfg.rMaxNear = 12.f * Acts::UnitConstants::mm;
  singleSeedVtxCfg.rMinMiddle = 18.f * Acts::UnitConstants::mm;
  singleSeedVtxCfg.rMaxMiddle = 22.f * Acts::UnitConstants::mm;
  singleSeedVtxCfg.rMinFar = 28.f * Acts::UnitConstants::mm;
  singleSeedVtxCfg.rMaxFar = 32.f * Acts::UnitConstants::mm;
  singleSeedVtxCfg.numPhiSlices = 60;
  singleSeedVtxCfg.useFracPhiSlices = 0.8;
  singleSeedVtxCfg.numZSlices = 150;
  singleSeedVtxCfg.useFracZSlices = 0.8;
  singleSeedVtxCfg.maxAbsZ = 75. * Acts::UnitConstants::mm;
  singleSeedVtxCfg.maxZPosition = 15.f * Acts::UnitConstants::mm;
  singleSeedVtxCfg.maxRPosition = 2.5f * Acts::UnitConstants::mm;
  singleSeedVtxCfg.minimalizeWRT = "rays";
  singleSeedVtxCfg.maxIterations = 10;
  singleSeedVtxCfg.removeFraction = 0.1;
  singleSeedVtxCfg.minVtxShift = 0.05f * Acts::UnitConstants::mm;
  Acts::SingleSeedVertexFinder<SpacePoint4SSVFT> SingleSeedVertexFinder(
      singleSeedVtxCfg);

  std::mt19937 gen(299792458);

  int vtxFound = 0;
  int nEvents = 5;
  for (int event = 0; event < nEvents; event++) {
    double vtxX = getRndDouble(gen, -1., 1.);
    double vtxY = getRndDouble(gen, -1., 1.);
    double vtxZ = getRndDouble(gen, -10., 10.);

    std::vector<SpacePoint4SSVFT> inputSpacepoints;

    // make straight lines originating from the given vertex
    int nTracks = getRndInt(gen, 200, 400);
    for (int track = 0; track < nTracks; ++track) {
      // direction of the track
      double dirX = getRndDouble(gen, -1., 1.);
      double dirY = getRndDouble(gen, -1., 1.);
      double dirZ = getRndDouble(gen, -1., 1.);
      // use upper or lower intersection?
      int part = (getRndDouble(gen, -1., 1.) > 0 ? 1 : -1);

      for (int rIndx = 1; rIndx <= 3; rIndx += 1) {
        double sgn = std::copysign(1., dirY);
        double dirR2 = dirX * dirX + dirY * dirY;
        double D = vtxX * (vtxY + dirY) - vtxY * (vtxX + dirX);
        // add some smearing to the layers
        // layers are (9-11), (19-21), and (29-31)
        double r = rIndx * 10 + getRndDouble(gen, -1., 1.);
        // intersection of the layer and the straigh line
        double x1 =
            (D * dirY + part * sgn * dirX * std::sqrt(r * r * dirR2 - D * D)) /
            dirR2;
        double y1 = (-D * dirX + part * std::fabs(dirY) *
                                     std::sqrt(r * r * dirR2 - D * D)) /
                    dirR2;
        // how many units from the vertex to the intersection
        double zDist = std::fabs((x1 - vtxX) / dirX);
        // use the same amount of units for distance in Z
        inputSpacepoints.emplace_back(x1, y1, zDist * dirZ + vtxZ);
      }
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    auto vtx = SingleSeedVertexFinder.findVertex(inputSpacepoints);
    auto t2 = std::chrono::high_resolution_clock::now();

    if (vtx.ok()) {
      std::cout << "Found a vertex in the event in " << (t2 - t1).count() / 1e6
                << " ms at x = " << vtx.value()[0]
                << "mm, y = " << vtx.value()[1] << "mm, z = " << vtx.value()[2]
                << "mm" << std::endl;
      std::cout << "Truth vertex was at x = " << vtxX << "mm, y = " << vtxY
                << "mm, z = " << vtxZ << "mm" << std::endl;
      std::cout << "Difference is in x = " << std::abs(vtxX - vtx.value()[0])
                << "mm, y = " << std::abs(vtxY - vtx.value()[1])
                << "mm, z = " << std::abs(vtxZ - vtx.value()[2]) << "mm"
                << std::endl;
      if (std::abs(vtxX - vtx.value()[0]) < 0.3 &&
          std::abs(vtxY - vtx.value()[1]) < 0.3 &&
          std::abs(vtxZ - vtx.value()[2]) < 0.3) {
        ++vtxFound;
      }
    } else {
      std::cout << "Not found a vertex in the event after "
                << (t2 - t1).count() / 1e6 << " ms" << std::endl;
      std::cout << "Truth vertex was at x = " << vtxX << "mm, y = " << vtxY
                << "mm, z = " << vtxZ << "mm" << std::endl;
    }
  }

  std::cout << "Found " << vtxFound << " out of " << nEvents << " vertices"
            << std::endl;

  // check if all vertices have compatible positions
  BOOST_CHECK_EQUAL(vtxFound, nEvents);
}
