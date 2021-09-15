// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ActsExamples/TrackFinding/SeedingOrthogonalAlgorithm.hpp"

#include "Acts/Seeding/BinFinder.hpp"
#include "Acts/Seeding/BinnedSPGroup.hpp"
#include "Acts/Seeding/Seed.hpp"
#include "Acts/Seeding/SeedFilter.hpp"
#include "Acts/Seeding/Seedfinder.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/KDTree.hpp"
#include "ActsExamples/EventData/ProtoTrack.hpp"
#include "ActsExamples/EventData/SimSeed.hpp"
#include "ActsExamples/Framework/WhiteBoard.hpp"

#include <stdexcept>

using SP = Acts::InternalSpacePoint<ActsExamples::SimSpacePoint>;
using Protoseed = std::tuple<const SP *, const SP *, const SP *>;
using namespace std::placeholders;

namespace {
void transformCoordinates(std::vector<const SP *> &vec, const SP &spM,
                          bool bottom,
                          std::vector<Acts::LinCircle> &linCircleVec) {
  float xM = spM.x();
  float yM = spM.y();
  float zM = spM.z();
  float rM = spM.radius();
  float varianceZM = spM.varianceZ();
  float varianceRM = spM.varianceR();
  float cosPhiM = xM / rM;
  float sinPhiM = yM / rM;
  for (auto sp : vec) {
    float deltaX = sp->x() - xM;
    float deltaY = sp->y() - yM;
    float deltaZ = sp->z() - zM;
    // calculate projection fraction of spM->sp vector pointing in same
    // direction as
    // vector origin->spM (x) and projection fraction of spM->sp vector pointing
    // orthogonal to origin->spM (y)
    float x = deltaX * cosPhiM + deltaY * sinPhiM;
    float y = deltaY * cosPhiM - deltaX * sinPhiM;
    // 1/(length of M -> SP)
    float iDeltaR2 = 1. / (deltaX * deltaX + deltaY * deltaY);
    float iDeltaR = std::sqrt(iDeltaR2);
    //
    int bottomFactor = 1 * (int(!bottom)) - 1 * (int(bottom));
    // cot_theta = (deltaZ/deltaR)
    float cot_theta = deltaZ * iDeltaR * bottomFactor;
    // VERY frequent (SP^3) access
    Acts::LinCircle l;
    l.cotTheta = cot_theta;
    // location on z-axis of this SP-duplet
    l.Zo = zM - rM * cot_theta;
    l.iDeltaR = iDeltaR;
    // transformation of circle equation (x,y) into linear equation (u,v)
    // x^2 + y^2 - 2x_0*x - 2y_0*y = 0
    // is transformed into
    // 1 - 2x_0*u - 2y_0*v = 0
    // using the following m_U and m_V
    // (u = A + B*v); A and B are created later on
    l.U = x * iDeltaR2;
    l.V = y * iDeltaR2;
    // error term for sp-pair without correlation of middle space point
    l.Er = ((varianceZM + sp->varianceZ()) +
            (cot_theta * cot_theta) * (varianceRM + sp->varianceR())) *
           iDeltaR2;
    linCircleVec.push_back(l);
  }
}

template <typename SP>
Acts::LinCircle _lincircle(const SP &middle, const SP &other, bool bottom) {
  float xM = middle.x();
  float yM = middle.y();
  float zM = middle.z();
  float rM = middle.radius();
  float varianceZM = middle.varianceZ();
  float varianceRM = middle.varianceR();
  float cosPhiM = xM / rM;
  float sinPhiM = yM / rM;

  float deltaX = other.x() - xM;
  float deltaY = other.y() - yM;
  float deltaZ = other.z() - zM;

  float x = deltaX * cosPhiM + deltaY * sinPhiM;
  float y = deltaY * cosPhiM - deltaX * sinPhiM;

  float iDeltaR2 = 1. / (deltaX * deltaX + deltaY * deltaY);
  float iDeltaR = std::sqrt(iDeltaR2);

  int bottomFactor = 1 * (int(!bottom)) - 1 * (int(bottom));

  float cot_theta = deltaZ * iDeltaR * bottomFactor;

  Acts::LinCircle l;
  l.cotTheta = cot_theta;

  l.Zo = zM - rM * cot_theta;
  l.iDeltaR = iDeltaR;

  l.U = x * iDeltaR2;
  l.V = y * iDeltaR2;

  l.Er = ((varianceZM + other.varianceZ()) +
          (cot_theta * cot_theta) * (varianceRM + other.varianceR())) *
         iDeltaR2;
  return l;
}

template <typename SP>
Acts::LinCircle lcBotMid(const SP &bottom, const SP &middle) {
  return _lincircle(middle, bottom, true);
}

template <typename SP>
float maxDeltaR(double r) {
  return 100;
  if (r <= 60) {
    return 40;
  } else if (r <= 220) {
    return 70;
  } else {
    return 100;
  }
}

template <typename SP>
bool validMiddle(const SP &s) {
  if (s.radius() > 120) {
    return false;
  }

  if (s.radius() < 60) {
    return false;
  }

  return true;
}

template <std::size_t N, typename SP>
Acts::RangeXD<N, double> validTupleOrthoRange(
    const SP &low,
    const Acts::SeedfinderConfig<ActsExamples::SimSpacePoint> &config) {
  double colMin = config.collisionRegionMin;
  double colMax = config.collisionRegionMax;
  double pL = low.phi();
  double rL = low.radius();
  double zL = low.z();

  Acts::RangeXD<N, double> res;

  res[0].shrinkMin(config.phiMin);
  res[0].shrinkMax(config.phiMax);

  // res[1].shrinkMin(config.rMin);
  res[1].shrinkMax(config.rMax);

  res[2].shrinkMin(config.zMin);
  res[2].shrinkMax(config.zMax);

  // if constexpr(N >= 6) {
  //   res[5].shrinkMin(-config.cotThetaMax);
  //   res[5].shrinkMax(config.cotThetaMax);
  // }

  res[1].shrinkMin(rL + config.deltaRMin);
  res[1].shrinkMax(rL + std::min(config.deltaRMax, maxDeltaR<SP>(rL)));

  double zMax = (res[1].max() / rL) * (zL - colMin) + colMin;
  double zMin = colMax - (res[1].max() / rL) * (colMax - zL);

  if (zL > colMin) {
    res[2].shrinkMax(zMax);
  }

  if (zL < colMax) {
    res[2].shrinkMin(zMin);
  }

  res[2].shrinkMin(zL - config.cotThetaMax * (res[1].max() - rL));
  res[2].shrinkMax(zL + config.cotThetaMax * (res[1].max() - rL));

  // WARNING: Experimental extra cut.
  double az = (res[1].max() / low.radius()) *
              std::max(std::abs(low.z() - colMin), std::abs(low.z() - colMax));
  double p3 = 0.0015 * std::sqrt(az);
  double delta_phi = std::min(0.085, p3);

  res[0].shrinkMin(pL - delta_phi);
  res[0].shrinkMax(pL + delta_phi);

  return res;
}

template <std::size_t N, typename SP>
Acts::RangeXD<N, double> validMidToLowOrthoRange(
    const SP &mid,
    const Acts::SeedfinderConfig<ActsExamples::SimSpacePoint> &config) {
  float pM = mid.phi();
  float rM = mid.radius();

  Acts::RangeXD<N, double> res;

  res[0].shrinkMin(config.phiMin);
  res[0].shrinkMax(config.phiMax);

  // res[1].shrinkMin(config.rMin);
  res[1].shrinkMax(config.rMax);

  res[2].shrinkMin(config.zMin);
  res[2].shrinkMax(config.zMax);

  // if constexpr(N >= 6) {
  //   res[5].shrinkMin(-config.cotThetaMax);
  //   res[5].shrinkMax(config.cotThetaMax);
  // }

  res[1].shrinkMin(rM - std::min(config.deltaRMax, maxDeltaR<SP>(rM)));
  res[1].shrinkMax(rM - config.deltaRMin);

  double frac_r = res[1].min() / rM;

  float zMin = (mid.z() - config.collisionRegionMin) * frac_r +
               config.collisionRegionMin;
  float zMax = (mid.z() - config.collisionRegionMax) * frac_r +
               config.collisionRegionMax;

  res[2].shrinkMin(std::min(zMin, mid.z()));
  res[2].shrinkMax(std::max(zMax, mid.z()));

  // WARNING: Experimental extra cuts.
  double az = std::max(std::abs(mid.z() - config.collisionRegionMin),
                       std::abs(mid.z() - config.collisionRegionMax));
  double p3 = 0.003 * std::sqrt(az);
  double delta_phi = std::min(0.085, p3);

  res[0].shrinkMin(pM - delta_phi);
  res[0].shrinkMax(pM + delta_phi);

  return res;
}

template <std::size_t N, typename SP>
bool validTupleOrtho(
    const SP *low, const SP *high,
    const Acts::SeedfinderConfig<ActsExamples::SimSpacePoint> &config) {
  Acts::RangeXD<N, double> r = validTupleOrthoRange<N>(*low, config);
  typename Acts::RangeXD<N, double>::coordinate_t p = {
      high->phi(), high->radius(), high->z()};

  return r.contains(p);
}

template <typename SP>
bool validTuple(const SP *low, const SP *high, double colMin, double colMax,
                double cotThetaMax) {
  float rL = low->radius();
  float rH = high->radius();

  float zL = low->z();
  float zH = high->z();

  float deltaR = rH - rL;

  // ratio Z/R (forward angle) of space point duplet
  // float cotTheta2 = (zH - zL) / config.deltaRMax;
  // if (std::fabs(cotTheta2) > config.cotThetaMax) {
  //   std::cout << "Tuple discriminator 0.1" << std::endl;
  //   return false;
  // }

  // ratio Z/R (forward angle) of space point duplet
  float cotTheta = (zH - zL) / deltaR;
  if (std::fabs(cotTheta) > cotThetaMax) {
    // std::cout << "Tuple discriminator 1" << std::endl;
    return false;
  }
  // check if duplet origin on z axis within collision region
  float zOrigin = zL - rL * cotTheta;
  if (zOrigin < colMin || zOrigin > colMax) {
    // std::cout << "Tuple discriminator 2" << std::endl;
    return false;
  }

  return true;
}

}  // namespace

