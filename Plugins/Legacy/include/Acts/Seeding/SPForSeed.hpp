// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

///////////////////////////////////////////////////////////////////
// SPForSeed.hpp Acts project
///////////////////////////////////////////////////////////////////

#pragma once
#include <cmath>

// COLLECTION OF MAGIC NUMBERS IN HERE:
//
//
// m_q        = 100000.;
//
// float cov = wid*wid*.08333;
//
// Pixel Case
//    if(de->isBarrel()) {m_covz = 9.*cov; m_covr = .06;}
//    else               {m_covr = 9.*cov; m_covz = .06;}
//
// SCT Case
//    if(de->isBarrel()) {m_covz = 8.*f22; m_covr = .1;}
//    else               {m_covr = 8.*f22; m_covz = .1;}

namespace Acts::Legacy {

template <typename SpacePoint>
class SPForSeed {
  /////////////////////////////////////////////////////////////////////////////////
  // Public methods:
  /////////////////////////////////////////////////////////////////////////////////

 public:
  SPForSeed();
  SPForSeed(SpacePoint* const& /*sp*/, const float* /*r*/);
  SPForSeed(SpacePoint* const& /*sp*/, const float* /*r*/, const float* /*sc*/);
  SPForSeed(const SPForSeed& /*sp*/);
  virtual ~SPForSeed();
  SPForSeed& operator=(const SPForSeed& /*sp*/);

  void set(SpacePoint* const& /*sp*/, const float* /*r*/);
  void set(SpacePoint* const& /*sp*/, const float* /*r*/, const float* /*sc*/);
  void setQuality(float /*q*/);
  void setParam(const float& /*p*/);

  const SpacePoint* spacepoint = nullptr;
  const float& x() const { return m_x; }
  const float& y() const { return m_y; }
  const float& z() const { return m_z; }
  const float& radius() const { return m_r; }
  float phi() const { return atan2(m_y, m_x); }
  const float& covr() const { return m_covr; }
  const float& covz() const { return m_covz; }
  const float& param() const { return m_param; }
  const float& quality() const { return m_q; }

  const int& surface() const { return m_surface; }

 protected:
  float m_x = 0;     // x-coordinate in beam system coordinates
  float m_y = 0;     // y-coordinate in beam system coordinates
  float m_z = 0;     // z-coordinate in beam system coordinetes
  float m_r = 0;     // radius       in beam system coordinates
  float m_covr = 0;  //
  float m_covz = 0;  //
  float m_param = 0;
  float m_q = 0;

