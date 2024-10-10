// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <algorithm>
#include <cmath>
#include <system_error>

#include <Eigen/Eigenvalues>

template <typename spacepoint_t>
Acts::SingleSeedVertexFinder<spacepoint_t>::SingleSeedVertexFinder(
    const Acts::SingleSeedVertexFinder<spacepoint_t>::Config& cfg,
    std::unique_ptr<const Logger> lgr)
    : m_cfg(cfg), m_logger(std::move(lgr)) {
  if (cfg.numPhiSlices < 3) {
    ACTS_INFO("value of numPhiSlices is "
              << cfg.numPhiSlices
              << ", which is less than 3. There will be duplicate triplets.");
  }
  if (cfg.useFracPhiSlices <= 0. || cfg.useFracPhiSlices > 1.) {
    ACTS_ERROR("value of useFracPhiSlices is "
               << cfg.useFracPhiSlices
               << ", allowed values are between 0 and 1");
  }
  if (cfg.useFracZSlices <= 0. || cfg.useFracZSlices > 1.) {
    ACTS_ERROR("value of useFracZSlices is "
               << cfg.useFracZSlices << ", allowed values are between 0 and 1");
  }
  if (cfg.minimalizeWRT != "planes" && cfg.minimalizeWRT != "rays") {
    ACTS_ERROR("value of minimalizeWRT is "
               << cfg.minimalizeWRT
               << ", allowed values are \"planes\" or \"rays\" ");
  }
  if (cfg.removeFraction < 0. || cfg.removeFraction >= 1.) {
    ACTS_ERROR("value of removeFraction is "
               << cfg.removeFraction << ", allowed values are between 0 and 1");
  }
}

template <typename spacepoint_t>
Acts::Result<Acts::Vector3>
Acts::SingleSeedVertexFinder<spacepoint_t>::findVertex(
    const std::vector<spacepoint_t>& spacepoints) const {
  // sort spacepoints to different phi and z slices
  Acts::SingleSeedVertexFinder<spacepoint_t>::SortedSpacepoints
      sortedSpacepoints = sortSpacepoints(spacepoints);

  // find triplets
  std::vector<Acts::SingleSeedVertexFinder<spacepoint_t>::Triplet> triplets =
      findTriplets(sortedSpacepoints);

  // if no valid triplets found
  if (triplets.empty()) {
    return Acts::Result<Acts::Vector3>::failure(std::error_code());
  }

  Acts::Vector3 vtx = Acts::Vector3::Zero();
  if (m_cfg.minimalizeWRT == "planes") {
    // find a point closest to all planes defined by the triplets
    vtx = findClosestPointFromPlanes(triplets);
  } else if (m_cfg.minimalizeWRT == "rays") {
    // find a point closest to all rays fitted through the triplets
    vtx = findClosestPointFromRays(triplets);
  } else {
    ACTS_ERROR("value of minimalizeWRT is "
               << m_cfg.minimalizeWRT
               << ", allowed values are \"planes\" or \"rays\" ");
  }

  return Acts::Result<Acts::Vector3>::success(vtx);
}

