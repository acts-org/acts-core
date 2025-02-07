// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Geometry/TrivialPortalLink.hpp"

#include "Acts/Geometry/GridPortalLink.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"

#include <memory>

namespace Acts {

std::unique_ptr<GridPortalLink> TrivialPortalLink::makeGrid(
    AxisDirection direction) const {
  return GridPortalLink::make(m_surface, *m_volume, direction);
}

Result<const TrackingVolume*> TrivialPortalLink::resolveVolume(
    const GeometryContext& /*gctx*/, const Vector2& /*position*/,
    double /*tolerance*/) const {
  return m_volume;
}

Result<const TrackingVolume*> TrivialPortalLink::resolveVolume(
    const GeometryContext& gctx, const Vector3& position,
    double tolerance) const {
  static_cast<void>(gctx);
  static_cast<void>(position);
  static_cast<void>(tolerance);
  assert(m_surface->isOnSurface(gctx, position, BoundaryTolerance::None(),
                                tolerance) &&
         "Trivial portal lookup point should be on surface");
  return m_volume;
}

void TrivialPortalLink::toStream(std::ostream& os) const {
  os << "TrivialPortalLink<vol=" << m_volume->volumeName() << ">";
}

const TrackingVolume& TrivialPortalLink::volume() const {
  return *m_volume;
}

}  // namespace Acts
