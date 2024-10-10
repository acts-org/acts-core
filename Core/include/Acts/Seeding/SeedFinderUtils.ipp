// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

namespace Acts {
template <typename external_spacepoint_t, typename callable_t>
inline LinCircle transformCoordinates(Acts::SpacePointMutableData& mutableData,
                                      const external_spacepoint_t& sp,
                                      const external_spacepoint_t& spM,
                                      bool bottom,
                                      callable_t&& extractFunction) {
  // The computation inside this function is exactly identical to that in the
  // vectorized version of this function, except that it operates on a single
  // spacepoint. Please see the other version of this function for more
  // detailed comments.

  auto [xM, yM, zM, rM, varianceRM, varianceZM] = extractFunction(spM);
  auto [xSP, ySP, zSP, rSP, varianceRSP, varianceZSP] = extractFunction(sp);

  float cosPhiM = xM / rM;
  float sinPhiM = yM / rM;
  float deltaX = xSP - xM;
  float deltaY = ySP - yM;
  float deltaZ = zSP - zM;
  float xNewFrame = deltaX * cosPhiM + deltaY * sinPhiM;
  float yNewFrame = deltaY * cosPhiM - deltaX * sinPhiM;
  float deltaR2 = (xNewFrame * xNewFrame + yNewFrame * yNewFrame);
  float iDeltaR2 = 1. / (deltaX * deltaX + deltaY * deltaY);
  float iDeltaR = std::sqrt(iDeltaR2);
  int bottomFactor = bottom ? -1 : 1;
  float cotTheta = deltaZ * iDeltaR * bottomFactor;

  // conformal transformation u=x/(x²+y²) v=y/(x²+y²) transform the
  // circle into straight lines in the u/v plane the line equation can
  // be described in terms of aCoef and bCoef, where v = aCoef * u +
  // bCoef
  const float U = xNewFrame * iDeltaR2;
  const float V = yNewFrame * iDeltaR2;

  // error term for sp-pair without correlation of middle space point
  const float Er = ((varianceZM + varianceZSP) +
                    (cotTheta * cotTheta) * (varianceRM + varianceRSP)) *
                   iDeltaR2;

  mutableData.setDeltaR(sp.index(), std::sqrt(deltaR2 + (deltaZ * deltaZ)));
  return LinCircle(cotTheta, iDeltaR, Er, U, V, xNewFrame, yNewFrame);
}

template <typename external_spacepoint_t>
inline void transformCoordinates(
    Acts::SpacePointMutableData& mutableData,
    const std::vector<const external_spacepoint_t*>& vec,
    const external_spacepoint_t& spM, bool bottom,
    std::vector<LinCircle>& linCircleVec) {
  const float xM = spM.x();
  const float yM = spM.y();
  const float zM = spM.z();
  const float rM = spM.radius();
  const float varianceRM = spM.varianceR();
  const float varianceZM = spM.varianceZ();

  // resize + operator[] is faster than reserve and push_back
  linCircleVec.resize(vec.size());

  float cosPhiM = xM / rM;
  float sinPhiM = yM / rM;

  int bottomFactor = bottom ? -1 : 1;

  for (std::size_t idx(0); idx < vec.size(); ++idx) {
    const external_spacepoint_t* sp = vec[idx];

    const float xSP = sp->x();
    const float ySP = sp->y();
    const float zSP = sp->z();
    const float varianceRSP = sp->varianceR();
    const float varianceZSP = sp->varianceZ();

    const float deltaX = xSP - xM;
    const float deltaY = ySP - yM;
    const float deltaZ = zSP - zM;
    // calculate projection fraction of spM->sp vector pointing in same
    // direction as
    // vector origin->spM (x) and projection fraction of spM->sp vector pointing
    // orthogonal to origin->spM (y)
    const float xNewFrame = deltaX * cosPhiM + deltaY * sinPhiM;
    const float yNewFrame = deltaY * cosPhiM - deltaX * sinPhiM;
    // 1/(length of M -> SP)
    const float deltaR2 = (xNewFrame * xNewFrame + yNewFrame * yNewFrame);
    const float iDeltaR2 = 1. / deltaR2;
    const float iDeltaR = std::sqrt(iDeltaR2);
    //
    // cot_theta = (deltaZ/deltaR)
    const float cotTheta = deltaZ * iDeltaR * bottomFactor;
    // transformation of circle equation (x,y) into linear equation (u,v)
    // x^2 + y^2 - 2x_0*x - 2y_0*y = 0
    // is transformed into
    // 1 - 2x_0*u - 2y_0*v = 0
    // using the following m_U and m_V
    // (u = A + B*v); A and B are created later on
    const float U = xNewFrame * iDeltaR2;
    const float V = yNewFrame * iDeltaR2;
    // error term for sp-pair without correlation of middle space point
    const float Er = ((varianceZM + varianceZSP) +
                      (cotTheta * cotTheta) * (varianceRM + varianceRSP)) *
                     iDeltaR2;

    // Fill Line Circle
    linCircleVec[idx].cotTheta = cotTheta;
    linCircleVec[idx].iDeltaR = iDeltaR;
    linCircleVec[idx].Er = Er;
    linCircleVec[idx].U = U;
    linCircleVec[idx].V = V;
    linCircleVec[idx].x = xNewFrame;
    linCircleVec[idx].y = yNewFrame;
    mutableData.setDeltaR(sp->index(), std::sqrt(deltaR2 + (deltaZ * deltaZ)));
  }
}

template <typename external_spacepoint_t>
inline bool xyzCoordinateCheck(
    const Acts::SeedFinderConfig<external_spacepoint_t>& m_config,
    const external_spacepoint_t& sp, const double* spacepointPosition,
    double* outputCoordinates) {
  // check the compatibility of SPs coordinates in xyz assuming the
  // Bottom-Middle direction with the strip measurement details

  using namespace Acts::HashedStringLiteral;
  const Acts::Vector3& topStripVector = sp.topStripVector();
  const Acts::Vector3& bottomStripVector = sp.bottomStripVector();
  const Acts::Vector3& stripCenterDistance = sp.stripCenterDistance();

  const double xTopStripVector = topStripVector[0];
  const double yTopStripVector = topStripVector[1];
  const double zTopStripVector = topStripVector[2];
  const double xBottomStripVector = bottomStripVector[0];
  const double yBottomStripVector = bottomStripVector[1];
  const double zBottomStripVector = bottomStripVector[2];

  // cross product between top strip vector and spacepointPosition
  double d1[3] = {yTopStripVector * spacepointPosition[2] -
                      zTopStripVector * spacepointPosition[1],
                  zTopStripVector * spacepointPosition[0] -
                      xTopStripVector * spacepointPosition[2],
                  xTopStripVector * spacepointPosition[1] -
                      yTopStripVector * spacepointPosition[0]};

  // scalar product between bottom strip vector and d1
  double bd1 = xBottomStripVector * d1[0] + yBottomStripVector * d1[1] +
               zBottomStripVector * d1[2];

  // compatibility check using distance between strips to evaluate if
  // spacepointPosition is inside the bottom detector element
  double s1 = (stripCenterDistance[0] * d1[0] + stripCenterDistance[1] * d1[1] +
               stripCenterDistance[2] * d1[2]);
  if (std::abs(s1) > std::abs(bd1) * m_config.toleranceParam) {
    return false;
  }

  // cross product between bottom strip vector and spacepointPosition
  double d0[3] = {yBottomStripVector * spacepointPosition[2] -
                      zBottomStripVector * spacepointPosition[1],
                  zBottomStripVector * spacepointPosition[0] -
                      xBottomStripVector * spacepointPosition[2],
                  xBottomStripVector * spacepointPosition[1] -
                      yBottomStripVector * spacepointPosition[0]};

  // compatibility check using distance between strips to evaluate if
  // spacepointPosition is inside the top detector element
  double s0 = (stripCenterDistance[0] * d0[0] + stripCenterDistance[1] * d0[1] +
               stripCenterDistance[2] * d0[2]);
  if (std::abs(s0) > std::abs(bd1) * m_config.toleranceParam) {
    return false;
  }

  // if arrive here spacepointPosition is compatible with strip directions and
  // detector elements

  const Acts::Vector3& topStripCenterPosition = sp.topStripCenterPosition();

  // spacepointPosition corrected with respect to the top strip position and
  // direction and the distance between the strips
  s0 = s0 / bd1;
  outputCoordinates[0] = topStripCenterPosition[0] + xTopStripVector * s0;
  outputCoordinates[1] = topStripCenterPosition[1] + yTopStripVector * s0;
  outputCoordinates[2] = topStripCenterPosition[2] + zTopStripVector * s0;
  return true;
}
}  // namespace Acts
