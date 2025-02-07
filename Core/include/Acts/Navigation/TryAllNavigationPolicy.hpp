// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Navigation/INavigationPolicy.hpp"
#include "Acts/Navigation/NavigationStream.hpp"

namespace Acts {

class TrackingVolume;
class GeometryContext;
class Logger;

/// Policy which adds **all** candidates of the configured type to the
/// stream
class TryAllNavigationPolicy final : public INavigationPolicy {
 public:
  struct Config {
    bool portals = true;
    bool sensitives = true;
  };

  /// Constructor from a volume
  /// @param gctx is the geometry context
  /// @param volume is the volume to navigate
  /// @param logger is the logger
  /// @param config The configuration for the policy
  TryAllNavigationPolicy(const GeometryContext& gctx,
                         const TrackingVolume& volume, const Logger& logger,
                         const Config& config);

  /// Constructor from a volume
  /// @param gctx is the geometry context
  /// @param volume is the volume to navigate
  /// @param logger is the logger
  TryAllNavigationPolicy(const GeometryContext& gctx,
                         const TrackingVolume& volume, const Logger& logger);

  /// Add all candidates to the stream
  /// @param args are the navigation arguments
  /// @param stream is the navigation stream to update
  /// @param logger is the logger
  void initializeCandidates(const NavigationArguments& args,
                            AppendOnlyNavigationStream& stream,
                            const Logger& logger) const;

  /// Connect the policy to a navigation delegate
  /// @param delegate is the navigation delegate
  void connect(NavigationDelegate& delegate) const override;

 private:
  Config m_cfg;
  const TrackingVolume* m_volume;
};

static_assert(NavigationPolicyConcept<TryAllNavigationPolicy>);

}  // namespace Acts
