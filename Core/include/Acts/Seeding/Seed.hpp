// This file is part of the Acts project.
//
// Copyright (C) 2018 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <array>
#include <limits>
#include <vector>

#include <boost/container/small_vector.hpp>

namespace Acts {
template <typename SpacePoint>
class Seed {
  /////////////////////////////////////////////////////////////////////////////////
  // Public methods:
  /////////////////////////////////////////////////////////////////////////////////

 public:
  Seed(const SpacePoint& b, const SpacePoint& m, const SpacePoint& u,
       std::size_t b_idx, std::size_t m_idx, std::size_t t_idx,
       float vertex,
       float seedQuality = -std::numeric_limits<float>::infinity());
  Seed(const Seed&) = default;
  Seed& operator=(const Seed&) = default;

  const auto& sp() const { return m_spacepoints; }
  double z() const { return m_zvertex; }
  float seedQuality() const { return m_seedQuality; }
  const std::array<std::size_t, 3>& idxs() const { return m_idxs; }
  
 private:
  boost::container::small_vector<const SpacePoint*, 3> m_spacepoints;  
  float m_zvertex;
  float m_seedQuality;
  std::array<std::size_t, 3> m_idxs;
};

///////////////////////////////////////////////////////////////////////////////
// Constructors
///////////////////////////////////////////////////////////////////////////////

template <typename SpacePoint>
Seed<SpacePoint>::Seed(const SpacePoint& b, const SpacePoint& m,
                       const SpacePoint& u,
		       std::size_t b_idx, std::size_t m_idx, std::size_t t_idx,
		       float vertex, float seedQuality) :
  m_zvertex(vertex),
  m_seedQuality(seedQuality),
  m_idxs({b_idx, m_idx, t_idx})
{
  m_spacepoints.push_back(&b);
  m_spacepoints.push_back(&m);
  m_spacepoints.push_back(&u);
}

}  // namespace Acts
