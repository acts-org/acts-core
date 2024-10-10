// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/Extent.hpp"
#include "Acts/Seeding/SeedFilter.hpp"
#include "Acts/Seeding/SeedFinder.hpp"
#include "Acts/Seeding/SeedFinderOrthogonalConfig.hpp"
#include "Acts/Seeding/SeedFinderUtils.hpp"
#include "Acts/Utilities/BinningType.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <numeric>
#include <type_traits>

namespace Acts {
template <typename external_spacepoint_t>
auto SeedFinderOrthogonal<external_spacepoint_t>::validTupleOrthoRangeLH(
    const external_spacepoint_t &low) const -> typename tree_t::range_t {
  float colMin = m_config.collisionRegionMin;
  float colMax = m_config.collisionRegionMax;
  float pL = low.phi();
  float rL = low.radius();
  float zL = low.z();

  typename tree_t::range_t res;

  /*
   * Cut: Ensure that we search only in φ_min ≤ φ ≤ φ_max, as defined by the
   * seeding configuration.
   */
  res[DimPhi].shrinkMin(m_config.phiMin);
  res[DimPhi].shrinkMax(m_config.phiMax);

  /*
   * Cut: Ensure that we search only in r ≤ r_max, as defined by the seeding
   * configuration.
   */
  res[DimR].shrinkMax(m_config.rMax);

  /*
   * Cut: Ensure that we search only in z_min ≤ z ≤ z_max, as defined by the
   * seeding configuration.
   */
  res[DimZ].shrinkMin(m_config.zMin);
  res[DimZ].shrinkMax(m_config.zMax);

  /*
   * Cut: Ensure that we search only in Δr_min ≤ r - r_L ≤ Δr_max, as defined
   * by the seeding configuration and the given lower spacepoint.
   */
  res[DimR].shrinkMin(rL + m_config.deltaRMinTopSP);
  res[DimR].shrinkMax(rL + m_config.deltaRMaxTopSP);

  /*
   * Cut: Now that we have constrained r, we can use that new r range to
   * further constrain z.
   */
  float zMax = (res[DimR].max() / rL) * (zL - colMin) + colMin;
  float zMin = colMax - (res[DimR].max() / rL) * (colMax - zL);

  /*
   * This cut only works if z_low is outside the collision region for z.
   */
  if (zL > colMin) {
    res[DimZ].shrinkMax(zMax);
  } else if (zL < colMax) {
    res[DimZ].shrinkMin(zMin);
  }

  /*
   * Cut: Shrink the z-range using the maximum cot(θ).
   */
  res[DimZ].shrinkMin(zL - m_config.cotThetaMax * (res[DimR].max() - rL));
  res[DimZ].shrinkMax(zL + m_config.cotThetaMax * (res[DimR].max() - rL));

  /*
   * Cut: Shrink the φ range, such that Δφ_min ≤ φ - φ_L ≤ Δφ_max
   */
  res[DimPhi].shrinkMin(pL - m_config.deltaPhiMax);
  res[DimPhi].shrinkMax(pL + m_config.deltaPhiMax);

  // Cut: Ensure that z-distance between SPs is within max and min values.
  res[DimZ].shrinkMin(zL - m_config.deltaZMax);
  res[DimZ].shrinkMax(zL + m_config.deltaZMax);

  return res;
}

template <typename external_spacepoint_t>
auto SeedFinderOrthogonal<external_spacepoint_t>::validTupleOrthoRangeHL(
    const external_spacepoint_t &high) const -> typename tree_t::range_t {
  float pM = high.phi();
  float rM = high.radius();
  float zM = high.z();

  typename tree_t::range_t res;

  /*
   * Cut: Ensure that we search only in φ_min ≤ φ ≤ φ_max, as defined by the
   * seeding configuration.
   */
  res[DimPhi].shrinkMin(m_config.phiMin);
  res[DimPhi].shrinkMax(m_config.phiMax);

  /*
   * Cut: Ensure that we search only in r ≤ r_max, as defined by the seeding
   * configuration.
   */
  res[DimR].shrinkMax(m_config.rMax);

  /*
   * Cut: Ensure that we search only in z_min ≤ z ≤ z_max, as defined by the
   * seeding configuration.
   */
  res[DimZ].shrinkMin(m_config.zMin);
  res[DimZ].shrinkMax(m_config.zMax);

  /*
   * Cut: Ensure that we search only in Δr_min ≤ r_H - r ≤ Δr_max, as defined
   * by the seeding configuration and the given higher spacepoint.
   */
  res[DimR].shrinkMin(rM - m_config.deltaRMaxBottomSP);
  res[DimR].shrinkMax(rM - m_config.deltaRMinBottomSP);

  /*
   * Cut: Now that we have constrained r, we can use that new r range to
   * further constrain z.
   */
  float fracR = res[DimR].min() / rM;

  float zMin =
      (zM - m_config.collisionRegionMin) * fracR + m_config.collisionRegionMin;
  float zMax =
      (zM - m_config.collisionRegionMax) * fracR + m_config.collisionRegionMax;

  res[DimZ].shrinkMin(std::min(zMin, zM));
  res[DimZ].shrinkMax(std::max(zMax, zM));

  /*
   * Cut: Shrink the φ range, such that Δφ_min ≤ φ - φ_H ≤ Δφ_max
   */
  res[DimPhi].shrinkMin(pM - m_config.deltaPhiMax);
  res[DimPhi].shrinkMax(pM + m_config.deltaPhiMax);

  // Cut: Ensure that z-distance between SPs is within max and min values.
  res[DimZ].shrinkMin(zM - m_config.deltaZMax);
  res[DimZ].shrinkMax(zM + m_config.deltaZMax);

  return res;
}

template <typename external_spacepoint_t>
bool SeedFinderOrthogonal<external_spacepoint_t>::validTuple(
    const SeedFinderOptions &options, const external_spacepoint_t &low,
    const external_spacepoint_t &high, bool isMiddleInverted) const {
  float rL = low.radius();
  float rH = high.radius();

  float zL = low.z();
  float zH = high.z();

  float deltaR = rH - rL;

  float deltaZ = (zH - zL);
  float cotTheta = deltaZ / deltaR;
  /*
   * Cut: Ensure that the origin of the dublet (the intersection of the line
   * between them with the z axis) lies within the collision region.
   */
  float zOrigin = zL - rL * cotTheta;
  if (zOrigin < m_config.collisionRegionMin ||
      zOrigin > m_config.collisionRegionMax) {
    return false;
  }

  // cut on the max curvature calculated from dublets
  // first transform the space point coordinates into a frame such that the
  // central space point SPm is in the origin of the frame and the x axis
  // points away from the interaction point in addition to a translation
  // transformation we also perform a rotation in order to keep the
  // curvature of the circle tangent to the x axis
  if (m_config.interactionPointCut) {
    float xVal = (high.x() - low.x()) * (low.x() / rL) +
                 (high.y() - low.y()) * (low.y() / rL);
    float yVal = (high.y() - low.y()) * (low.x() / rL) -
                 (high.x() - low.x()) * (low.y() / rL);

    const int sign = isMiddleInverted ? -1 : 1;

    if (std::abs(rL * yVal) > sign * m_config.impactMax * xVal) {
      // conformal transformation u=x/(x²+y²) v=y/(x²+y²) transform the
      // circle into straight lines in the u/v plane the line equation can
      // be described in terms of aCoef and bCoef, where v = aCoef * u +
      // bCoef
      float uT = xVal / (xVal * xVal + yVal * yVal);
      float vT = yVal / (xVal * xVal + yVal * yVal);
      // in the rotated frame the interaction point is positioned at x = -rM
      // and y ~= impactParam
      float uIP = -1. / rL;
      float vIP = m_config.impactMax / (rL * rL);
      if (sign * yVal > 0.) {
        vIP = -vIP;
      }
      // we can obtain aCoef as the slope dv/du of the linear function,
      // estimated using du and dv between the two SP bCoef is obtained by
      // inserting aCoef into the linear equation
      float aCoef = (vT - vIP) / (uT - uIP);
      float bCoef = vIP - aCoef * uIP;
      // the distance of the straight line from the origin (radius of the
      // circle) is related to aCoef and bCoef by d^2 = bCoef^2 / (1 +
      // aCoef^2) = 1 / (radius^2) and we can apply the cut on the curvature
      if ((bCoef * bCoef) > (1 + aCoef * aCoef) / options.minHelixDiameter2) {
        return false;
      }
    }
  }

  /*
   * Cut: Ensure that the forward angle (z / r) lies within reasonable bounds,
   * which is to say the absolute value must be smaller than the max cot(θ).
   */
  if (std::fabs(cotTheta) > m_config.cotThetaMax) {
    return false;
  }

  /*
   * Cut: Ensure that inner-middle dublet is in a certain (r, eta) region of the
   * detector according to detector specific cuts.
   */
  const float rInner = (isMiddleInverted) ? rH : rL;
  if (!m_config.experimentCuts(rInner, cotTheta)) {
    return false;
  }

  return true;
}

template <typename external_spacepoint_t>
SeedFinderOrthogonal<external_spacepoint_t>::SeedFinderOrthogonal(
    const SeedFinderOrthogonalConfig<external_spacepoint_t> &config)
    : m_config(config) {
  if (!config.isInInternalUnits) {
    throw std::runtime_error(
        "SeedFinderOrthogonalConfig not in ACTS internal units in "
        "SeedFinderOrthogonal");
  }
}

template <typename external_spacepoint_t>
void SeedFinderOrthogonal<external_spacepoint_t>::filterCandidates(
    const SeedFinderOptions &options, Acts::SpacePointMutableData &mutableData,
    const external_spacepoint_t &middle,
    const std::vector<const external_spacepoint_t *> &bottom,
    const std::vector<const external_spacepoint_t *> &top,
    SeedFilterState seedFilterState,
    CandidatesForMiddleSp<const external_spacepoint_t> &candidates_collector)
    const {
  float rM = middle.radius();
  float zM = middle.z();
  float varianceRM = middle.varianceR();
  float varianceZM = middle.varianceZ();

  // apply cut on the number of top SP if seedConfirmation is true
  if (m_config.seedConfirmation == true) {
    // check if middle SP is in the central or forward region
    SeedConfirmationRangeConfig seedConfRange =
        (zM > m_config.centralSeedConfirmationRange.zMaxSeedConf ||
         zM < m_config.centralSeedConfirmationRange.zMinSeedConf)
            ? m_config.forwardSeedConfirmationRange
            : m_config.centralSeedConfirmationRange;
    // set the minimum number of top SP depending on whether the middle SP is
    // in the central or forward region
    seedFilterState.nTopSeedConf = rM > seedConfRange.rMaxSeedConf
                                       ? seedConfRange.nTopForLargeR
                                       : seedConfRange.nTopForSmallR;
    // set max bottom radius for seed confirmation
    seedFilterState.rMaxSeedConf = seedConfRange.rMaxSeedConf;
    // continue if number of top SPs is smaller than minimum
    if (top.size() < seedFilterState.nTopSeedConf) {
      return;
    }
  }

  std::vector<const external_spacepoint_t *> top_valid;
  std::vector<float> curvatures;
  std::vector<float> impactParameters;

  // contains parameters required to calculate circle with linear equation
  // ...for bottom-middle
  std::vector<LinCircle> linCircleBottom;
  // ...for middle-top
  std::vector<LinCircle> linCircleTop;

  // transform coordinates
  transformCoordinates(mutableData, bottom, middle, true, linCircleBottom);
  transformCoordinates(mutableData, top, middle, false, linCircleTop);

  // sort: make index vector
  std::vector<std::size_t> sorted_bottoms(linCircleBottom.size());
  for (std::size_t i(0); i < sorted_bottoms.size(); ++i) {
    sorted_bottoms[i] = i;
  }

  std::vector<std::size_t> sorted_tops(linCircleTop.size());
  for (std::size_t i(0); i < sorted_tops.size(); ++i) {
    sorted_tops[i] = i;
  }

  std::ranges::sort(sorted_bottoms, {},
                    [&linCircleBottom](const std::size_t s) {
                      return linCircleBottom[s].cotTheta;
                    });

  std::ranges::sort(sorted_tops, {}, [&linCircleTop](const std::size_t s) {
    return linCircleTop[s].cotTheta;
  });

  std::vector<float> tanMT;
  tanMT.reserve(top.size());

  std::size_t numTopSP = top.size();
  for (std::size_t t = 0; t < numTopSP; t++) {
    tanMT.push_back(
        std::atan2(top[t]->radius() - middle.radius(), top[t]->z() - zM));
  }

  std::size_t t0 = 0;

  for (const std::size_t b : sorted_bottoms) {
    // break if we reached the last top SP
    if (t0 == numTopSP) {
      break;
    }

    auto lb = linCircleBottom[b];

    // 1+(cot^2(theta)) = 1/sin^2(theta)
    float iSinTheta2 = (1. + lb.cotTheta * lb.cotTheta);
    float sigmaSquaredPtDependent = iSinTheta2 * options.sigmapT2perRadius;
    // calculate max scattering for min momentum at the seed's theta angle
    // scaling scatteringAngle^2 by sin^2(theta) to convert pT^2 to p^2
    // accurate would be taking 1/atan(thetaBottom)-1/atan(thetaTop) <
    // scattering
    // but to avoid trig functions we approximate cot by scaling by
    // 1/sin^4(theta)
    // resolving with pT to p scaling --> only divide by sin^2(theta)
    // max approximation error for allowed scattering angles of 0.04 rad at
    // eta=infinity: ~8.5%
    float scatteringInRegion2 = options.multipleScattering2 * iSinTheta2;

    // minimum number of compatible top SPs to trigger the filter for a certain
    // middle bottom pair if seedConfirmation is false we always ask for at
    // least one compatible top to trigger the filter
    std::size_t minCompatibleTopSPs = 2;
    if (!m_config.seedConfirmation ||
        bottom[b]->radius() > seedFilterState.rMaxSeedConf) {
      minCompatibleTopSPs = 1;
    }
    if (m_config.seedConfirmation &&
        candidates_collector.nHighQualityCandidates()) {
      minCompatibleTopSPs++;
    }

    // clear all vectors used in each inner for loop
    top_valid.clear();
    curvatures.clear();
    impactParameters.clear();

    float tanLM = std::atan2(rM - bottom[b]->radius(), zM - bottom[b]->z());

    for (std::size_t index_t = t0; index_t < numTopSP; index_t++) {
      const std::size_t t = sorted_tops[index_t];
      auto lt = linCircleTop[t];

      if (std::abs(tanLM - tanMT[t]) > 0.005) {
        continue;
      }

      // add errors of spB-spM and spM-spT pairs and add the correlation term
      // for errors on spM
      float error2 = lt.Er + lb.Er +
                     2 * (lb.cotTheta * lt.cotTheta * varianceRM + varianceZM) *
                         lb.iDeltaR * lt.iDeltaR;

      float deltaCotTheta = lb.cotTheta - lt.cotTheta;
      float deltaCotTheta2 = deltaCotTheta * deltaCotTheta;

      // Apply a cut on the compatibility between the r-z slope of the two
      // seed segments. This is done by comparing the squared difference
      // between slopes, and comparing to the squared uncertainty in this
      // difference - we keep a seed if the difference is compatible within
      // the assumed uncertainties. The uncertainties get contribution from
      // the  space-point-related squared error (error2) and a scattering term
      // calculated assuming the minimum pt we expect to reconstruct
      // (scatteringInRegion2). This assumes gaussian error propagation which
      // allows just adding the two errors if they are uncorrelated (which is
      // fair for scattering and measurement uncertainties)
      if (deltaCotTheta2 > (error2 + scatteringInRegion2)) {
        // skip top SPs based on cotTheta sorting when producing triplets
        // break if cotTheta from bottom SP < cotTheta from top SP because
        // the SP are sorted by cotTheta
        if (deltaCotTheta < 0) {
          break;
        }
        t0 = index_t + 1;
        continue;
      }

      float dU = lt.U - lb.U;

      // A and B are evaluated as a function of the circumference parameters
      // x_0 and y_0
      float A = (lt.V - lb.V) / dU;
      float S2 = 1. + A * A;
      float B = lb.V - A * lb.U;
      float B2 = B * B;
      // sqrt(S2)/B = 2 * helixradius
      // calculated radius must not be smaller than minimum radius
      if (S2 < B2 * options.minHelixDiameter2) {
        continue;
      }

      // 1/helixradius: (B/sqrt(S2))*2 (we leave everything squared)
      // convert p(T) to p scaling by sin^2(theta) AND scale by 1/sin^4(theta)
      // from rad to deltaCotTheta
      float p2scatterSigma = B2 / S2 * sigmaSquaredPtDependent;
      if (!std::isinf(m_config.maxPtScattering)) {
        // if pT > maxPtScattering, calculate allowed scattering angle using
        // maxPtScattering instead of pt.
        if (B2 == 0 || options.pTPerHelixRadius * std::sqrt(S2 / B2) >
                           2. * m_config.maxPtScattering) {
          float pTscatterSigma =
              (m_config.highland / m_config.maxPtScattering) *
              m_config.sigmaScattering;
          p2scatterSigma = pTscatterSigma * pTscatterSigma * iSinTheta2;
        }
      }
      // if deltaTheta larger than allowed scattering for calculated pT, skip
      if (deltaCotTheta2 > (error2 + p2scatterSigma)) {
        if (deltaCotTheta < 0) {
          break;
        }
        t0 = index_t;
        continue;
      }

      // A and B allow calculation of impact params in U/V plane with linear
      // function
      // (in contrast to having to solve a quadratic function in x/y plane)
      float Im = std::abs((A - B * rM) * rM);

      if (Im <= m_config.impactMax) {
        top_valid.push_back(top[t]);
        // inverse diameter is signed depending on if the curvature is
        // positive/negative in phi
        curvatures.push_back(B / std::sqrt(S2));
        impactParameters.push_back(Im);
      }
    }

    // continue if number of top SPs is smaller than minimum required for filter
    if (top_valid.size() < minCompatibleTopSPs) {
      continue;
    }

    seedFilterState.zOrigin = zM - rM * lb.cotTheta;

    m_config.seedFilter->filterSeeds_2SpFixed(
        mutableData, *bottom[b], middle, top_valid, curvatures,
        impactParameters, seedFilterState, candidates_collector);

  }  // loop on bottoms
}

template <typename external_spacepoint_t>
template <typename output_container_t>
void SeedFinderOrthogonal<external_spacepoint_t>::processFromMiddleSP(
    const SeedFinderOptions &options, Acts::SpacePointMutableData &mutableData,
    const tree_t &tree, output_container_t &out_cont,
    const typename tree_t::pair_t &middle_p) const {
  using range_t = typename tree_t::range_t;
  const external_spacepoint_t &middle = *middle_p.second;

  /*
   * Prepare four output vectors for seed candidates:
   *
   * bottom_lh_v denotes the candidates bottom seed points, assuming that the
   * track has monotonically _increasing_ z position. bottom_hl_v denotes the
   * candidate bottom points assuming that the track has monotonically
   * _decreasing_ z position. top_lh_v are the candidate top points for an
   * increasing z track, and top_hl_v are the candidate top points for a
   * decreasing z track.
   */
  std::vector<const external_spacepoint_t *> bottom_lh_v, bottom_hl_v, top_lh_v,
      top_hl_v;

  /*
   * Storage for seed candidates
   */
  std::size_t max_num_quality_seeds_per_spm =
      m_config.seedFilter->getSeedFilterConfig().maxQualitySeedsPerSpMConf;
  std::size_t max_num_seeds_per_spm =
      m_config.seedFilter->getSeedFilterConfig().maxSeedsPerSpMConf;

  CandidatesForMiddleSp<const external_spacepoint_t> candidates_collector;
  candidates_collector.setMaxElements(max_num_seeds_per_spm,
                                      max_num_quality_seeds_per_spm);

  /*
   * Calculate the search ranges for bottom and top candidates for this middle
   * space point.
   */
  range_t bottom_r = validTupleOrthoRangeHL(middle);
  range_t top_r = validTupleOrthoRangeLH(middle);

  /*
   * Calculate the value of cot(θ) for this middle spacepoint.
   */
  float myCotTheta =
      std::max(std::abs(middle.z() / middle.radius()), m_config.cotThetaMax);

  /*
   * Calculate the maximum Δr, given that we have already constrained our
   * search space.
   */
  float deltaRMaxTop = top_r[DimR].max() - middle.radius();
  float deltaRMaxBottom = middle.radius() - bottom_r[DimR].min();

  /*
   * Create the search range for the bottom spacepoint assuming a
   * monotonically increasing z track, by calculating the minimum z value from
   * the cot(θ), and by setting the maximum to the z position of the middle
   * spacepoint - if the z position is higher than the middle point, then it
   * would be a decreasing z track!
   */
  range_t bottom_lh_r = bottom_r;
  bottom_lh_r[DimZ].shrink(middle.z() - myCotTheta * deltaRMaxBottom,
                           middle.z());

  /*
   * Calculate the search ranges for the other four sets of points in a
   * similar fashion.
   */
  range_t top_lh_r = top_r;
  top_lh_r[DimZ].shrink(middle.z(), middle.z() + myCotTheta * deltaRMaxTop);

  range_t bottom_hl_r = bottom_r;
  bottom_hl_r[DimZ].shrink(middle.z(),
                           middle.z() + myCotTheta * deltaRMaxBottom);
  range_t top_hl_r = top_r;
  top_hl_r[DimZ].shrink(middle.z() - myCotTheta * deltaRMaxTop, middle.z());

  /*
   * Make sure the candidate vectors are clear, in case we've used them
   * before.
   */
  bottom_lh_v.clear();
  bottom_hl_v.clear();
  top_lh_v.clear();
  top_hl_v.clear();

  /*
   * Now, we will actually search for the spaces. Remembering that we combine
   * bottom and top candidates for increasing and decreasing tracks
   * separately, we will first check whether both the search ranges for
   * increasing tracks are not degenerate - if they are, we will never find
   * any seeds and we do not need to bother doing the search.
   */
  if (!bottom_lh_r.degenerate() && !top_lh_r.degenerate()) {
    /*
     * Search the trees for points that lie in the given search range.
     */
    tree.rangeSearchMapDiscard(top_lh_r,
                               [this, &options, &middle, &top_lh_v](
                                   const typename tree_t::coordinate_t &,
                                   const typename tree_t::value_t &top) {
                                 if (validTuple(options, *top, middle, true)) {
                                   top_lh_v.push_back(top);
                                 }
                               });
  }

  /*
   * Perform the same search for candidate bottom spacepoints, but for
   * monotonically decreasing z tracks.
   */
  if (!bottom_hl_r.degenerate() && !top_hl_r.degenerate()) {
    tree.rangeSearchMapDiscard(top_hl_r,
                               [this, &options, &middle, &top_hl_v](
                                   const typename tree_t::coordinate_t &,
                                   const typename tree_t::value_t &top) {
                                 if (validTuple(options, middle, *top, false)) {
                                   top_hl_v.push_back(top);
                                 }
                               });
  }

  // apply cut on the number of top SP if seedConfirmation is true
  SeedFilterState seedFilterState;
  bool search_bot_hl = true;
  bool search_bot_lh = true;
  if (m_config.seedConfirmation) {
    // check if middle SP is in the central or forward region
    SeedConfirmationRangeConfig seedConfRange =
        (middle.z() > m_config.centralSeedConfirmationRange.zMaxSeedConf ||
         middle.z() < m_config.centralSeedConfirmationRange.zMinSeedConf)
            ? m_config.forwardSeedConfirmationRange
            : m_config.centralSeedConfirmationRange;
    // set the minimum number of top SP depending on whether the middle SP is
    // in the central or forward region
    seedFilterState.nTopSeedConf = middle.radius() > seedConfRange.rMaxSeedConf
                                       ? seedConfRange.nTopForLargeR
                                       : seedConfRange.nTopForSmallR;
    // set max bottom radius for seed confirmation
    seedFilterState.rMaxSeedConf = seedConfRange.rMaxSeedConf;
    // continue if number of top SPs is smaller than minimum
    if (top_lh_v.size() < seedFilterState.nTopSeedConf) {
      search_bot_lh = false;
    }
    if (top_hl_v.size() < seedFilterState.nTopSeedConf) {
      search_bot_hl = false;
    }
  }

  /*
   * Next, we perform a search for bottom candidates in increasing z tracks,
   * which only makes sense if we found any bottom candidates.
   */
  if (!top_lh_v.empty() && search_bot_lh) {
    tree.rangeSearchMapDiscard(
        bottom_lh_r, [this, &options, &middle, &bottom_lh_v](
                         const typename tree_t::coordinate_t &,
                         const typename tree_t::value_t &bottom) {
          if (validTuple(options, *bottom, middle, false)) {
            bottom_lh_v.push_back(bottom);
          }
        });
  }

  /*
   * And repeat for the top spacepoints for decreasing z tracks!
   */
  if (!top_hl_v.empty() && search_bot_hl) {
    tree.rangeSearchMapDiscard(
        bottom_hl_r, [this, &options, &middle, &bottom_hl_v](
                         const typename tree_t::coordinate_t &,
                         const typename tree_t::value_t &bottom) {
          if (validTuple(options, middle, *bottom, true)) {
            bottom_hl_v.push_back(bottom);
          }
        });
  }

  /*
   * If we have candidates for increasing z tracks, we try to combine them.
   */
  if (!bottom_lh_v.empty() && !top_lh_v.empty()) {
    filterCandidates(options, mutableData, middle, bottom_lh_v, top_lh_v,
                     seedFilterState, candidates_collector);
  }
  /*
   * Try to combine candidates for decreasing z tracks.
   */
  if (!bottom_hl_v.empty() && !top_hl_v.empty()) {
    filterCandidates(options, mutableData, middle, bottom_hl_v, top_hl_v,
                     seedFilterState, candidates_collector);
  }
  /*
   * Run a seed filter, just like in other seeding algorithms.
   */
  if ((!bottom_lh_v.empty() && !top_lh_v.empty()) ||
      (!bottom_hl_v.empty() && !top_hl_v.empty())) {
    m_config.seedFilter->filterSeeds_1SpFixed(mutableData, candidates_collector,
                                              out_cont);
  }
}

template <typename external_spacepoint_t>
auto SeedFinderOrthogonal<external_spacepoint_t>::createTree(
    const std::vector<const external_spacepoint_t *> &spacePoints) const
    -> tree_t {
  std::vector<typename tree_t::pair_t> points;

  /*
   * For every input point, we create a coordinate-pointer pair, which we then
   * linearly pass to the k-d tree constructor. That constructor will take
   * care of sorting the pairs and splitting the space.
   */
  for (const external_spacepoint_t *sp : spacePoints) {
    typename tree_t::coordinate_t point;

    point[DimPhi] = sp->phi();
    point[DimR] = sp->radius();
    point[DimZ] = sp->z();

    points.emplace_back(point, sp);
  }

  return tree_t(std::move(points));
}

template <typename external_spacepoint_t>
template <typename input_container_t, typename output_container_t>
void SeedFinderOrthogonal<external_spacepoint_t>::createSeeds(
    const Acts::SeedFinderOptions &options,
    const input_container_t &spacePoints, output_container_t &out_cont) const {
  if (!options.isInInternalUnits) {
    throw std::runtime_error(
        "SeedFinderOptions not in ACTS internal units in "
        "SeedFinderOrthogonal");
  }
  /*
   * The template parameters we accept are a little too generic, so we want to
   * run some basic checks to make sure the containers have the correct value
   * types.
   */
  static_assert(std::is_same_v<typename output_container_t::value_type,
                               Seed<external_spacepoint_t>>,
                "Output iterator container type must accept seeds.");
  static_assert(std::is_same_v<typename input_container_t::value_type,
                               external_spacepoint_t>,
                "Input container must contain external spacepoints.");

  /*
   * Sadly, for the time being, we will need to construct our internal space
   * points on the heap. This adds some additional overhead and work. Here we
   * take each external spacepoint, allocate a corresponding internal space
   * point, and save it in a vector.
   */
  Acts::Extent rRangeSPExtent;
  std::vector<const external_spacepoint_t *> internal_sps;
  internal_sps.reserve(spacePoints.size());

  Acts::SpacePointMutableData mutableData;
  mutableData.resize(spacePoints.size());

  for (const external_spacepoint_t &p : spacePoints) {
    // store x,y,z values in extent
    rRangeSPExtent.extend({p.x(), p.y(), p.z()});
    internal_sps.push_back(&p);
  }
  // variable middle SP radial region of interest
  const Acts::Range1D<float> rMiddleSPRange(
      std::floor(rRangeSPExtent.min(Acts::BinningValue::binR) / 2) * 2 +
          m_config.deltaRMiddleMinSPRange,
      std::floor(rRangeSPExtent.max(Acts::BinningValue::binR) / 2) * 2 -
          m_config.deltaRMiddleMaxSPRange);

  /*
   * Construct the k-d tree from these points. Note that this not consume or
   * take ownership of the points.
   */
  tree_t tree = createTree(internal_sps);
  /*
   * Run the seeding algorithm by iterating over all the points in the tree
   * and seeing what happens if we take them to be our middle spacepoint.
   */
  for (const typename tree_t::pair_t &middle_p : tree) {
    const external_spacepoint_t &middle = *middle_p.second;
    auto rM = middle.radius();

    /*
     * Cut: Ensure that the middle spacepoint lies within a valid r-region for
     * middle points.
     */
    if (m_config.useVariableMiddleSPRange) {
      if (rM < rMiddleSPRange.min() || rM > rMiddleSPRange.max()) {
        continue;
      }
    } else {
      if (rM > m_config.rMaxMiddle || rM < m_config.rMinMiddle) {
        continue;
      }
    }

    // remove all middle SPs outside phi and z region of interest
    if (middle.z() < m_config.zOutermostLayers.first ||
        middle.z() > m_config.zOutermostLayers.second) {
      continue;
    }
    float spPhi = middle.phi();
    if (spPhi > m_config.phiMax || spPhi < m_config.phiMin) {
      continue;
    }

    processFromMiddleSP(options, mutableData, tree, out_cont, middle_p);
  }
}

template <typename external_spacepoint_t>
template <typename input_container_t>
std::vector<Seed<external_spacepoint_t>>
SeedFinderOrthogonal<external_spacepoint_t>::createSeeds(
    const Acts::SeedFinderOptions &options,
    const input_container_t &spacePoints) const {
  std::vector<seed_t> r;
  createSeeds(options, spacePoints, r);
  return r;
}

}  // namespace Acts