  int m_surface = 0;  // surface identifier
};

/////////////////////////////////////////////////////////////////////////////////
// Inline methods
/////////////////////////////////////////////////////////////////////////////////

template <typename SpacePoint>
inline SPForSeed<SpacePoint>::SPForSeed() {
  spacepoint = 0;
  m_x = 0.;
  m_y = 0.;
  m_z = 0.;
  m_r = 0.;
  m_covr = 0.;
  m_covz = 0.;
  m_param = 0.;
  m_q = 0.;
}

template <typename SpacePoint>
inline SPForSeed<SpacePoint>& SPForSeed<SpacePoint>::operator=(
    const SPForSeed<SpacePoint>& sp) {
  if (&sp != this) {
    spacepoint = sp.spacepoint;
    m_x = sp.m_x;
    m_y = sp.m_y;
    m_z = sp.m_z;
    m_r = sp.m_r;
    m_covr = sp.m_covr;
    m_covz = sp.m_covz;
    m_q = sp.m_q;
  }
  return (*this);
}

template <typename SpacePoint>
inline SPForSeed<SpacePoint>::SPForSeed(SpacePoint* const& sp, const float* r) {
  set(sp, r);
  m_param = 0.;
}

template <typename SpacePoint>
inline SPForSeed<SpacePoint>::SPForSeed(SpacePoint* const& sp, const float* r,
                                        const float* sc) {
  set(sp, r, sc);
  m_param = 0.;
}

/////////////////////////////////////////////////////////////////////////////////
// Copy constructor
/////////////////////////////////////////////////////////////////////////////////

template <typename SpacePoint>
inline SPForSeed<SpacePoint>::SPForSeed(const SPForSeed& sp) {
  *this = sp;
}

/////////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////////

template <typename SpacePoint>
inline SPForSeed<SpacePoint>::~SPForSeed() = default;

/////////////////////////////////////////////////////////////////////////////////
// Set
/////////////////////////////////////////////////////////////////////////////////

template <typename SpacePoint>
inline void SPForSeed<SpacePoint>::set(SpacePoint* const& sp, const float* r) {
  spacepoint = sp;
  m_x = r[0];
  m_y = r[1];
  m_z = r[2];
  m_r = std::hypot(m_x, m_y);
  m_surface = sp->surface;
  m_q = 100000.;

  if (!sp->clusterList().second) {
    m_covr = sp->covr * 9.;
    m_covz = sp->covz * 9.;
    if (m_covr < 0.06) {
      m_covr = 0.06;
    }
    if (m_covz < 0.06) {
      m_covz = 0.06;
    }
  } else {
    m_covr = sp->covr * 8.;
    m_covz = sp->covz * 8.;
    if (m_covr < 0.1) {
      m_covr = 0.1;
    }
    if (m_covz < 0.1) {
      m_covz = 0.1;
    }
  }
}

/////////////////////////////////////////////////////////////////////////////////
// Set with error correction
// sc[0] - barrel pixels error correction
// sc[1] - endcap pixels
// sc[2] - barrel sct
// sc[3] - endcap sct
/////////////////////////////////////////////////////////////////////////////////

template <typename SpacePoint>
inline void SPForSeed<SpacePoint>::set(SpacePoint* const& sp, const float* r,
                                       const float* sc) {
  spacepoint = sp;
  m_x = r[0];
  m_y = r[1];
  m_z = r[2];
  m_r = std::hypot(m_x, m_y);
  m_q = 100000.;
  if (!sp->clusterList().second) {
    m_covr = sp->covr * 9. * sc[0];
    m_covz = sp->covz * 9. * sc[1];
    if (m_covr < 0.06) {
      m_covr = 0.06;
    }
    if (m_covz < 0.06) {
      m_covz = 0.06;
    }
  } else {
    m_covr = sp->covr * 8. * sc[2];
    m_covz = sp->covz * 8. * sc[3];
    if (m_covr < 0.1) {
      m_covr = 0.1;
    }
    if (m_covz < 0.1) {
      m_covz = 0.1;
    }
  }

  // old code:
  //      const InDet::SiCluster*           c  = static_cast<const
  //      InDet::SiCluster*>(sp->clusterList().first);
  //      if( de->isPixel() ) {
  //
  //    const Amg::MatrixX& v =  sp->localCovariance();
  //    //ATTENTION: v(1,1) contains error for z in barrel, error for r in
  //    endcaps
  //    float f22 = float(v(1,1));
  //    //ATTENTION: Variable Z contains width of R in the endcaps.
  //    float wid = float(c->width().z());
  //    float cov = wid*wid*.08333; if(cov < f22) cov = f22;
  //    // Set error of low uncertainty dimension (i.e. r for barrel, z for
  //    EC)
  //    to "small"
  //    if(sp->isBarrel()) {m_covz = 9.*cov*sc[0]; m_covr = .06;}
  //    else               {m_covr = 9.*cov*sc[1]; m_covz = .06;}
  //      }
  //      else                {
  //
  //    const Amg::MatrixX& v = sp->localCovariance();
  //    float f22 = float(v(1,1));
  //    if(sp->isBarrel()) {m_covz = 8.*f22*sc[2]; m_covr = .1;}
  //    else               {m_covr = 8.*f22*sc[3]; m_covz = .1;}
  //      }
}
template <typename SpacePoint>
inline void SPForSeed<SpacePoint>::setParam(const float& p) {
  m_param = p;
}
template <typename SpacePoint>
inline void SPForSeed<SpacePoint>::setQuality(float q) {
  if (q <= m_q) {
    m_q = q;
  }
}

}  // namespace Acts::Legacy
