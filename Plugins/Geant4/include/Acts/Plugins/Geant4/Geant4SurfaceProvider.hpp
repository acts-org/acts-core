// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Detector/KdtSurfacesProvider.hpp"
#include "Acts/Detector/interface/ISurfacesProvider.hpp"
#include "Acts/Plugins/Geant4/Geant4DetectorSurfaceFactory.hpp"

#include "G4GDMLParser.hh"
#include "G4LogicalVolume.hh"
#include "G4VPhysicalVolume.hh"

namespace Acts::Experimental {

/// @brief A surface provider that extracts surfaces from a gdml file
///
/// This provider extracts volumes from a gdml file based on
/// the preselection criteria and converts them to surfaces.
/// By default, all the volumes are converted.
///
/// Optionally, it can be configured to return a range-based
/// subset of all the preselected surfaces. This is done
/// by setting the range and binning values in the kdtOptions
///
/// @note if the KDTree selection is not needed, the
/// template parameters can be left to their default values
/// as they will not affect the result.
///
/// @tparam kDim The number of dimensions for the KDTree
/// @tparam bSize The maximum number of surfaces per KDTree leaf
/// @tparam reference_generator The reference generator for the KDTree
template <std::size_t kDim = 2u, std::size_t bSize = 100u,
          typename reference_generator =
              detail::PolyhedronReferenceGenerator<1u, false>>
class Geant4SurfaceProvider : public Acts::Experimental::ISurfacesProvider {
 public:
  /// Nested configuration struct
  struct Config {
    /// Pointer to the g4World volume
    const G4VPhysicalVolume* g4World = nullptr;

    /// Convert the length scale
    ActsScalar scaleConversion = 1.;

    /// Convert the material
    bool convertMaterial = true;

    /// Converted material thickness (< 0 indicates keeping original thickness)
    ActsScalar convertedMaterialThickness = -1;

    /// Transformation to apply to the
    /// G4World volume
    G4Transform3D worldTransform = G4Transform3D();

    /// A selector for passive surfaces
    std::shared_ptr<IGeant4PhysicalVolumeSelector> surfacePreselector =
        std::make_shared<Acts::Geant4PhysicalVolumeSelectors::AllSelector>();
  };

  /// Optional configuration for the KDTree
  struct kdtOptions {
    /// A set of ranges to separate the surfaces
    Acts::RangeXD<kDim, Acts::ActsScalar> range;

    /// A set of binning values to perform the separation
    std::array<Acts::BinningValue, kDim> binningValues;

    /// The maximum number of surfaces per leaf
    std::size_t leafSize = bSize;

    /// The reference generator for the KDTree
    reference_generator rgen;

    /// Initialize range to be degenerate by default
    kdtOptions() {
      for (std::size_t i = 0; i < kDim; ++i) {
        range[i].set(1, -1);
      }
    }
  };

  /// Constructor
  /// @param config The configuration struct
  /// @param options The optional configuration for KDTree
  Geant4SurfaceProvider(const Config& config,
                        const kdtOptions& options = kdtOptions()) {
    if (config.g4World == nullptr) {
      throw std::invalid_argument(
          "Geant4SurfaceProvider: No World volume provided");
    }
    if (config.surfacePreselector == nullptr) {
      throw std::invalid_argument(
          "Geant4SurfaceProvider: no preselection criteria provided");
    }

    m_cfg = config;
    m_kdtOptions = options;
    m_g4World = m_cfg.g4World;
    m_g4ToWorld = m_cfg.worldTransform;
  };

  /// Destructor
  ~Geant4SurfaceProvider() override = default;

  std::vector<std::shared_ptr<Acts::Surface>> surfaces(
      [[maybe_unused]] const Acts::GeometryContext& gctx) const override {
    /// Surface factory options
    Acts::Geant4DetectorSurfaceFactory::Options g4SurfaceOptions;

    /// Copy the configuration
    /// This is done to avoid checking nullptrs
    /// in the factory
    g4SurfaceOptions.scaleConversion = m_cfg.scaleConversion;
    g4SurfaceOptions.convertMaterial = m_cfg.convertMaterial;
    g4SurfaceOptions.convertedMaterialThickness =
        m_cfg.convertedMaterialThickness;
    g4SurfaceOptions.passiveSurfaceSelector = m_cfg.surfacePreselector;

    /// Generate the surface cache
    Acts::Geant4DetectorSurfaceFactory::Cache g4SurfaceCache;

    /// Find and store surfaces in the cache object
    auto g4ToWorldConsistent = G4Transform3D(
        m_g4ToWorld.getRotation().inverse(), m_g4ToWorld.getTranslation());
    Acts::Geant4DetectorSurfaceFactory{}.construct(
        g4SurfaceCache, g4ToWorldConsistent, *m_g4World, g4SurfaceOptions);

    auto surfaces = g4SurfaceCache.passiveSurfaces;

    /// If range is degenerate, return all surfaces
    if (m_kdtOptions.range.degenerate()) {
      return surfaces;
    }

    /// Otherwise, select the surfaces based on the range
    auto kdtSurfaces =
        Acts::Experimental::KdtSurfaces<kDim, bSize, reference_generator>(
            gctx, surfaces, m_kdtOptions.binningValues);

    return kdtSurfaces.surfaces(m_kdtOptions.range);
  };

 private:
  Config m_cfg;

  kdtOptions m_kdtOptions;

  const G4VPhysicalVolume* m_g4World;

  G4Transform3D m_g4ToWorld;
};

}  // namespace Acts::Experimental