ActsExamples::SeedingOrthogonalAlgorithm::SeedingOrthogonalAlgorithm(
    ActsExamples::SeedingOrthogonalAlgorithm::Config cfg,
    Acts::Logging::Level lvl)
    : ActsExamples::BareAlgorithm("SeedingAlgorithm", lvl),
      m_cfg(std::move(cfg)) {
  if (m_cfg.inputSpacePoints.empty()) {
    throw std::invalid_argument("Missing space point input collections");
  }
  for (const auto &i : m_cfg.inputSpacePoints) {
    if (i.empty()) {
      throw std::invalid_argument("Invalid space point input collection");
    }
  }
  if (m_cfg.outputProtoTracks.empty()) {
    throw std::invalid_argument("Missing proto tracks output collection");
  }
  if (m_cfg.outputSeeds.empty()) {
    throw std::invalid_argument("Missing seeds output collection");
  }

  // construct seed filter
  Acts::SeedFilterConfig filterCfg;
  filterCfg.maxSeedsPerSpM = m_cfg.maxSeedsPerSpM;
  m_finderCfg.seedFilter = std::make_unique<Acts::SeedFilter<SimSpacePoint>>(
      Acts::SeedFilter<SimSpacePoint>(filterCfg));

  m_finderCfg.rMax = m_cfg.rMax;
  m_finderCfg.deltaRMin = m_cfg.deltaRMin;
  m_finderCfg.deltaRMax = m_cfg.deltaRMax;
  m_finderCfg.collisionRegionMin = m_cfg.collisionRegionMin;
  m_finderCfg.collisionRegionMax = m_cfg.collisionRegionMax;
  m_finderCfg.zMin = m_cfg.zMin;
  m_finderCfg.zMax = m_cfg.zMax;
  m_finderCfg.maxSeedsPerSpM = m_cfg.maxSeedsPerSpM;
  m_finderCfg.cotThetaMax = m_cfg.cotThetaMax;
  m_finderCfg.sigmaScattering = m_cfg.sigmaScattering;
  m_finderCfg.radLengthPerSeed = m_cfg.radLengthPerSeed;
  m_finderCfg.minPt = m_cfg.minPt;
  m_finderCfg.bFieldInZ = m_cfg.bFieldInZ;
  m_finderCfg.beamPos = Acts::Vector2(m_cfg.beamPosX, m_cfg.beamPosY);
  m_finderCfg.impactMax = m_cfg.impactMax;

  auto _config = m_finderCfg.toInternalUnits();
  m_finderCfg = _config;

  // calculation of scattering using the highland formula
  // convert pT to p once theta angle is known
  m_finderCfg.highland = 13.6 * std::sqrt(m_finderCfg.radLengthPerSeed) *
                         (1 + 0.038 * std::log(m_finderCfg.radLengthPerSeed));
  float maxScatteringAngle = m_finderCfg.highland / m_finderCfg.minPt;
  m_finderCfg.maxScatteringAngle2 = maxScatteringAngle * maxScatteringAngle;
  // helix radius in homogeneous magnetic field. Units are Kilotesla, MeV and
  // millimeter
  // TODO: change using ACTS units
  m_finderCfg.pTPerHelixRadius = 300. * m_finderCfg.bFieldInZ;
  m_finderCfg.minHelixDiameter2 =
      std::pow(m_finderCfg.minPt * 2 / m_finderCfg.pTPerHelixRadius, 2);
  std::cout << "Min helix radius = "
            << (0.5 * std::sqrt(m_finderCfg.minHelixDiameter2)) << std::endl;
  m_finderCfg.pT2perRadius =
      std::pow(m_finderCfg.highland / m_finderCfg.pTPerHelixRadius, 2);
}

