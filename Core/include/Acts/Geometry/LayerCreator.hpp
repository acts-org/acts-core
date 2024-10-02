// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/ApproachDescriptor.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/ProtoLayer.hpp"
#include "Acts/Surfaces/SurfaceArray.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

namespace Acts {

namespace Test {
struct LayerCreatorFixture;
}
class Surface;
class SurfaceArrayCreator;
class Layer;

using MutableLayerPtr = std::shared_ptr<Layer>;

/// @class LayerCreator
///
/// The LayerCreator is able to build cylinder disc layers or plane layers from
/// detector elements
///
class LayerCreator {
 public:
  friend Acts::Test::LayerCreatorFixture;
  ///  @struct Config
  ///  Configuration for the LayerCreator
  ///  This is the nexted configuration struct for the LayerCreator class
  struct Config {
    /// surface array helper
    std::shared_ptr<const SurfaceArrayCreator> surfaceArrayCreator = nullptr;
    /// cylinder module z tolerance: it counts as same z, if ...
    double cylinderZtolerance{10.};
    /// cylinder module phi tolerance: it counts as same phi, if ...
    double cylinderPhiTolerance{0.1};
    /// standard constructor
    Config() = default;
  };

  /// Constructor
  ///
  /// @param lcConfig is the configuration object
  /// @param logger logging instance
  LayerCreator(const Config& lcConfig,
               std::unique_ptr<const Logger> logger =
                   getDefaultLogger("LayerCreator", Logging::INFO));

  /// Destructor
  ~LayerCreator() = default;

  /// returning a cylindrical layer
  ///
  /// @param gctx is the geometry context with which the geometry is built
  /// @param surfaces is the vector of pointers to sensitive surfaces
  /// represented by this layer
  /// @pre the pointers to the sensitive surfaces in the surfaces vectors all
  /// need to be valid, since no check is performed
  /// @param binsPhi is number of bins the sensitive surfaces are ordered in phi
  /// @param binsZ is number of bins the sensitive surfaces are ordered in Z
  /// @param _protoLayer (optional) proto layer specifying the dimensions and
  /// envelopes
  /// @param transform is the (optional) transform of the layer
  /// @param ad possibility to hand over a specific ApproachDescriptor, which is
  /// needed for material mapping. Otherwise the default ApproachDescriptor will
  /// be taken used for this layer
  ///
  /// @return shared pointer to a newly created layer
  MutableLayerPtr cylinderLayer(
      const GeometryContext& gctx,
      std::vector<std::shared_ptr<const Surface>> surfaces, std::size_t binsPhi,
      std::size_t binsZ, std::optional<ProtoLayer> _protoLayer = std::nullopt,
      const Transform3& transform = Transform3::Identity(),
      std::unique_ptr<ApproachDescriptor> ad = nullptr) const;

  /// returning a cylindrical layer
  ///
  /// @param gctx is the geometry context with which the geometry is built
  /// @param surfaces is the vector of pointers to sensitive surfaces
  /// represented by this layer
  /// @pre the pointers to the sensitive surfaces in the surfaces vectors all
  /// need to be valid, since no check is performed
  /// @param bTypePhi binning type in phi (equidistant/arbitrary)
  /// @param bTypeZ binning type in z (equidistant/arbitrary)
  /// @param _protoLayer (optional) proto layer specifying the dimensions and
  /// envelopes
  /// @param transform is the (optional) transform of the layer
  /// @param ad possibility to hand over a specific ApproachDescriptor, which is
  /// needed for material mapping. Otherwise the default ApproachDescriptor will
  /// be taken used for this layer
  ///
  /// @return shared pointer to a newly created layer
  MutableLayerPtr cylinderLayer(
      const GeometryContext& gctx,
      std::vector<std::shared_ptr<const Surface>> surfaces,
      BinningType bTypePhi, BinningType bTypeZ,
      std::optional<ProtoLayer> _protoLayer = std::nullopt,
      const Transform3& transform = Transform3::Identity(),
      std::unique_ptr<ApproachDescriptor> ad = nullptr) const;

