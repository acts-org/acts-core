// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Navigation/TryAllNavigationPolicy.hpp"

#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Navigation/NavigationStream.hpp"

namespace Acts {

TryAllNavigationPolicy::TryAllNavigationPolicy(const GeometryContext& /*gctx*/,
                                               const TrackingVolume& volume,
                                               const Logger& logger,
                                               const Config& config)
    : m_cfg{config}, m_volume(&volume) {
  assert(m_volume != nullptr);
  ACTS_VERBOSE("TryAllNavigationPolicy created for volume "
               << m_volume->volumeName());
}

TryAllNavigationPolicy::TryAllNavigationPolicy(const GeometryContext& gctx,
                                               const TrackingVolume& volume,
                                               const Logger& logger)
    : TryAllNavigationPolicy(gctx, volume, logger, {}) {}

void TryAllNavigationPolicy::initializeCandidates(
    const NavigationArguments& args, AppendOnlyNavigationStream& stream,
    const Logger& logger) const {
  ACTS_VERBOSE("TryAllNavigationPolicy");
  assert(m_volume != nullptr);

  if (m_cfg.portals) {
    for (const auto& portal : m_volume->portals()) {
      stream.addPortalCandidate(portal);
    }
  }

  if (m_cfg.sensitives) {
    for (const auto& surface : m_volume->surfaces()) {
      stream.addSurfaceCandidate(surface, args.tolerance);
    };
  }
}

void TryAllNavigationPolicy::connect(NavigationDelegate& delegate) const {
  connectDefault<TryAllNavigationPolicy>(delegate);
}

}  // namespace Acts
