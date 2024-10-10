// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

///////////////////////////////////////////////////////////////////
// Seed.hpp Acts project
///////////////////////////////////////////////////////////////////

#pragma once
#include <list>

namespace Acts::Legacy {

template <typename SpacePoint>
class Seed {
  /////////////////////////////////////////////////////////////////////////////////
  // Public methods:
  /////////////////////////////////////////////////////////////////////////////////

 public:
  Seed();
  Seed(const SpacePoint* /*b*/, const SpacePoint* /*m*/,
       const SpacePoint* /*u*/, const double /*vertex*/);
  Seed(const Seed& /*s*/);
  Seed& operator=(const Seed& /*s*/);
  virtual ~Seed();
  void erase();
  void add(const SpacePoint*& /*sp*/);
  void setZVertex(const double& /*z*/);
  const std::list<const SpacePoint*>& spacePoints() const;
  const double& zVertex() const;

  /////////////////////////////////////////////////////////////////////////////////
  // Protected data members
  /////////////////////////////////////////////////////////////////////////////////

 protected:
  std::list<const SpacePoint*> m_spacepoints;
  double m_zvertex = 0;
};

/// @cond

/////////////////////////////////////////////////////////////////////////////////
// Inline methods
/////////////////////////////////////////////////////////////////////////////////

template <typename SpacePoint>
inline const std::list<const SpacePoint*>& Seed<SpacePoint>::spacePoints()
    const {
  return this->m_spacepoints;
}

template <typename SpacePoint>
inline void Seed<SpacePoint>::erase() {
  m_spacepoints.erase(m_spacepoints.begin(), m_spacepoints.end());
}

template <typename SpacePoint>
inline void Seed<SpacePoint>::add(const SpacePoint*& p) {
  m_spacepoints.push_back(p);
}

template <typename SpacePoint>
inline void Seed<SpacePoint>::setZVertex(const double& z) {
  m_zvertex = z;
}

template <typename SpacePoint>
inline const double& Seed<SpacePoint>::zVertex() const {
  return m_zvertex;
}

///////////////////////////////////////////////////////////////////////////////
// Constructors
///////////////////////////////////////////////////////////////////////////////

template <typename SpacePoint>
Seed<SpacePoint>::Seed(const Seed<SpacePoint>& s) {
  m_spacepoints = s.spacePoints();
  m_zvertex = s.zVertex();
}

template <typename SpacePoint>
Seed<SpacePoint>& Seed<SpacePoint>::operator=(const Seed<SpacePoint>& s) {
  m_spacepoints = s.spacePoints();
  m_zvertex = s.zVertex();
  return *this;
}

template <typename SpacePoint>
Seed<SpacePoint>::Seed() = default;

template <typename SpacePoint>
Seed<SpacePoint>::Seed(const SpacePoint* b, const SpacePoint* m,
                       const SpacePoint* u, const double vertex) {
  m_zvertex = vertex;
  m_spacepoints.push_back(b);
  m_spacepoints.push_back(m);
  m_spacepoints.push_back(u);
}

template <typename SpacePoint>
Seed<SpacePoint>::~Seed() = default;

/// @endcond

}  // namespace Acts::Legacy