template <typename spacepoint_t>
typename Acts::SingleSeedVertexFinder<spacepoint_t>::SortedSpacepoints
Acts::SingleSeedVertexFinder<spacepoint_t>::sortSpacepoints(
    const std::vector<spacepoint_t>& spacepoints) const {
  Acts::SingleSeedVertexFinder<spacepoint_t>::SortedSpacepoints
      sortedSpacepoints(m_cfg.numPhiSlices, m_cfg.numZSlices);

  for (const auto& sp : spacepoints) {
    // phi will be saved for later
    Acts::ActsScalar phi = detail::radian_pos(std::atan2(sp.y(), sp.x()));
    std::uint32_t phislice =
        static_cast<std::uint32_t>(phi / (2 * M_PI) * m_cfg.numPhiSlices);
    if (phislice >= m_cfg.numPhiSlices) {
      phislice = 0;
    }

    if (std::abs(sp.z()) >= m_cfg.maxAbsZ) {
      continue;
    }
    std::uint32_t zslice = static_cast<std::uint32_t>(
        (sp.z() + m_cfg.maxAbsZ) / (2 * m_cfg.maxAbsZ) * m_cfg.numZSlices);

    // input spacepoint is sorted into one subset
    if (sp.r() < m_cfg.rMinMiddle) {
      if (m_cfg.rMinNear < sp.r() && sp.r() < m_cfg.rMaxNear) {
        if (std::fmod(m_cfg.useFracPhiSlices * phislice, 1.0) >=
            m_cfg.useFracPhiSlices) {
          continue;
        }
        sortedSpacepoints.addSP(0, phislice, zslice)
            .emplace_back(static_cast<spacepoint_t const*>(&sp), phi);
      }
    } else if (sp.r() < m_cfg.rMinFar) {
      if (sp.r() < m_cfg.rMaxMiddle) {
        if (std::fmod(m_cfg.useFracZSlices * zslice, 1.0) >=
            m_cfg.useFracZSlices) {
          continue;
        }
        sortedSpacepoints.addSP(1, phislice, zslice)
            .emplace_back(static_cast<spacepoint_t const*>(&sp), phi);
      }
    } else if (sp.r() < m_cfg.rMaxFar) {
      sortedSpacepoints.addSP(2, phislice, zslice)
          .emplace_back(static_cast<spacepoint_t const*>(&sp), phi);
    }
  }

  return sortedSpacepoints;
}

