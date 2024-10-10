// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/ILayerBuilder.hpp"
#include "Acts/Geometry/LayerCreator.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <memory>
#include <string>
#include <vector>

#include <DD4hep/DetElement.h>

class TGeoMatrix;

namespace Acts {
class LayerCreator;
class Logger;
class Surface;

/// @brief build layers of one cylinder-endcap setup from DD4hep input
///
/// This class is an implementation of the Acts::ILayerBuilder,
/// creating the central (layers of barrel), the negative and positive layers
/// (layers of endcaps) of one hierarchy (e.g. PixelDetector, StripDetector,...)
/// with input from DD4hep.

class DD4hepLayerBuilder : public ILayerBuilder {
 public:
  /// @struct Config
  /// nested configuration struct for steering of the layer builder
  struct Config {
    /// string based identification
    std::string configurationName = "undefined";
    /// layer creator which is internally used to build layers
    std::shared_ptr<const LayerCreator> layerCreator = nullptr;
    /// the binning type of the contained surfaces in phi
    /// (equidistant/arbitrary)
    BinningType bTypePhi = equidistant;
    /// the binning type of the contained surfaces in r
    /// (equidistant/arbitrary)
    BinningType bTypeR = equidistant;
    /// the binning type of the contained surfaces in z
    /// (equidistant/arbitrary)
    BinningType bTypeZ = equidistant;
    /// the DD4hep::DetElements of the layers of the negative volume (negative
    /// endcap)
    /// @note if the current volume has no endcaps or no layers this parameter
    /// will not be set
    std::vector<dd4hep::DetElement> negativeLayers;
    /// the DD4hep::DetElements of the layers of the central volume (barrel)
    /// @note if the current volume has no layers this parameter will not be set
    std::vector<dd4hep::DetElement> centralLayers;
    /// the DD4hep::DetElements of the layers of the positive volume (positive
    /// endcap)
    /// @note if the current volume has no endcaps or no layers this parameter
    /// will not be set
    std::vector<dd4hep::DetElement> positiveLayers;
    /// In case no surfaces (to be contained by the layer) are handed over, the
    /// layer thickness will be set to this value
    /// @note Layers containing surfaces per default are not allowed to be
    ///       attached to each other (navigation will bail at this point).
    ///       However, to allow material layers (not containing surfaces) to be
    ///       attached to each other, this default thickness is needed. In this
    ///       way, the layer will be thin (with space to the next layer), but
    ///       the material will have the 'real' thickness.
    /// @attention The default thickness should be set thin enough that no
    ///            touching or overlapping with the next layer can happen.
    double defaultThickness = UnitConstants::fm;
  };

  /// Constructor
  /// @param config is the configuration struct
  /// @param logger is the logging instance
  DD4hepLayerBuilder(const Acts::DD4hepLayerBuilder::Config& config,
                     std::unique_ptr<const Logger> logger);
  /// Destructor
  ~DD4hepLayerBuilder() override;

  /// LayerBuilder interface method
  ///
  /// @param gctx the geometry context for this build call
  ///
  /// @return  the layers at negative side
  const LayerVector negativeLayers(const GeometryContext& gctx) const final;

  /// LayerBuilder interface method
  ///
  /// @param gctx the geometry context for this build call
  ///
  /// @return the layers at the central sector
  const LayerVector centralLayers(const GeometryContext& gctx) const final;

  /// LayerBuilder interface method
  ///
  /// @param gctx the geometry context for this build call
  ///
  /// @return  the layers at positive side
  const LayerVector positiveLayers(const GeometryContext& gctx) const final;

  /// Name identification
  /// @return the string based identification of this configuration
  const std::string& identification() const final;

  /// set the configuration object
  /// @param config is the configuration struct
  void setConfiguration(const Config& config);

  /// get the configuration object
  Config getConfiguration() const;

  /// set logging instance
  void setLogger(std::unique_ptr<const Logger> logger);

 private:
  /// configuration object
  Config m_cfg;

  /// logging instance
  std::unique_ptr<const Logger> m_logger;

  /// Private access to the logger
  const Logger& logger() const { return *m_logger; }

  /// Private helper method to be called for endcap layers
  ///
  /// @param gctx the geometry context for this build call
  /// @param dendcapLayers Vector of detector elements for the endcap layers
  /// @param side Which endcap side it is
  ///
  /// @return  the layers for either endcap side
  const LayerVector endcapLayers(
      const GeometryContext& gctx,
      const std::vector<dd4hep::DetElement>& dendcapLayers,
      const std::string& side) const;

  /// Private helper function collecting all sensitive detector elements of a
  /// layer
  /// @param detElement the DD4hep::DetElement of the layer
  /// @param surfaces the vector of surfaces which should be filled with the
  /// sensitive detector elements
  void resolveSensitive(
      const dd4hep::DetElement& detElement,
      std::vector<std::shared_ptr<const Acts::Surface>>& surfaces) const;

  /// Private helper function to create a sensitive surface from a given
  /// detector element
  /// @param detElement the DD4hep::DetElement of sensitive surface to be
  /// created
  /// @param isDisc in case the sensitive detector module should be translated
  ///        as disc (e.g. for endcaps) this flag should be set to true
  std::shared_ptr<const Acts::Surface> createSensitiveSurface(
      const dd4hep::DetElement& detElement, bool isDisc = false) const;

  // Private helper function to convert the TGeo transformation matrix into
  // an Acts transformation matrix
  // @param tGeoTrans TGeo transformation matrix which should be converted
  Acts::Transform3 convertTransform(const TGeoMatrix* tGeoTrans) const;
};

inline const std::string& DD4hepLayerBuilder::identification() const {
  return m_cfg.configurationName;
}

inline DD4hepLayerBuilder::Config DD4hepLayerBuilder::getConfiguration() const {
  return m_cfg;
}

}  // namespace Acts
