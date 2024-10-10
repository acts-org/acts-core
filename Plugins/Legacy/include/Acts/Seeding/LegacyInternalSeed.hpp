// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

///////////////////////////////////////////////////////////////////
// InternalSeed.hpp Acts project
///////////////////////////////////////////////////////////////////

#pragma once
#include "Acts/Seeding/LegacySeed.hpp"
#include "Acts/Seeding/SPForSeed.hpp"

namespace Acts::Legacy {
template <typename SpacePoint>
class InternalSeed {
  /////////////////////////////////////////////////////////////////////////////////
  // Public methods:
  /////////////////////////////////////////////////////////////////////////////////

 public:
  InternalSeed();
  InternalSeed(SPForSeed<SpacePoint>*& /*s0*/, SPForSeed<SpacePoint>*& /*s1*/,
               SPForSeed<SpacePoint>*& /*s2*/, float /*z*/);
  InternalSeed(const InternalSeed<SpacePoint>& /*sp*/);
  virtual ~InternalSeed();
  InternalSeed<SpacePoint>& operator=(const InternalSeed<SpacePoint>& /*sp*/);

  SPForSeed<SpacePoint>* spacepoint0() { return m_s0; }
  SPForSeed<SpacePoint>* spacepoint1() { return m_s1; }
  SPForSeed<SpacePoint>* spacepoint2() { return m_s2; }
  const float& z() const { return m_z; }
  const float& quality() const { return m_q; }

  void set(SPForSeed<SpacePoint>*& /*s0*/, SPForSeed<SpacePoint>*& /*s1*/,
           SPForSeed<SpacePoint>*& /*s2*/, float /*z*/);

  bool setQuality(float /*q*/);

  bool set3(Acts::Legacy::Seed<SpacePoint>& /*s*/);

 protected:
  SPForSeed<SpacePoint>* m_s0 = nullptr;
  SPForSeed<SpacePoint>* m_s1 = nullptr;
  SPForSeed<SpacePoint>* m_s2 = nullptr;
  float m_z = 0;
  float m_q = 0;
};

/// @cond

/////////////////////////////////////////////////////////////////////////////////
// Inline methods
/////////////////////////////////////////////////////////////////////////////////

template <typename SpacePoint>
inline InternalSeed<SpacePoint>::InternalSeed() {
  m_s0 = nullptr;
  m_s1 = nullptr;
  m_s2 = nullptr;
  m_z = 0.;
  m_q = 0.;
}

template <typename SpacePoint>
inline InternalSeed<SpacePoint>& InternalSeed<SpacePoint>::operator=(
    const InternalSeed& sp) {
  if (&sp != this) {
    m_z = sp.m_z;
    m_q = sp.m_q;
    m_s0 = sp.m_s0;
    m_s1 = sp.m_s1;
    m_s2 = sp.m_s2;
  }
  return (*this);
}

template <typename SpacePoint>
inline InternalSeed<SpacePoint>::InternalSeed(SPForSeed<SpacePoint>*& s0,
                                              SPForSeed<SpacePoint>*& s1,
                                              SPForSeed<SpacePoint>*& s2,
                                              float z) {
  set(s0, s1, s2, z);
  m_q = 0.;
}

/////////////////////////////////////////////////////////////////////////////////
// Copy constructor
/////////////////////////////////////////////////////////////////////////////////

template <typename SpacePoint>
inline InternalSeed<SpacePoint>::InternalSeed(const InternalSeed& sp)
    : m_s0(sp.m_s0), m_s1(sp.m_s1), m_s2(sp.m_s2) {
  *this = sp;
}

/////////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////////

template <typename SpacePoint>
inline InternalSeed<SpacePoint>::~InternalSeed() = default;

/////////////////////////////////////////////////////////////////////////////////
// Set
/////////////////////////////////////////////////////////////////////////////////

template <typename SpacePoint>
inline void InternalSeed<SpacePoint>::set(SPForSeed<SpacePoint>*& s0,
                                          SPForSeed<SpacePoint>*& s1,
                                          SPForSeed<SpacePoint>*& s2, float z) {
  m_z = z;
  m_s0 = s0;
  m_s1 = s1;
  m_s2 = s2;
}

/////////////////////////////////////////////////////////////////////////////////
// Set three space points seed
/////////////////////////////////////////////////////////////////////////////////

template <typename SpacePoint>
inline bool InternalSeed<SpacePoint>::set3(Acts::Legacy::Seed<SpacePoint>& s) {
  bool pixb = !m_s0->spacepoint->clusterList().second;
  bool pixt = !m_s2->spacepoint->clusterList().second;

  if (pixb != pixt) {
    if (m_q > m_s0->quality() && m_q > m_s1->quality() &&
        m_q > m_s2->quality()) {
      return false;
    }
  }

  m_s0->setQuality(m_q);
  m_s1->setQuality(m_q);
  m_s2->setQuality(m_q);

  s.erase();
  s.add(m_s0->spacepoint);
  s.add(m_s1->spacepoint);
  s.add(m_s2->spacepoint);
  s.setZVertex(static_cast<double>(m_z));
  return true;
}

/////////////////////////////////////////////////////////////////////////////////
// Set quality in pro seed
/////////////////////////////////////////////////////////////////////////////////

template <typename SpacePoint>
inline bool InternalSeed<SpacePoint>::setQuality(float q) {
  m_q = q;
  bool pixb = !m_s0->spacepoint->clusterList().second;
  bool pixt = !m_s2->spacepoint->clusterList().second;
  if (pixb == pixt) {
    m_s0->setQuality(q);
    m_s1->setQuality(q);
    m_s2->setQuality(q);
    return true;
  }
  if (q < m_s0->quality() || q < m_s1->quality() || q < m_s2->quality()) {
    return true;
  }
  return false;
}

/// @endcond

}  // namespace Acts::Legacy
