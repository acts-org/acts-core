// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Surfaces/ConvexPolygonBounds.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/detail/BoundaryCheckHelper.hpp"

#include <algorithm>
#include <optional>
#include <ostream>

std::ostream& Acts::ConvexPolygonBoundsBase::toStream(std::ostream& sl) const {
  std::vector<Vector2> vtxs = vertices();
  sl << "Acts::ConvexPolygonBounds<" << vtxs.size() << ">: vertices: [x, y]\n";
  for (std::size_t i = 0; i < vtxs.size(); i++) {
    const auto& vtx = vtxs[i];
    if (i > 0) {
      sl << ",";
      sl << "\n";
    }
    sl << "[" << vtx.x() << ", " << vtx.y() << "]";
  }
  return sl;
}

std::vector<double> Acts::ConvexPolygonBoundsBase::values() const {
  std::vector<double> values;
  for (const auto& vtx : vertices()) {
    values.push_back(vtx.x());
    values.push_back(vtx.y());
  }
  return values;
}

Acts::ConvexPolygonBounds<Acts::PolygonDynamic>::ConvexPolygonBounds(
    const std::vector<Vector2>& vertices)
    : m_vertices(vertices.begin(), vertices.end()),
      m_boundingBox(makeBoundingBox(vertices)) {}

Acts::SurfaceBounds::BoundsType
Acts::ConvexPolygonBounds<Acts::PolygonDynamic>::type() const {
  return SurfaceBounds::eConvexPolygon;
}

bool Acts::ConvexPolygonBounds<Acts::PolygonDynamic>::inside(
    const Acts::Vector2& lposition,
    const Acts::BoundaryTolerance& boundaryTolerance) const {
  return detail::insidePolygon(
      std::span<const Vector2>(m_vertices.data(), m_vertices.size()),
      boundaryTolerance, lposition, std::nullopt);
}

std::vector<Acts::Vector2> Acts::ConvexPolygonBounds<
    Acts::PolygonDynamic>::vertices(unsigned int /*lseg*/) const {
  return {m_vertices.begin(), m_vertices.end()};
}

const Acts::RectangleBounds&
Acts::ConvexPolygonBounds<Acts::PolygonDynamic>::boundingBox() const {
  return m_boundingBox;
}

void Acts::ConvexPolygonBounds<Acts::PolygonDynamic>::checkConsistency() const
    noexcept(false) {
  convex_impl(m_vertices);
}
