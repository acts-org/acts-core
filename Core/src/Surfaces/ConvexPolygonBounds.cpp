// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Surfaces/ConvexPolygonBounds.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/detail/BoundaryCheckHelper.hpp"

#include <optional>
#include <ostream>

namespace Acts {

std::ostream& ConvexPolygonBoundsBase::toStream(std::ostream& sl) const {
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

std::vector<double> ConvexPolygonBoundsBase::values() const {
  std::vector<double> values;
  for (const auto& vtx : vertices()) {
    values.push_back(vtx.x());
    values.push_back(vtx.y());
  }
  return values;
}

ConvexPolygonBounds<PolygonDynamic>::ConvexPolygonBounds(
    const std::vector<Vector2>& vertices)
    : m_vertices(vertices.begin(), vertices.end()),
      m_boundingBox(makeBoundingBox(vertices)) {}

bool ConvexPolygonBounds<PolygonDynamic>::inside(
    const Vector2& lposition,
    const BoundaryTolerance& boundaryTolerance) const {
  return detail::insidePolygon(
      std::span<const Vector2>(m_vertices.data(), m_vertices.size()),
      boundaryTolerance, lposition, std::nullopt);
}

std::vector<Vector2> ConvexPolygonBounds<PolygonDynamic>::vertices(
    unsigned int /*lseg*/) const {
  return {m_vertices.begin(), m_vertices.end()};
}

const RectangleBounds& ConvexPolygonBounds<PolygonDynamic>::boundingBox()
    const {
  return m_boundingBox;
}

void ConvexPolygonBounds<PolygonDynamic>::checkConsistency() const
    noexcept(false) {
  convex_impl(m_vertices);
}

}  // namespace Acts