  /// returning a disc layer
  ///
  /// @param gctx is the geometry context with which the geometry is built
  /// @param surfaces is the vector of pointers to sensitive surfaces
  /// represented by this layer
  /// @pre the pointers to the sensitive surfaces in the surfaces vectors all
  /// need to be valid, since no check is performed
  /// @param binsR is number of bins the sensitive surfaces are ordered in R
  /// @param binsPhi is number of bins the sensitive surfaces are ordered in Phi
  /// @param transform is the (optional) transform of the layer
  /// @param _protoLayer (optional) proto layer specifying the dimensions and
  /// envelopes
  /// @param ad possibility to hand over a specific ApproachDescriptor, which is
  /// needed for material mapping. Otherwise the default ApproachDescriptor will
  /// be taken used for this layer
  ///
  /// @return shared pointer to a newly created layer
  MutableLayerPtr discLayer(
      const GeometryContext& gctx,
      std::vector<std::shared_ptr<const Surface>> surfaces, std::size_t binsR,
      std::size_t binsPhi, std::optional<ProtoLayer> _protoLayer = std::nullopt,
      const Transform3& transform = Transform3::Identity(),
      std::unique_ptr<ApproachDescriptor> ad = nullptr) const;

  /// returning a disc layer
  ///
  /// @param gctx is the geometry context with which the geometry is built
  /// @param surfaces is the vector of pointers to sensitive surfaces
  /// represented by this layer
  /// @pre the pointers to the sensitive surfaces in the surfaces vectors all
  /// need to be valid, since no check is performed
  /// @param bTypeR binning type in r (equidistant/arbitrary)
  /// @param bTypePhi binning type in phi (equidistant/arbitrary)
  /// @param transform is the (optional) transform of the layer
  /// @param _protoLayer (optional) proto layer specifying the dimensions and
  /// envelopes
  /// @param ad possibility to hand over a specific ApproachDescriptor, which is
  /// needed for material mapping. Otherwise the default ApproachDescriptor will
  /// be taken used for this layer
  ///
  /// @return shared pointer to a newly created layer
  MutableLayerPtr discLayer(
      const GeometryContext& gctx,
      std::vector<std::shared_ptr<const Surface>> surfaces, BinningType bTypeR,
      BinningType bTypePhi,
      std::optional<ProtoLayer> _protoLayer = std::nullopt,
      const Transform3& transform = Transform3::Identity(),
      std::unique_ptr<ApproachDescriptor> ad = nullptr) const;

  /// returning a plane layer
  ///
  /// @param gctx is the geometry context with which the geometry is built
  /// @param [in] surfaces is the vector of pointers to sensitive surfaces
  /// represented by this layer
  /// @pre the pointers to the sensitive surfaces in the surfaces vectors all
  /// need to be valid, since no check is performed
  /// @param [in] bins1 is the number of bins in the orthogonal direction to @p
  /// bValue
  /// @param [in] bins2 is the number of bins in the orthogonal direction to @p
  /// bValue
  /// @param [in] bValue Direction of the aligned surfaces
  /// @param [in] transform is the (optional) transform of the layer
  /// @param [in] _protoLayer (optional) proto layer specifying the dimensions
  /// and
  /// envelopes
  /// @param [in] ad possibility to hand over a specific ApproachDescriptor,
  /// which is needed for material mapping. Otherwise the default
  /// ApproachDescriptor will be taken used for this layer
  ///
  /// @return shared pointer to a newly created layer
  MutableLayerPtr planeLayer(
      const GeometryContext& gctx,
      std::vector<std::shared_ptr<const Surface>> surfaces, std::size_t bins1,
      std::size_t bins2, BinningValue bValue,
      std::optional<ProtoLayer> _protoLayer = std::nullopt,
      const Transform3& transform = Transform3::Identity(),
      std::unique_ptr<ApproachDescriptor> ad = nullptr) const;

  /// Set the configuration object
  /// @param lcConfig is the configuration struct
  void setConfiguration(const Config& lcConfig);

  /// Access th configuration object
  Config getConfiguration() const;

  /// set logging instance
  /// @param newLogger the logger instance
  void setLogger(std::unique_ptr<const Logger> newLogger);

  /// associate surfaces contained by this layer to this layer
  void associateSurfacesToLayer(Layer& layer) const;

 private:
  /// Validates that all the sensitive surfaces are actually accessible through
  /// the binning
  ///
  /// @param gctx Geometry context to work with
  /// @param sArray @c SurfaceArray instance to check
  bool checkBinning(const GeometryContext& gctx,
                    const SurfaceArray& sArray) const;

  /// configuration object
  Config m_cfg;

  /// Private access method to the logger
  const Logger& logger() const { return *m_logger; }

  /// logging instance
  std::unique_ptr<const Logger> m_logger;
};

inline LayerCreator::Config LayerCreator::getConfiguration() const {
  return m_cfg;
}

}  // namespace Acts