std::optional<std::pair<double, double>>
ActsExamples::SeedingOrthogonalAlgorithm::validTriple(
    const Acts::InternalSpacePoint<ActsExamples::SimSpacePoint> &b,
    const Acts::InternalSpacePoint<ActsExamples::SimSpacePoint> &m,
    const Acts::InternalSpacePoint<ActsExamples::SimSpacePoint> &t) const {
  const double maxScatteringAngle = std::sqrt(m_finderCfg.maxScatteringAngle2);
  const double pTperRadius = std::sqrt(m_finderCfg.pT2perRadius);
  std::complex<double> rotPhiM =
      std::polar(1.0, static_cast<double>(std::asin(-m.y() / m.radius())));
  std::complex<double> mc(m.x(), m.y());

  Acts::LinCircle lb = lcBotMid(b, m);

  std::complex<double> bc(b.x(), b.y());

  std::complex<double> qc = (mc - bc);
  std::complex<double> tf = std::complex(1.0, 0.0) / qc;

  float csc = std::sqrt(1. + lb.cotTheta * lb.cotTheta);

  std::complex<double> tc(t.x(), t.y());

  std::complex<double> w = (tc - bc) * tf;
  std::complex<double> mp = (w - std::norm(w)) / (w - std::conj(w));
  std::complex<double> M = qc * mp + bc;
  double norm = std::norm(bc - M);

  if (4 * norm < m_finderCfg.minHelixDiameter2) {
    return std::nullopt;
  }

  double radius = std::sqrt(norm);

  std::complex<double> I = M - M * radius / std::abs(M);

  if (std::norm(I) > std::pow(m_finderCfg.impactMax, 2)) {
    return std::nullopt;
  }

  std::complex<double> deltalb = bc - mc;
  std::complex<double> deltalt = tc - mc;

  double ltCotTheta = (t.z() - m.z()) / std::abs(deltalt);

  double ltEr = ((m.varianceZ() + t.varianceZ()) +
                 (ltCotTheta * ltCotTheta) * (m.varianceR() + t.varianceR())) /
                std::norm(deltalt);

  // add errors of spB-spM and spM-spT pairs and add the correlation term
  // for errors on spM
  double error2 =
      ltEr + lb.Er +
      2 * (lb.cotTheta * ltCotTheta * m.varianceR() + m.varianceZ()) *
          lb.iDeltaR / std::abs(tc - mc);

  double deltaCotTheta = std::abs(lb.cotTheta - ltCotTheta);

  // if the error is larger than the difference in theta, no need to
  // compare with scattering
  if (deltaCotTheta * deltaCotTheta > error2) {
    double pT2scatter;

    if (m_finderCfg.pTPerHelixRadius * radius > m_finderCfg.maxPtScattering) {
      pT2scatter = m_finderCfg.highland / m_finderCfg.maxPtScattering;
    } else {
      pT2scatter = pTperRadius / radius;
    }

    double max_scat = std::min(maxScatteringAngle, pT2scatter);

    if (deltaCotTheta - std::sqrt(error2) >
        m_finderCfg.sigmaScattering * csc * max_scat) {
      return std::nullopt;
    }
  }

  std::complex<double> dlb = deltalb / std::complex(std::norm(deltalb), 0.0);
  std::complex<double> dlt = deltalt / std::complex(std::norm(deltalt), 0.0);
  std::complex<double> ltc = dlt * rotPhiM;
  std::complex<double> lbc = dlb * rotPhiM;
  std::complex<double> myd = ltc - lbc;
  double secMyd = std::abs(myd) / myd.real();
  double tanMyd = myd.imag() / myd.real();
  double sectanMyd = secMyd * tanMyd;

  return std::pair<double, double>(std::abs(I),
                                   lbc.imag() - sectanMyd * lbc.real());
}

