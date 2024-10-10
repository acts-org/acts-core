// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/ProtoLayer.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <memory>
#include <utility>
#include <vector>

namespace Acts {

class Surface;
struct ProtoLayer;

/// @class ProtoLayerHelper
///
/// This class is designed to parse a vector of Surfaces and sort them
/// into corresponding proto layers.
///
/// @todo write more documentation on how this is done
class ProtoLayerHelper {
 public:
  using SortingConfig = std::pair<BinningValue, double>;

  struct Config {};

  /// Constructor with explicit config
  ///
  /// @param logger logging instance
  ProtoLayerHelper(const Config& /*config*/,
                   std::unique_ptr<const Logger> logger =
                       getDefaultLogger("ProtoLayerHelper", Logging::INFO))
      : m_logger(std::move(logger)) {}
  ~ProtoLayerHelper() = default;

  /// Sort the surfaces into ProtoLayers
  ///
  /// @param gctx The geometry context (usually building context at this stage)
  /// @param surfaces The surfaces to be sorted into arrays
  /// @param sorting The sorting setup, one single sorting
  ///
  /// @return A vector of ProtoLayers
  std::vector<ProtoLayer> protoLayers(
      const GeometryContext& gctx, const std::vector<const Surface*>& surfaces,
      const SortingConfig& sorting) const;

  /// Sort the surfaces into ProtoLayers, sequential sorting
  ///
  /// @param gctx The geometry context (usually building context at this stage)
  /// @param surfaces The surfaces to be sorted into arrays
  /// @param sortings The sequential sorting setup
  ///
  /// @return A vector of ProtoLayers
  std::vector<ProtoLayer> protoLayers(
      const GeometryContext& gctx, const std::vector<const Surface*>& surfaces,
      const std::vector<SortingConfig>& sortings) const;

 private:
  /// Logging instance
  std::unique_ptr<const Logger> m_logger;

  /// Private access to logger
  const Logger& logger() const { return *m_logger; }
};

}  // namespace Acts
