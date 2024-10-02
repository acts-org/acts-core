// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Definitions/Algebra.hpp"
template <typename value_t, std::size_t DIM>
Acts::Ray<value_t, DIM>::Ray(const VertexType& origin, const VertexType& dir)
    : m_origin(origin), m_dir(dir.normalized()), m_idir(1 / m_dir.array()) {}
template <typename value_t, std::size_t DIM>
std::ostream& Acts::Ray<value_t, DIM>::toStream(std::ostream& os) const {
  os << "Ray(";
  for (std::size_t i = 0; i < DIM; i++) {
    if (i > 0) {
      os << ", ";
    }
    os << m_origin[i];
  }
  os << " -> ";
  for (std::size_t i = 0; i < DIM; i++) {
    if (i > 0) {
      os << ", ";
    }
    os << m_dir[i];
  }
  os << ")";

  return os;
}

template <typename value_t, std::size_t DIM>
Acts::Ray<value_t, DIM> Acts::Ray<value_t, DIM>::transformed(
    const transform_type& trf) const {
  return Ray<value_t, DIM>(trf * m_origin, trf.rotation() * m_dir);
}

template <typename value_t, std::size_t DIM>
void Acts::Ray<value_t, DIM>::draw(IVisualization3D& helper,
                                   value_type far_distance) const
  requires(DIM == 3)
{
  static_assert(DIM == 3, "OBJ is only supported in 3D");

  helper.line(m_origin, (m_origin + m_dir * far_distance).eval());
}

template <typename U, std::size_t V>
std::ostream& operator<<(std::ostream& os, const Acts::Ray<U, V>& ray) {
  ray.dump(os);
  return os;
}