template <typename OutputIt>
void ActsExamples::SeedingOrthogonalAlgorithm::helper(
    const SP &middle, std::vector<const SP *> &bottom,
    std::vector<const SP *> &top, OutputIt it) const {
  float rM = middle.radius();
  float varianceRM = middle.varianceR();
  float varianceZM = middle.varianceZ();

  std::vector<const SP *> top_valid;
  std::vector<float> curvatures;
  std::vector<float> impactParameters;

  // contains parameters required to calculate circle with linear equation
  // ...for bottom-middle
  std::vector<Acts::LinCircle> linCircleBottom;
  linCircleBottom.reserve(bottom.size());
  // ...for middle-top
  std::vector<Acts::LinCircle> linCircleTop;
  linCircleTop.reserve(top.size());
  transformCoordinates(bottom, middle, true, linCircleBottom);
  transformCoordinates(top, middle, false, linCircleTop);

  std::vector<double> tanLM;
  std::vector<double> tanMT;

  tanLM.reserve(bottom.size());
  tanMT.reserve(top.size());

  size_t numBotSP = bottom.size();
  size_t numTopSP = top.size();

  for (size_t b = 0; b < numBotSP; b++) {
    tanLM.push_back(std::atan2(middle.radius() - bottom[b]->radius(),
                               middle.z() - bottom[b]->z()));
  }

  for (size_t t = 0; t < numTopSP; t++) {
    tanMT.push_back(std::atan2(top[t]->radius() - middle.radius(),
                               top[t]->z() - middle.z()));
  }

  for (size_t b = 0; b < numBotSP; b++) {
    auto lb = linCircleBottom[b];
    float Zob = lb.Zo;
    float cotThetaB = lb.cotTheta;
    float Vb = lb.V;
    float Ub = lb.U;
    float ErB = lb.Er;
    float iDeltaRB = lb.iDeltaR;

    // 1+(cot^2(theta)) = 1/sin^2(theta)
    float iSinTheta2 = (1. + cotThetaB * cotThetaB);
    // calculate max scattering for min momentum at the seed's theta angle
    // scaling scatteringAngle^2 by sin^2(theta) to convert pT^2 to p^2
    // accurate would be taking 1/atan(thetaBottom)-1/atan(thetaTop) <
    // scattering
    // but to avoid trig functions we approximate cot by scaling by
    // 1/sin^4(theta)
    // resolving with pT to p scaling --> only divide by sin^2(theta)
    // max approximation error for allowed scattering angles of 0.04 rad at
    // eta=infinity: ~8.5%
    float scatteringInRegion2 = m_finderCfg.maxScatteringAngle2 * iSinTheta2;
    // multiply the squared sigma onto the squared scattering
    scatteringInRegion2 *=
        m_finderCfg.sigmaScattering * m_finderCfg.sigmaScattering;

    // clear all vectors used in each inner for loop
    top_valid.clear();
    curvatures.clear();
    impactParameters.clear();
    for (size_t t = 0; t < numTopSP; t++) {
      auto lt = linCircleTop[t];

      if (std::abs(tanLM[b] - tanMT[t]) > 0.005) {
        continue;
      }

      // add errors of spB-spM and spM-spT pairs and add the correlation term
      // for errors on spM
      float error2 = lt.Er + ErB +
                     2 * (cotThetaB * lt.cotTheta * varianceRM + varianceZM) *
                         iDeltaRB * lt.iDeltaR;

      float deltaCotTheta = cotThetaB - lt.cotTheta;
      float deltaCotTheta2 = deltaCotTheta * deltaCotTheta;
      float error;
      float dCotThetaMinusError2;
      // if the error is larger than the difference in theta, no need to
      // compare with scattering
      if (deltaCotTheta2 - error2 > 0) {
        deltaCotTheta = std::abs(deltaCotTheta);
        // if deltaTheta larger than the scattering for the lower pT cut, skip
        error = std::sqrt(error2);
        dCotThetaMinusError2 =
            deltaCotTheta2 + error2 - 2 * deltaCotTheta * error;
        // avoid taking root of scatteringInRegion
        // if left side of ">" is positive, both sides of unequality can be
        // squared
        // (scattering is always positive)

        if (dCotThetaMinusError2 > scatteringInRegion2) {
          continue;
        }
      }

      float dU = lt.U - Ub;

      // A and B are evaluated as a function of the circumference parameters
      // x_0 and y_0
      float A = (lt.V - Vb) / dU;
      float S2 = 1. + A * A;
      float B = Vb - A * Ub;
      float B2 = B * B;
      // sqrt(S2)/B = 2 * helixradius
      // calculated radius must not be smaller than minimum radius
      if (S2 < B2 * m_finderCfg.minHelixDiameter2) {
        continue;
      }
      // 1/helixradius: (B/sqrt(S2))*2 (we leave everything squared)
      float iHelixDiameter2 = B2 / S2;
      // calculate scattering for p(T) calculated from seed curvature
      float pT2scatter = 4 * iHelixDiameter2 * m_finderCfg.pT2perRadius;
      // if pT > maxPtScattering, calculate allowed scattering angle using
      // maxPtScattering instead of pt.
      float pT = m_finderCfg.pTPerHelixRadius * std::sqrt(S2 / B2) / 2.;
      if (pT > m_finderCfg.maxPtScattering) {
        float pTscatter = m_finderCfg.highland / m_finderCfg.maxPtScattering;
        pT2scatter = pTscatter * pTscatter;
      }
      // convert p(T) to p scaling by sin^2(theta) AND scale by 1/sin^4(theta)
      // from rad to deltaCotTheta
      // if deltaTheta larger than allowed scattering for calculated pT, skip
      if (deltaCotTheta2 - error2 > 0) {
        if (dCotThetaMinusError2 > pT2scatter * iSinTheta2 *
                                       m_finderCfg.sigmaScattering *
                                       m_finderCfg.sigmaScattering) {
          continue;
        }
      }
      // A and B allow calculation of impact params in U/V plane with linear
      // function
      // (in contrast to having to solve a quadratic function in x/y plane)
      float Im = std::abs((A - B * rM) * rM);

      if (Im <= m_finderCfg.impactMax) {
        top_valid.push_back(top[t]);
        // inverse diameter is signed depending if the curvature is
        // positive/negative in phi
        curvatures.push_back(B / std::sqrt(S2));
        impactParameters.push_back(Im);
      }
    }
    if (!top_valid.empty()) {
      m_finderCfg.seedFilter->filterSeeds_2SpFixed(
          *bottom[b], middle, top_valid, curvatures, impactParameters, Zob, it);
    }
  }
}

