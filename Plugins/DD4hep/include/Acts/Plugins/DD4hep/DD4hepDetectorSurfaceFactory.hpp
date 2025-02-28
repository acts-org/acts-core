// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/detail/ReferenceWrapperAnyCompat.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Common.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Detector/LayerStructureBuilder.hpp"
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/BinningData.hpp"
#include "Acts/Utilities/ProtoAxis.hpp"

#include <tuple>
#include <vector>

#include <DD4hep/DD4hepUnits.h>

class TGeoMatrix;

namespace dd4hep {
class DetElement;
}

namespace Acts {

using namespace UnitLiterals;

class DD4hepDetectorElement;

/// A factory to convert DD4hep DetectorElements into sensitive
/// of passive surfaces which are filled into a Cache object,
/// also the create DD4hepDetector elements are provided
///
class DD4hepDetectorSurfaceFactory {
 public:
  /// Collect the sensitive surface & detector element
  using DD4hepSensitiveSurface =
      std::tuple<std::shared_ptr<DD4hepDetectorElement>,
                 std::shared_ptr<Surface>>;

  /// Collect the passive surfaces, bool whether it should be
  /// added as an "always try, i.e. assignToAll=true" surface
  using DD4hepPassiveSurface = std::tuple<std::shared_ptr<Surface>, bool>;

  /// Nested cache that records the conversion status
  struct Cache {
    /// The created detector elements - for the detector store
    std::vector<DD4hepSensitiveSurface> sensitiveSurfaces;
    /// The created non-const surfaces - for further processing,
    std::vector<DD4hepPassiveSurface> passiveSurfaces;
    /// matching and conversion statistics: surfaces
    std::size_t convertedSurfaces = 0;
    /// matching and conversion statistics: materials
    std::size_t convertedMaterials = 0;
    /// The collected binnings
    std::vector<std::tuple<ProtoAxis, std::size_t>> binnings = {};
    /// The collected supports
    std::vector<Experimental::ProtoSupport> supports = {};
    /// Optionally provide an Extent object to measure the sensitives
    std::optional<Extent> sExtent = std::nullopt;
    /// Optionally provide an Extent object to measure the passive
    std::optional<Extent> pExtent = std::nullopt;
    /// Optionally provide an Extent constraints to measure the layers
    std::vector<AxisDirection> extentConstraints = {};
    /// The approximination of a circle quarter for extent measuring
    std::size_t nExtentQSegments = 1u;
  };

  /// Nested options struct to steer the conversion
  struct Options {
    /// Convert sensitive surfaces
    bool convertSensitive = true;
    /// Convert passive surfaces
    bool convertPassive = true;
    /// Convert material directly
    bool convertMaterial = false;
    /// New reference material thickness for surfaces
    double surfaceMaterialThickness = 1_mm;
  };

  /// The DD4hep detector element factory
  ///
  /// @param mlogger a screen output logger
  explicit DD4hepDetectorSurfaceFactory(
      std::unique_ptr<const Logger> mlogger = getDefaultLogger(
          "DD4hepDetectorSurfaceFactory", Acts::Logging::INFO));

  /// Construction method of the detector elements
  ///
  /// @param cache [in,out] into which the Elements are filled
  /// @param gctx the geometry context
  /// @param dd4hepElement the detector element representing the super structure
  /// @param options to steer the conversion
  ///
  /// @note this method will call the recursive construction
  void construct(Cache& cache, const GeometryContext& gctx,
                 const dd4hep::DetElement& dd4hepElement,
                 const Options& options);

 private:
  /// @brief  auto-calculate the unit length conversion
  static constexpr double unitLength =
      Acts::UnitConstants::mm / dd4hep::millimeter;

  /// Logging instance
  std::unique_ptr<const Logger> m_logger;

  /// Private access to the logger
  const Logger& logger() const { return *m_logger; }

  /// Construction method of the detector elements - recursive walk down
  ///
  /// @param cache [in,out] into which the Elements are filled
  /// @param gctx the geometry context
  /// @param dd4hepElement the detector element representing the super structure
  /// @param options to steer the conversion
  /// @param level the current level of the tree, used for log message output
  ///
  /// @note this method is called recursively
  void recursiveConstruct(Cache& cache, const GeometryContext& gctx,
                          const dd4hep::DetElement& dd4hepElement,
                          const Options& options, int level);

  /// Method to convert a single sensitive detector element
  ///
  /// @param cache [in,out] into which the Elements are filled
  /// @param gctx the geometry context
  /// @param dd4hepElement the detector element
  /// @param options to steer the conversion
  ///
  /// @note the cache is handed through in order to optionally measure the
  /// extent of the sensitive surface and register it
  ///
  /// @return a created detector element and surface
  DD4hepSensitiveSurface constructSensitiveComponents(
      Cache& cache, const GeometryContext& gctx,
      const dd4hep::DetElement& dd4hepElement, const Options& options) const;

  /// Method to convert a single sensitive detector element
  ///
  /// @param cache [in,out] into which the Elements are filled
  /// @param gctx the geometry context
  /// @param dd4hepElement the detector element
  /// @param options to steer the conversion
  ///
  /// @return a created surface
  DD4hepPassiveSurface constructPassiveComponents(
      Cache& cache, const GeometryContext& gctx,
      const dd4hep::DetElement& dd4hepElement, const Options& options) const;

  /// Attach surface material if present
  ///
  /// @param gctx the geometry context
  /// @param prefix the acts prefix for the variant parameter string
  /// @param dd4hepElement the detector element
  /// @param surface the surface to attach the material to
  /// @param thickness the thickness of the condensed component
  /// @param options to steer the conversion
  ///
  /// @note the cache is handed through in order to optionally measure the
  /// extent of the sensitive surface and register it
  ///
  /// @note void function that also checks the options if the attachment should be applied
  void attachSurfaceMaterial(const GeometryContext& gctx,
                             const std::string& prefix,
                             const dd4hep::DetElement& dd4hepElement,
                             Acts::Surface& surface, double thickness,
                             const Options& options) const;
};

}  // namespace Acts