template <typename spacepoint_t>
std::vector<typename Acts::SingleSeedVertexFinder<spacepoint_t>::Triplet>
Acts::SingleSeedVertexFinder<spacepoint_t>::findTriplets(
    const Acts::SingleSeedVertexFinder<spacepoint_t>::SortedSpacepoints&
        sortedSpacepoints) const {
  std::vector<Acts::SingleSeedVertexFinder<spacepoint_t>::Triplet> triplets;

  std::uint32_t phiStep =
      static_cast<std::uint32_t>(m_cfg.maxPhideviation /
                                 (2 * M_PI / m_cfg.numPhiSlices)) +
      1;

  // calculate limits for middle spacepoints
  Acts::Vector2 vecA{-m_cfg.maxAbsZ + m_cfg.maxZPosition, m_cfg.rMinFar};
  vecA /= 2.;
  Acts::Vector2 vecB = {vecA[1], -vecA[0]};
  vecB /= std::tan(m_cfg.maxXYZdeviation);
  Acts::Vector2 posR = Acts::Vector2(-m_cfg.maxZPosition, 0.) + vecA + vecB;
  Acts::ActsScalar R = vecA.norm() / std::sin(m_cfg.maxXYZdeviation);
  Acts::ActsScalar constB = -2. * posR[0];
  Acts::ActsScalar constC =
      posR[0] * posR[0] +
      (posR[1] - m_cfg.rMaxNear) * (posR[1] - m_cfg.rMaxNear) - R * R;
  Acts::ActsScalar maxZMiddle =
      -1. * (-constB - sqrt(constB * constB - 4. * constC)) / 2.;
  if (maxZMiddle <= 0) {
    ACTS_WARNING(
        "maximum position of middle spacepoints is not positive, maxZMiddle = "
        << maxZMiddle << ", check your config; setting maxZMiddle to "
        << m_cfg.maxAbsZ);
    maxZMiddle = m_cfg.maxAbsZ;
  }

  // save some constant values for later
  Acts::ActsScalar rNearRatio[2] = {m_cfg.rMinNear / m_cfg.rMaxMiddle,
                                    m_cfg.rMaxNear / m_cfg.rMinMiddle};
  Acts::ActsScalar rMiddle[2] = {m_cfg.rMaxMiddle, m_cfg.rMinMiddle};
  Acts::ActsScalar rFarDelta[2] = {
      m_cfg.rMaxFar - m_cfg.rMinMiddle,
      m_cfg.rMinFar - m_cfg.rMaxMiddle,
  };
  Acts::ActsScalar zBinLength = 2. * m_cfg.maxAbsZ / m_cfg.numZSlices;

  // limits in terms of slice numbers
  std::uint32_t limitMiddleSliceFrom =
      static_cast<std::uint32_t>((-maxZMiddle + m_cfg.maxAbsZ) / zBinLength);
  std::uint32_t limitMiddleSliceTo =
      static_cast<std::uint32_t>((maxZMiddle + m_cfg.maxAbsZ) / zBinLength + 1);
  std::uint32_t limitAbsZSliceFrom = static_cast<std::uint32_t>(
      (-m_cfg.maxZPosition + m_cfg.maxAbsZ) / zBinLength + 0.01);
  std::uint32_t limitAbsZSliceTo = static_cast<std::uint32_t>(
      (m_cfg.maxZPosition + m_cfg.maxAbsZ) / zBinLength + 1.01);

  for (std::uint32_t middleZ = limitMiddleSliceFrom;
       middleZ < limitMiddleSliceTo; ++middleZ) {
    // skip slices that are empty anyway
    if (std::fmod(m_cfg.useFracZSlices * middleZ, 1.0) >=
        m_cfg.useFracZSlices) {
      continue;
    }

    // calculate limits for near spacepoints, assuming the middle spacepoints
    // are within some boundaries
    bool isLessFrom = (middleZ <= limitAbsZSliceFrom);
    Acts::ActsScalar deltaZfrom =
        (middleZ - limitAbsZSliceFrom - 1) * zBinLength;
    Acts::ActsScalar angleZfrom =
        std::atan2(rMiddle[isLessFrom], deltaZfrom) + m_cfg.maxXYZdeviation;
    std::uint32_t nearZFrom = 0;
    if (angleZfrom < M_PI) {
      Acts::ActsScalar new_deltaZfrom =
          rMiddle[isLessFrom] / std::tan(angleZfrom) / zBinLength;
      nearZFrom = static_cast<std::uint32_t>(std::max(
          new_deltaZfrom * rNearRatio[isLessFrom] + limitAbsZSliceFrom, 0.));
    }

    bool isLessTo = (middleZ < limitAbsZSliceTo);
    Acts::ActsScalar deltaZto = (middleZ - limitAbsZSliceTo + 1) * zBinLength;
    Acts::ActsScalar angleZto =
        std::atan2(rMiddle[!isLessTo], deltaZto) - m_cfg.maxXYZdeviation;
    std::uint32_t nearZTo = m_cfg.numZSlices;
    if (angleZto > 0) {
      Acts::ActsScalar new_deltaZto =
          rMiddle[!isLessTo] / std::tan(angleZto) / zBinLength;
      nearZTo = static_cast<std::uint32_t>(std::max(
          new_deltaZto * rNearRatio[!isLessTo] + limitAbsZSliceTo, 0.));
      if (nearZTo > m_cfg.numZSlices) {
        nearZTo = m_cfg.numZSlices;
      }
    }

    for (std::uint32_t nearZ = nearZFrom; nearZ < nearZTo; ++nearZ) {
      // calculate limits for far spacepoits, assuming middle and near
      // spacepoits are within some boundaries
      bool isMiddleLess = (middleZ <= nearZ);

      Acts::ActsScalar delta2Zfrom = (middleZ - nearZ - 1) * zBinLength;
      Acts::ActsScalar angle2Zfrom =
          std::atan2(rFarDelta[isMiddleLess], delta2Zfrom) +
          m_cfg.maxXYZdeviation;
      std::uint32_t farZFrom = 0;
      if (angle2Zfrom < M_PI) {
        farZFrom = static_cast<std::uint32_t>(std::max(
            (rFarDelta[isMiddleLess] / std::tan(angle2Zfrom) / zBinLength) +
                middleZ,
            0.));
        if (farZFrom >= m_cfg.numZSlices) {
          continue;
        }
      }

      isMiddleLess = (middleZ < nearZ);
      Acts::ActsScalar delta2Zto = (middleZ - nearZ + 1) * zBinLength;
      Acts::ActsScalar angle2Zto =
          std::atan2(rFarDelta[!isMiddleLess], delta2Zto) -
          m_cfg.maxXYZdeviation;
      std::uint32_t farZTo = m_cfg.numZSlices;
      if (angle2Zto > 0) {
        farZTo = static_cast<std::uint32_t>(std::max(
            (rFarDelta[!isMiddleLess] / std::tan(angle2Zto) / zBinLength) +
                middleZ + 1,
            0.));
        if (farZTo > m_cfg.numZSlices) {
          farZTo = m_cfg.numZSlices;
        } else if (farZTo == 0) {
          continue;
        }
      }

      for (std::uint32_t farZ = farZFrom; farZ < farZTo; farZ++) {
        // loop over near phi slices
        for (std::uint32_t nearPhi = 0; nearPhi < m_cfg.numPhiSlices;
             ++nearPhi) {
          // skip slices that are empty anyway
          if (std::fmod(m_cfg.useFracPhiSlices * nearPhi, 1.0) >=
              m_cfg.useFracPhiSlices) {
            continue;
          }

          // loop over some middle phi slices
          for (std::uint32_t middlePhi_h =
                   m_cfg.numPhiSlices + nearPhi - phiStep;
               middlePhi_h <= m_cfg.numPhiSlices + nearPhi + phiStep;
               ++middlePhi_h) {
            std::uint32_t middlePhi = middlePhi_h % m_cfg.numPhiSlices;

            // loop over some far phi slices
            for (std::uint32_t farPhi_h =
                     m_cfg.numPhiSlices + middlePhi - phiStep;
                 farPhi_h <= m_cfg.numPhiSlices + middlePhi + phiStep;
                 ++farPhi_h) {
              std::uint32_t farPhi = farPhi_h % m_cfg.numPhiSlices;

              // for all near spacepoints in this slice
              for (const auto& nearSP :
                   sortedSpacepoints.getSP(0, nearPhi, nearZ)) {
                Acts::ActsScalar phiA = nearSP.second;

                // for all middle spacepoints in this slice
                for (const auto& middleSP :
                     sortedSpacepoints.getSP(1, middlePhi, middleZ)) {
                  Acts::ActsScalar phiB = middleSP.second;
                  Acts::ActsScalar deltaPhiAB =
                      detail::difference_periodic(phiA, phiB, 2 * M_PI);
                  if (std::abs(deltaPhiAB) > m_cfg.maxPhideviation) {
                    continue;
                  }

                  // for all far spacepoints in this slice
                  for (const auto& farSP :
                       sortedSpacepoints.getSP(2, farPhi, farZ)) {
                    Acts::ActsScalar phiC = farSP.second;
                    Acts::ActsScalar deltaPhiBC =
                        detail::difference_periodic(phiB, phiC, 2 * M_PI);
                    if (std::abs(deltaPhiBC) > m_cfg.maxPhideviation) {
                      continue;
                    }

                    Acts::SingleSeedVertexFinder<spacepoint_t>::Triplet tr(
                        *nearSP.first, *middleSP.first, *farSP.first);

                    if (tripletValidationAndUpdate(tr)) {
                      triplets.push_back(tr);
                    }
                  }  // loop over far spacepoints
                }  // loop over middle spacepoints
              }  // loop over near spacepoints
            }  // loop over far phi slices
          }  // loop over middle phi slices
        }  // loop over near phi slices
      }  // loop over far Z slices
    }  // loop over near Z slices
  }  // loop over middle Z slices

  return triplets;
}