template <std::size_t NDims, typename tree_t>
ActsExamples::SimSeedContainer
ActsExamples::SeedingOrthogonalAlgorithm::strategy1(
    const std::vector<
        const Acts::InternalSpacePoint<ActsExamples::SimSpacePoint> *>
        spacePoints,
    const tree_t &tree) const {
  using range_t = typename tree_t::range_t;

  ActsExamples::SimSeedContainer seeds;
  std::size_t timer_0 = 0, timer_1 = 0, timer_2 = 0, timer_3 = 0, timer_4 = 0;

  std::vector<const SP *> bottom_lh_v, bottom_hl_v, top_lh_v, top_hl_v;

  std::vector<std::pair<float, std::unique_ptr<const Acts::InternalSeed<
                                   ActsExamples::SimSpacePoint>>>>
      protoseeds;

  /***************************************************************************
   * STRATEGY 1
   ***************************************************************************/

  for (const SP *middle : spacePoints) {
    std::chrono::high_resolution_clock::time_point tt0 =
        std::chrono::high_resolution_clock::now();

    if (!validMiddle(*middle)) {
      continue;
    }

    range_t bottom_r = validMidToLowOrthoRange<NDims>(*middle, m_finderCfg);
    range_t top_r = validTupleOrthoRange<NDims>(*middle, m_finderCfg);

    double myCotTheta = std::max(std::abs(middle->z() / middle->radius()),
                                 m_finderCfg.cotThetaMax);

    double deltaRMaxTop = top_r[1].max() - middle->radius();
    double deltaRMaxBottom = middle->radius() - bottom_r[1].min();

    range_t bottom_lh_r = bottom_r;
    bottom_lh_r[2].shrink(middle->z() - myCotTheta * deltaRMaxBottom,
                          middle->z());
    range_t top_lh_r = top_r;
    top_lh_r[2].shrink(middle->z(), middle->z() + myCotTheta * deltaRMaxTop);

    range_t bottom_hl_r = bottom_r;
    bottom_hl_r[2].shrink(middle->z(),
                          middle->z() + myCotTheta * deltaRMaxBottom);
    range_t top_hl_r = top_r;
    top_hl_r[2].shrink(middle->z() - myCotTheta * deltaRMaxTop, middle->z());

    // std::cout << "BotLH " << bottom_lh_r.str() << std::endl;
    // std::cout << "BotHL " << bottom_hl_r.str() << std::endl;
    // std::cout << "TopLH " << top_lh_r.str() << std::endl;
    // std::cout << "TopHL " << top_hl_r.str() << std::endl;

    auto valid_lh =
        std::bind(validTuple<SP>, _1, middle, m_finderCfg.collisionRegionMin,
                  m_finderCfg.collisionRegionMax, m_finderCfg.cotThetaMax);
    auto valid_hl =
        std::bind(validTuple<SP>, middle, _1, m_finderCfg.collisionRegionMin,
                  m_finderCfg.collisionRegionMax, m_finderCfg.cotThetaMax);

    bottom_lh_v.clear();
    bottom_hl_v.clear();
    top_lh_v.clear();
    top_hl_v.clear();

    if (!bottom_lh_r.degenerate() && !top_lh_r.degenerate()) {
      tree.rangeSearchMapDiscard(
          bottom_lh_r,
          [&valid_lh, &bottom_lh_v](const typename tree_t::coordinate_t &,
                                    const typename tree_t::value_t &bottom) {
            if (valid_lh(bottom)) {
              bottom_lh_v.push_back(bottom);
            }
          });
    }

    if (!bottom_hl_r.degenerate() && !top_hl_r.degenerate()) {
      tree.rangeSearchMapDiscard(
          bottom_hl_r,
          [&valid_hl, &bottom_hl_v](const typename tree_t::coordinate_t &,
                                    const typename tree_t::value_t &bottom) {
            if (valid_hl(bottom)) {
              bottom_hl_v.push_back(bottom);
            }
          });
    }

    std::chrono::high_resolution_clock::time_point tt1 =
        std::chrono::high_resolution_clock::now();

    if (!bottom_lh_v.empty()) {
      tree.rangeSearchMapDiscard(
          top_lh_r,
          [&valid_lh, &top_lh_v](const typename tree_t::coordinate_t &,
                                 const typename tree_t::value_t &top) {
            if (valid_lh(top)) {
              top_lh_v.push_back(top);
            }
          });
    }

    if (!bottom_hl_v.empty()) {
      tree.rangeSearchMapDiscard(
          top_hl_r,
          [&valid_hl, &top_hl_v](const typename tree_t::coordinate_t &,
                                 const typename tree_t::value_t &top) {
            if (valid_hl(top)) {
              top_hl_v.push_back(top);
            }
          });
    }

    std::chrono::high_resolution_clock::time_point tt2 =
        std::chrono::high_resolution_clock::now();

    protoseeds.clear();

    if (!bottom_lh_v.empty() && !top_lh_v.empty()) {
      helper(*middle, bottom_lh_v, top_lh_v, std::back_inserter(protoseeds));
    }

    if (!bottom_hl_v.empty() && !top_hl_v.empty()) {
      helper(*middle, bottom_hl_v, top_hl_v, std::back_inserter(protoseeds));
    }

    std::chrono::high_resolution_clock::time_point tt3 =
        std::chrono::high_resolution_clock::now();

    m_finderCfg.seedFilter->filterSeeds_1SpFixed(protoseeds,
                                                 std::back_inserter(seeds));

    std::chrono::high_resolution_clock::time_point tt4 =
        std::chrono::high_resolution_clock::now();

    timer_0 += std::chrono::duration_cast<std::chrono::microseconds>(tt1 - tt0)
                   .count();
    timer_1 += std::chrono::duration_cast<std::chrono::microseconds>(tt2 - tt1)
                   .count();
    timer_2 += std::chrono::duration_cast<std::chrono::microseconds>(tt3 - tt2)
                   .count();
    timer_3 += std::chrono::duration_cast<std::chrono::microseconds>(tt4 - tt3)
                   .count();
  }

  std::cout << "Time taken = " << (timer_0 / 1000.0) << "ms, "
            << (timer_1 / 1000.0) << "ms, " << (timer_2 / 1000.0) << "ms, "
            << (timer_3 / 1000.0) << "ms, " << (timer_4 / 1000.0) << "ms"
            << std::endl;

  return seeds;
}

