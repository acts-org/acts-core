// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/ILayerArrayCreator.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <memory>
#include <utility>

namespace Acts {

class Surface;
class Layer;

/// @class LayerArrayCreator
///  The LayerArrayCreator is a simple Tool that helps to construct
///  LayerArrays from std::vector of Acts::CylinderLayer, Acts::DiscLayer,
/// Acts::PlaneLayer.
///
///  It fills the gaps automatically with Acts::NavigationLayer to be processed
/// easily in the
///  Navigation of the Extrapolation process.
///

class LayerArrayCreator : public ILayerArrayCreator {
 public:
  /// @brief This struct stores the configuration of the tracking geometry
  struct Config {};

  /// Constructor
  ///
  /// @param logger logging instance
  LayerArrayCreator(const Config& /*cfg*/,
                    std::unique_ptr<const Logger> logger =
                        getDefaultLogger("LayerArrayCreator", Logging::INFO))
      : m_logger(std::move(logger)) {}

  /// Destructor
  ~LayerArrayCreator() override = default;

  /// LayerArrayCreator interface method
  ///
  /// @param gctx is the geometry context for witch the array is built
  /// @param layersInput are the layers to be moved into an array
  /// @param min is the minimum value for binning
  /// @param max is the maximum value for binning
  /// @param bType is the binning type
  /// @param bValue is the value in which the binning should be done
  ///
  /// @return unique pointer to a newly created LayerArray
  std::unique_ptr<const LayerArray> layerArray(
      const GeometryContext& gctx, const LayerVector& layersInput, double min,
      double max, BinningType bType = arbitrary,
      BinningValue bValue = BinningValue::binX) const override;

  /// set logging instance
  void setLogger(std::unique_ptr<const Logger> logger) {
    m_logger = std::move(logger);
  }

 private:
  /// Private access method to the logging instance
  const Logger& logger() const { return *m_logger; }

  /// logging instance
  std::unique_ptr<const Logger> m_logger = nullptr;

  /// Private helper method for creating a surface for
  /// the NavigationLayer, it clones the
  /// @param layer object and thus needs the
  /// @param gctx geometry context.
  ///
  /// @param bValue is the Binning value for the layer array
  /// @param offset is the sift for the navigation layer
  std::shared_ptr<Surface> createNavigationSurface(const GeometryContext& gctx,
                                                   const Layer& layer,
                                                   BinningValue bValue,
                                                   double offset) const;
};

}  // namespace Acts
