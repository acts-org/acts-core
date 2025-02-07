// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Navigation/SurfaceArrayNavigationPolicy.hpp"

#include "Acts/Geometry/SurfaceArrayCreator.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Navigation/NavigationStream.hpp"

#include <algorithm>

namespace Acts {

SurfaceArrayNavigationPolicy::SurfaceArrayNavigationPolicy(
    const GeometryContext& gctx, const TrackingVolume& volume,
    const Logger& logger, Config config)
    : m_volume(volume) {
  ACTS_VERBOSE("Constructing SurfaceArrayNavigationPolicy for volume "
               << volume.volumeName());
  ACTS_VERBOSE("~> Layer type is " << config.layerType);
  ACTS_VERBOSE("~> bins: " << config.bins.first << " x " << config.bins.second);

  SurfaceArrayCreator::Config sacConfig;
  SurfaceArrayCreator sac{sacConfig, logger.clone("SrfArrCrtr")};

  std::vector<std::shared_ptr<const Surface>> surfaces;
  surfaces.reserve(volume.surfaces().size());
  for (const auto& surface : volume.surfaces()) {
    if (surface.associatedDetectorElement() == nullptr) {
      continue;
    }
    surfaces.push_back(surface.getSharedPtr());
  }

  if (config.layerType == LayerType::Disc) {
    auto [binsR, binsPhi] = config.bins;
    m_surfaceArray =
        sac.surfaceArrayOnDisc(gctx, std::move(surfaces), binsPhi, binsR);
  } else if (config.layerType == LayerType::Cylinder) {
    auto [binsPhi, binsZ] = config.bins;
    m_surfaceArray =
        sac.surfaceArrayOnCylinder(gctx, std::move(surfaces), binsPhi, binsZ);
  } else if (config.layerType == LayerType::Plane) {
    ACTS_ERROR("Plane layers are not yet supported");
    throw std::invalid_argument("Plane layers are not yet supported");
  } else {
    throw std::invalid_argument("Unknown layer type");
  }

  if (!m_surfaceArray) {
    ACTS_ERROR("Failed to create surface array");
    throw std::runtime_error("Failed to create surface array");
  }
}

void SurfaceArrayNavigationPolicy::initializeCandidates(
    const NavigationArguments& args, AppendOnlyNavigationStream& stream,
    const Logger& logger) const {
  ACTS_VERBOSE("SrfArrNavPol (volume=" << m_volume.volumeName() << ")");

  ACTS_VERBOSE("Querying sensitive surfaces at " << args.position.transpose());
  const std::vector<const Surface*>& sensitiveSurfaces =
      m_surfaceArray->neighbors(args.position);
  ACTS_VERBOSE("~> Surface array reports " << sensitiveSurfaces.size()
                                           << " sensitive surfaces");

  for (const auto* surface : sensitiveSurfaces) {
    stream.addSurfaceCandidate(*surface, args.tolerance);
  };
}

void SurfaceArrayNavigationPolicy::connect(NavigationDelegate& delegate) const {
  connectDefault<SurfaceArrayNavigationPolicy>(delegate);
}

}  // namespace Acts
