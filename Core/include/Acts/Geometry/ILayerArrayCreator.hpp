// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Utilities/BinnedArray.hpp"
#include "Acts/Utilities/BinningType.hpp"

#include <memory>
#include <vector>

namespace Acts {

class Layer;
/// A std::shared_ptr to a Layer
using LayerPtr = std::shared_ptr<const Layer>;
/// A BinnedArray to a std::shared_ptr of a layer
using LayerArray = BinnedArray<LayerPtr>;
/// A vector of std::shared_ptr to layers
using LayerVector = std::vector<LayerPtr>;

/// @class ILayerArrayCreator
///
/// Interface class ILayerArrayCreators, it inherits from IAlgTool.
///
/// It receives the LayerVector and creaets an array with NaivgationLayer
/// objects
/// filled in between.
class ILayerArrayCreator {
 public:
  /// Virtual destructor
  virtual ~ILayerArrayCreator() = default;

  /// LayerArrayCreator interface method
  ///
  /// @param gctx is the geometry context for witch the volume is built
  /// @param layers are the layers to be moved into an array
  /// @param min is the minimul value for binning
  /// @param max is the maximum value for binning
  /// @param btype is the binning type
  /// @param bvalue is the value in which the binning should be done
  ///
  /// @return unique pointer to a new LayerArray
  virtual std::unique_ptr<const LayerArray> layerArray(
      const GeometryContext& gctx, const LayerVector& layers, double min,
      double max, BinningType btype = arbitrary,
      AxisDirection bvalue = AxisDirection::AxisX) const = 0;
};
}  // namespace Acts