template <typename spacepoint_t>
bool Acts::SingleSeedVertexFinder<spacepoint_t>::tripletValidationAndUpdate(
    Acts::SingleSeedVertexFinder<spacepoint_t>::Triplet& triplet) const {
  // slope for near+middle spacepoints
  Acts::ActsScalar alpha1 =
      std::atan2(triplet.a.y() - triplet.b.y(), triplet.a.x() - triplet.b.x());
  // slope for middle+far spacepoints
  Acts::ActsScalar alpha2 =
      std::atan2(triplet.b.y() - triplet.c.y(), triplet.b.x() - triplet.c.x());
  // these two slopes shouldn't be too different
  Acts::ActsScalar deltaAlpha =
      detail::difference_periodic(alpha1, alpha2, 2 * M_PI);
  if (std::abs(deltaAlpha) > m_cfg.maxXYdeviation) {
    return false;
  }

  // near-middle ray
  Acts::Vector3 ab{triplet.a.x() - triplet.b.x(), triplet.a.y() - triplet.b.y(),
                   triplet.a.z() - triplet.b.z()};
  // middle-far ray
  Acts::Vector3 bc{triplet.b.x() - triplet.c.x(), triplet.b.y() - triplet.c.y(),
                   triplet.b.z() - triplet.c.z()};
  // dot product of these two
  Acts::ActsScalar cosTheta = (ab.dot(bc)) / (ab.norm() * bc.norm());
  Acts::ActsScalar theta = std::acos(cosTheta);
  if (theta > m_cfg.maxXYZdeviation) {
    return false;
  }

  // reject the ray if it doesn't come close to the z-axis
  Acts::Ray3D ray = makeRayFromTriplet(triplet);
  const Acts::Vector3& startPoint = ray.origin();
  const Acts::Vector3& direction = ray.dir();
  // norm to z-axis and to the ray
  Acts::Vector3 norm{-1. * direction[1], 1. * direction[0], 0};
  Acts::ActsScalar norm_size = norm.norm();

  Acts::ActsScalar tanTheta = norm_size / direction[2];
  if (std::abs(tanTheta) < std::tan(m_cfg.minTheta)) {
    return false;
  }

  // nearest distance from the ray to z-axis
  Acts::ActsScalar dist = std::abs(startPoint.dot(norm)) / norm_size;
  if (dist > m_cfg.maxRPosition) {
    return false;
  }

  // z coordinate of the nearest distance from the ray to z-axis
  Acts::ActsScalar zDist =
      direction.cross(norm).dot(startPoint) / (norm_size * norm_size);
  if (std::abs(zDist) > m_cfg.maxZPosition) {
    return false;
  }

  if (m_cfg.minimalizeWRT == "rays") {
    // save for later
    triplet.ray = ray;
  }

  return true;
}

