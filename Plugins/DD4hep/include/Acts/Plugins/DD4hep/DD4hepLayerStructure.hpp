// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Units.hpp"
#include "Acts/Detector/LayerStructureBuilder.hpp"
#include "Acts/Detector/ProtoBinning.hpp"
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Plugins/DD4hep/DD4hepDetectorElement.hpp"
#include "Acts/Plugins/DD4hep/DD4hepDetectorSurfaceFactory.hpp"
#include "Acts/Utilities/BinningData.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <memory>
#include <optional>
#include <string>
#include <tuple>

namespace dd4hep {
class DetElement;
}

namespace Acts::Experimental {

/// @brief This class allows to generate layer structure builders for dd4hep sub detectors
/// It performs an intermediate step by taking dd4hep::DetElemnent objects that
/// describe a detector structure and prepares the translation of the detector
/// element and eventual passive surfaces.
///
/// It would also build passive support structures if configured to do so.
///
class DD4hepLayerStructure {
 public:
  /// Constructor with DD4hepDetectorSurfaceFactory
  ///
  /// @param surfaceFactory the surfac factory which converts dd4hep::DetElement objects
  /// into their Acts coutnerparts
  /// @param logger is the screen output logger
  ///
  /// @note this needs to be provided
  DD4hepLayerStructure(
      std::shared_ptr<DD4hepDetectorSurfaceFactory> surfaceFactory,
      std::unique_ptr<const Logger> logger =
          getDefaultLogger("DD4hepLayerStructure", Acts::Logging::INFO));

  DD4hepLayerStructure() = delete;

  /// @brief Nested options struct
  ///
  /// If a binning description or a support cylinder
  /// description is chosen through options, it overwrites the corresponding
  /// description coming from DD4hep.
  struct Options {
    /// The name of the object
    std::string name = "";
    /// An out put log level
    Logging::Level logLevel = Logging::INFO;
    // The extent structure - optionally
    std::optional<Extent> extent = std::nullopt;
    /// The extent constraints - optionally
    std::vector<BinningValue> extentConstraints = {};
    /// Approximation for the polyhedron binning
    unsigned int quarterSegments = 1u;
    /// Patch the binning with the extent if possible
    bool patchBinningWithExtent = true;
    /// Conversion options
    DD4hepDetectorSurfaceFactory::Options conversionOptions;
  };

  /// @brief This method generates a LayerStructure builder, which extends the
  /// IInternalStructureBuilder and can be used in the conjunction with a
  /// IExternalStructureBuilder to create `DetectorVolume` objects.
  ///
  /// It takes the detector element from DD4hep and some optional parameters
  ///
  /// @param dd4hepStore [in, out] the detector store for the built elements
  /// @param gctx the geometry context
  /// @param dd4hepElement the dd4hep detector element
  /// @param options containing the optional descriptions
  ///
  /// @return a LayerStructureBuilder, and an optional extent
  std::tuple<std::shared_ptr<LayerStructureBuilder>, std::optional<Extent>>
  builder(DD4hepDetectorElement::Store& dd4hepStore,
          const GeometryContext& gctx, const dd4hep::DetElement& dd4hepElement,
          const Options& options) const;

 private:
  /// The workorse: the surface factory
  std::shared_ptr<DD4hepDetectorSurfaceFactory> m_surfaceFactory = nullptr;

  /// Logging instance
  std::unique_ptr<const Logger> m_logger;

  /// Private access to the logger
  const Logger& logger() const { return *m_logger; }
};

}  // namespace Acts::Experimental