ActsExamples::ProcessCode ActsExamples::SeedingOrthogonalAlgorithm::execute(
    const AlgorithmContext &ctx) const {
  std::chrono::high_resolution_clock::time_point t0 =
      std::chrono::high_resolution_clock::now();
  // construct the combined input container of space point pointers from all
  // configured input sources.
  // pre-compute the total size required so we only need to allocate once
  size_t nSpacePoints = 0;
  for (const auto &isp : m_cfg.inputSpacePoints) {
    nSpacePoints += ctx.eventStore.get<SimSpacePointContainer>(isp).size();
  }
  std::vector<const Acts::InternalSpacePoint<SimSpacePoint> *> spacePoints;
  spacePoints.reserve(nSpacePoints);
  for (const auto &isp : m_cfg.inputSpacePoints) {
    for (const auto &spacePoint :
         ctx.eventStore.get<SimSpacePointContainer>(isp)) {
      // since the event store owns the space points, their pointers should be
      // stable and we do not need to create local copies.
      spacePoints.push_back(new Acts::InternalSpacePoint<SimSpacePoint>(
          spacePoint, {spacePoint.x(), spacePoint.y(), spacePoint.z()},
          {0.0, 0.0}, {spacePoint.varianceR(), spacePoint.varianceZ()}));
    }
  }

  constexpr std::size_t NDims = 3;

  std::vector<std::pair<std::array<double, NDims>, const SP *>> points;

  enum class Dim { Phi = 0, Radius = 1, Z = 2, Y = 3, X = 4, CotTheta = 5 };

  for (auto sp : spacePoints) {
    std::array<double, NDims> point;

    if constexpr (NDims >= 6) {
      point[5] = sp->z() / sp->radius();
    }

    if constexpr (NDims >= 5) {
      point[3] = sp->y();
      point[4] = sp->x();
    }

    if constexpr (NDims >= 3) {
      point[0] = sp->phi();
      point[1] = sp->radius();
      point[2] = sp->z();
    }

    points.emplace_back(point, sp);

    if (point[0] <= -3.14159265 + 0.1) {
      std::array<double, NDims> point_dup = point;
      point_dup[0] += 2 * 3.14159265;
      points.emplace_back(point_dup, sp);
    }

    if (point[0] >= 3.14159265 - 0.1) {
      std::array<double, NDims> point_dup = point;
      point_dup[0] -= 2 * 3.14159265;
      points.emplace_back(point_dup, sp);
    }
  }

  std::chrono::high_resolution_clock::time_point t1 =
      std::chrono::high_resolution_clock::now();

  Acts::KDTree<NDims, const SP *, Acts::ActsScalar, std::array, 4> tree(
      std::move(points));

  // tree.print();

  std::chrono::high_resolution_clock::time_point t2 =
      std::chrono::high_resolution_clock::now();

  // run the seeding
  SimSeedContainer seeds = strategy1<NDims>(spacePoints, tree);

  std::chrono::high_resolution_clock::time_point t3 =
      std::chrono::high_resolution_clock::now();

  // extract proto tracks, i.e. groups of measurement indices, from tracks seeds
  size_t nSeeds = seeds.size();
  ProtoTrackContainer protoTracks;
  protoTracks.reserve(nSeeds);
  for (const auto &seed : seeds) {
    ProtoTrack protoTrack;
    protoTrack.reserve(seed.sp().size());
    for (auto spacePointPtr : seed.sp()) {
      protoTrack.push_back(spacePointPtr->measurementIndex());
    }
    protoTracks.push_back(std::move(protoTrack));
  }

  std::chrono::high_resolution_clock::time_point t4 =
      std::chrono::high_resolution_clock::now();

  std::cout << "Preprocessing:     "
            << (std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0)
                    .count() /
                1000.0)
            << "ms" << std::endl;
  std::cout << "k-d tree creation: "
            << (std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1)
                    .count() /
                1000.0)
            << "ms" << std::endl;
  std::cout << "Seed finding:      "
            << (std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2)
                    .count() /
                1000.0)
            << "ms" << std::endl;
  std::cout << "Postprocessing:    "
            << (std::chrono::duration_cast<std::chrono::microseconds>(t4 - t3)
                    .count() /
                1000.0)
            << "ms" << std::endl;

  ACTS_DEBUG("Created " << seeds.size() << " track seeds from "
                        << spacePoints.size() << " space points");

  ctx.eventStore.add(m_cfg.outputSeeds, std::move(seeds));
  ctx.eventStore.add(m_cfg.outputProtoTracks, std::move(protoTracks));
  return ActsExamples::ProcessCode::SUCCESS;
}