template <typename spacepoint_t>
std::pair<Acts::Vector3, Acts::ActsScalar>
Acts::SingleSeedVertexFinder<spacepoint_t>::makePlaneFromTriplet(
    const Acts::SingleSeedVertexFinder<spacepoint_t>::Triplet& triplet) {
  Acts::Vector3 a{triplet.a.x(), triplet.a.y(), triplet.a.z()};
  Acts::Vector3 b{triplet.b.x(), triplet.b.y(), triplet.b.z()};
  Acts::Vector3 c{triplet.c.x(), triplet.c.y(), triplet.c.z()};

  Acts::Vector3 ba = b - a, ca = c - a;

  // vector (alpha,beta,gamma) normalized to unity for convenience
  Acts::Vector3 abg = ba.cross(ca).normalized();
  Acts::ActsScalar delta = -1. * abg.dot(a);

  // plane (alpha*x + beta*y + gamma*z + delta = 0), split to {{alpha, beta,
  // gamma}, delta} for convenience
  return {abg, delta};
}

template <typename spacepoint_t>
Acts::Vector3
Acts::SingleSeedVertexFinder<spacepoint_t>::findClosestPointFromPlanes(
    const std::vector<Acts::SingleSeedVertexFinder<spacepoint_t>::Triplet>&
        triplets) const {
  // 1. define function f = sum over all triplets [distance from an unknown
  // point
  //    (x_0,y_0,z_0) to the plane defined by the triplet]
  // 2. find minimum of "f" by partial derivations over x_0, y_0, and z_0
  // 3. each derivation has parts linearly depending on x_0, y_0, and z_0
  //    (will fill A[deriv][3]) or to nothing (will fill B[deriv])
  // 4. solve A*(x_0,y_0,z_0) = B

  Acts::Vector3 vtx = Acts::Vector3::Zero();
  Acts::Vector3 vtxPrev{m_cfg.rMaxFar, m_cfg.rMaxFar, m_cfg.maxAbsZ};

  // (alpha-beta-gamma, delta), distance
  std::vector<
      std::pair<std::pair<Acts::Vector3, Acts::ActsScalar>, Acts::ActsScalar>>
      tripletsWithPlanes;
  tripletsWithPlanes.reserve(triplets.size());

  for (const auto& triplet : triplets) {
    auto abgd = makePlaneFromTriplet(triplet);
    tripletsWithPlanes.emplace_back(abgd, -1.);
  }

  // elements of the linear equations to solve
  Acts::SquareMatrix3 A = Acts::SquareMatrix3::Zero();
  Acts::Vector3 B = Acts::Vector3::Zero();
  for (const auto& triplet : tripletsWithPlanes) {
    const auto& abg = triplet.first.first;
    const auto& delta = triplet.first.second;

    A += 2. * (abg * abg.transpose());
    B -= 2. * delta * abg;
  }

  for (std::uint32_t iter = 0; iter <= m_cfg.maxIterations; iter++) {
    // new vertex position
    vtx = A.lu().solve(B);

    Acts::Vector3 vtxDiff = vtx - vtxPrev;

    if (vtxDiff.norm() < m_cfg.minVtxShift) {
      // difference between the new vertex and the old vertex is not so large
      break;
    }

    if (iter != m_cfg.maxIterations) {
      // is not the last iteration
      vtxPrev = vtx;

      for (auto& triplet : tripletsWithPlanes) {
        const auto& abg = triplet.first.first;
        const auto& delta = triplet.first.second;
        Acts::ActsScalar distance = std::abs(abg.dot(vtx) + delta);
        triplet.second = distance;
      }

      std::ranges::sort(tripletsWithPlanes, {},
                        [](const auto& t) { return t.second; });

      std::uint32_t threshold = static_cast<std::uint32_t>(
          tripletsWithPlanes.size() * (1. - m_cfg.removeFraction));

      for (std::uint32_t tr = threshold + 1; tr < tripletsWithPlanes.size();
           ++tr) {
        const auto& abg = tripletsWithPlanes[tr].first.first;
        const auto& delta = tripletsWithPlanes[tr].first.second;

        // remove this triplet from A and B
        A -= 2. * (abg * abg.transpose());
        B += 2. * delta * abg;
      }

      // remove all excessive triplets
      tripletsWithPlanes.resize(threshold);
    }
  }

  return vtx;
}

template <typename spacepoint_t>
Acts::Ray3D Acts::SingleSeedVertexFinder<spacepoint_t>::makeRayFromTriplet(
    const Acts::SingleSeedVertexFinder<spacepoint_t>::Triplet& triplet) {
  Acts::SquareMatrix3 mat;
  mat.row(0) = Acts::Vector3(triplet.a.x(), triplet.a.y(), triplet.a.z());
  mat.row(1) = Acts::Vector3(triplet.b.x(), triplet.b.y(), triplet.b.z());
  mat.row(2) = Acts::Vector3(triplet.c.x(), triplet.c.y(), triplet.c.z());

  Acts::Vector3 mean = mat.colwise().mean();
  Acts::SquareMatrix3 cov = (mat.rowwise() - mean.transpose()).transpose() *
                            (mat.rowwise() - mean.transpose()) / 3.;

  // "cov" is self-adjoint matrix
  Eigen::SelfAdjointEigenSolver<Acts::SquareMatrix3> saes(cov);
  // eigenvalues are sorted in increasing order
  Acts::Vector3 eivec = saes.eigenvectors().col(2);

  return {mean, eivec};
}

template <typename spacepoint_t>
Acts::Vector3
Acts::SingleSeedVertexFinder<spacepoint_t>::findClosestPointFromRays(
    const std::vector<Acts::SingleSeedVertexFinder<spacepoint_t>::Triplet>&
        triplets) const {
  // 1. define function f = sum over all triplets [distance from an unknown
  // point
  //    (x_0,y_0,z_0) to the ray defined by the triplet]
  // 2. find minimum of "f" by partial derivations over x_0, y_0, and z_0
  // 3. each derivation has parts linearly depending on x_0, y_0, and z_0
  //    (will fill A[][3]) or to nothing (will fill B[])
  // 4. solve A*(x_0,y_0,z_0) = B

  Acts::Vector3 vtx = Acts::Vector3::Zero();
  Acts::Vector3 vtxPrev{m_cfg.rMaxFar, m_cfg.rMaxFar, m_cfg.maxAbsZ};

  // (startPoint, direction), distance
  std::vector<
      std::pair<std::pair<Acts::Vector3, Acts::Vector3>, Acts::ActsScalar>>
      tripletsWithRays;
  tripletsWithRays.reserve(triplets.size());

  for (const auto& triplet : triplets) {
    tripletsWithRays.emplace_back(
        std::make_pair(triplet.ray.origin(), triplet.ray.dir()), -1.);
  }

  // elements of the linear equations to solve
  Acts::SquareMatrix3 A =
      Acts::SquareMatrix3::Identity() * 2. * triplets.size();
  Acts::Vector3 B = Acts::Vector3::Zero();
  for (const auto& triplet : tripletsWithRays) {
    // use ray saved from earlier
    const auto& startPoint = triplet.first.first;
    const auto& direction = triplet.first.second;

    A -= 2. * (direction * direction.transpose());
    B += -2. * direction * (direction.dot(startPoint)) + 2. * startPoint;
  }

  for (std::uint32_t iter = 0; iter <= m_cfg.maxIterations; iter++) {
    // new vertex position
    vtx = A.lu().solve(B);

    Acts::Vector3 vtxDiff = vtx - vtxPrev;

    if (vtxDiff.norm() < m_cfg.minVtxShift) {
      // difference between the new vertex and the old vertex is not so large
      break;
    }

    if (iter != m_cfg.maxIterations) {
      // is not the last iteration
      vtxPrev = vtx;

      for (auto& triplet : tripletsWithRays) {
        const auto& startPoint = triplet.first.first;
        const auto& direction = triplet.first.second;
        Acts::ActsScalar distance = (vtx - startPoint).cross(direction).norm();
        triplet.second = distance;
      }

      std::ranges::sort(tripletsWithRays, {},
                        [](const auto& t) { return t.second; });

      std::uint32_t threshold = static_cast<std::uint32_t>(
          tripletsWithRays.size() * (1. - m_cfg.removeFraction));

      for (std::uint32_t tr = threshold + 1; tr < tripletsWithRays.size();
           ++tr) {
        const auto& startPoint = tripletsWithRays[tr].first.first;
        const auto& direction = tripletsWithRays[tr].first.second;

        // remove this triplet from A and B
        A -= Acts::SquareMatrix3::Identity() * 2.;
        A += 2. * (direction * direction.transpose());
        B -= -2. * direction * (direction.dot(startPoint)) + 2. * startPoint;
      }

      // remove all excessive triplets
      tripletsWithRays.resize(threshold);
    }
  }

  return vtx;
}
