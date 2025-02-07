// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Material/IMaterialDecorator.hpp"
#include "Acts/Material/ISurfaceMaterial.hpp"
#include "Acts/Material/IVolumeMaterial.hpp"
#include "Acts/Material/ProtoSurfaceMaterial.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include <Acts/Surfaces/AnnulusBounds.hpp>
#include <Acts/Surfaces/CylinderBounds.hpp>
#include <Acts/Surfaces/RadialBounds.hpp>
#include <Acts/Surfaces/SurfaceBounds.hpp>
#include <Acts/Surfaces/TrapezoidBounds.hpp>

#include <algorithm>
#include <fstream>
#include <map>
#include <mutex>
#include <numbers>

// Convenience shorthand

namespace Acts {

/// @brief Material decorator using a map as input
///
/// This reads in map with binning information to decorate the detector with
/// proto-material for material mapping. This allows us to change the mapping
/// parameters directly in the C++ code. Takes a tracking geometry in input, all
/// the surface with `mapMaterial=true` will be added to a binning map.
class MappingMaterialDecorator : public IMaterialDecorator {
 public:
  using BinningMap = std::map<std::uint64_t, std::pair<int, int>>;

  using VolumeMaterialMap =
      std::map<GeometryIdentifier, std::shared_ptr<const IVolumeMaterial>>;

  MappingMaterialDecorator(const Acts::TrackingGeometry& tGeometry,
                           Acts::Logging::Level level,
                           bool clearSurfaceMaterial = true,
                           bool clearVolumeMaterial = true)
      : m_clearSurfaceMaterial(clearSurfaceMaterial),
        m_clearVolumeMaterial(clearVolumeMaterial),
        m_logger{getDefaultLogger("MappingMaterialDecorator", level)} {
    volumeLoop(tGeometry.highestTrackingVolume());
  }

  /// Decorate a surface
  ///
  /// @param surface the non-cost surface that is decorated
  void decorate(Surface& surface) const final {
    ACTS_VERBOSE("Processing surface: " << surface.geometryId());
    // Clear the material if registered to do so
    if (m_clearSurfaceMaterial) {
      ACTS_VERBOSE("-> Clearing surface material");
      surface.assignSurfaceMaterial(nullptr);
    }
    // Try to find the surface in the map
    auto bins = m_binningMap.find(surface.geometryId().value());
    if (bins != m_binningMap.end()) {
      ACTS_VERBOSE("-> Found material for surface, assigning");
      surface.assignSurfaceMaterial(
          binnedSurfaceMaterial(surface.getSharedPtr()));
    }
  }

  /// Decorate a TrackingVolume
  ///
  /// @param volume the non-cost volume that is decorated
  void decorate(TrackingVolume& volume) const final {
    ACTS_VERBOSE("Processing volume: " << volume.geometryId());
    // Clear the material if registered to do so
    if (m_clearVolumeMaterial) {
      ACTS_VERBOSE("-> Clearing volume material");
      volume.assignVolumeMaterial(nullptr);
    }
    // Try to find the volume in the map
    auto vMaterial = m_volumeMaterialMap.find(volume.geometryId());
    if (vMaterial != m_volumeMaterialMap.end()) {
      ACTS_VERBOSE("-> Found material for volume, assigning");
      volume.assignVolumeMaterial(vMaterial->second);
    }
  }

  /// Loop over all subvolumes and there surfaces and add all the surface with
  /// protomaterial to the binning map.
  ///
  /// @param volume to be looped onto
  void volumeLoop(const Acts::TrackingVolume* tVolume) {
    auto sameId = [tVolume](const auto& pair) {
      return (tVolume->geometryId() == pair.first);
    };
    if (std::ranges::any_of(m_volumeMaterialMap, sameId)) {
      // this volume was already visited
      return;
    }
    if (tVolume->volumeMaterial() != nullptr) {
      m_volumeMaterialMap.insert(
          {tVolume->geometryId(), tVolume->volumeMaterialPtr()});
    }
    // there are confined volumes
    if (tVolume->confinedVolumes() != nullptr) {
      // get through the volumes
      auto& volumes = tVolume->confinedVolumes()->arrayObjects();
      // loop over the volumes
      for (auto& vol : volumes) {
        // recursive call
        volumeLoop(vol.get());
      }
    }
    // there are dense volumes
    if (!tVolume->denseVolumes().empty()) {
      // loop over the volumes
      for (auto& vol : tVolume->denseVolumes()) {
        // recursive call
        volumeLoop(vol.get());
      }
    }
    if (tVolume->confinedLayers() != nullptr) {
      // get the layers
      auto& layers = tVolume->confinedLayers()->arrayObjects();
      // loop over the layers
      for (auto& lay : layers) {
        auto& layRep = lay->surfaceRepresentation();
        if (layRep.surfaceMaterial() != nullptr &&
            layRep.geometryId() != GeometryIdentifier()) {
          m_binningMap.insert(
              {layRep.geometryId().value(), std::make_pair(1, 1)});
        }
        if (lay->approachDescriptor() != nullptr) {
          for (auto& asf : lay->approachDescriptor()->containedSurfaces()) {
            if (asf->surfaceMaterial() != nullptr) {
              m_binningMap.insert(
                  {asf->geometryId().value(), std::make_pair(1, 1)});
            }
          }
        }
        if (lay->surfaceArray() != nullptr) {
          for (auto& ssf : lay->surfaceArray()->surfaces()) {
            if (ssf->surfaceMaterial() != nullptr) {
              m_binningMap.insert(
                  {ssf->geometryId().value(), std::make_pair(1, 1)});
            }
          }
        }
      }
    }
    // Let's finally check the boundaries
    for (auto& bsurf : tVolume->boundarySurfaces()) {
      // the surface representation
      auto& bssfRep = bsurf->surfaceRepresentation();
      if (bssfRep.geometryId().volume() == tVolume->geometryId().volume()) {
        if (bssfRep.surfaceMaterial() != nullptr) {
          m_binningMap.insert(
              {bssfRep.geometryId().value(), std::make_pair(1, 1)});
        }
      }
    }
  }

  /// Add protomaterial to a surface bases on the binning map
  ///
  /// @param surface protomaterial will be added to
  std::shared_ptr<const Acts::ISurfaceMaterial> binnedSurfaceMaterial(
      const std::shared_ptr<const Acts::Surface>& surface) const {
    auto bin = m_binningMap.find(surface->geometryId().value());
    Acts::BinUtility bUtility;
    if (bin == m_binningMap.end()) {
      ACTS_ERROR("No corresponding binning in the map to surface "
                 << surface->geometryId());
    } else {
      auto binning = bin->second;
      // Check which type of bounds is associated to the surface
      const Acts::SurfaceBounds& surfaceBounds = surface->bounds();
      const Acts::RadialBounds* radialBounds =
          dynamic_cast<const Acts::RadialBounds*>(&surfaceBounds);
      const Acts::CylinderBounds* cylinderBounds =
          dynamic_cast<const Acts::CylinderBounds*>(&surfaceBounds);
      const Acts::AnnulusBounds* annulusBounds =
          dynamic_cast<const Acts::AnnulusBounds*>(&surfaceBounds);
      const Acts::RectangleBounds* rectangleBounds =
          dynamic_cast<const Acts::RectangleBounds*>(&surfaceBounds);
      const Acts::TrapezoidBounds* trapezoidBounds =
          dynamic_cast<const Acts::TrapezoidBounds*>(&surfaceBounds);

      if (radialBounds != nullptr) {
        bUtility += Acts::BinUtility(
            binning.first,
            radialBounds->get(Acts::RadialBounds::eAveragePhi) -
                radialBounds->get(Acts::RadialBounds::eHalfPhiSector),
            radialBounds->get(Acts::RadialBounds::eAveragePhi) +
                radialBounds->get(Acts::RadialBounds::eHalfPhiSector),
            (radialBounds->get(Acts::RadialBounds::eHalfPhiSector) -
             std::numbers::pi) < Acts::s_epsilon
                ? Acts::closed
                : Acts::open,
            Acts::AxisDirection::AxisPhi);
        bUtility += Acts::BinUtility(binning.second,
                                     static_cast<float>(radialBounds->rMin()),
                                     static_cast<float>(radialBounds->rMax()),
                                     Acts::open, Acts::AxisDirection::AxisR);
      }
      if (cylinderBounds != nullptr) {
        bUtility += Acts::BinUtility(
            binning.first,
            cylinderBounds->get(Acts::CylinderBounds::eAveragePhi) -
                cylinderBounds->get(Acts::CylinderBounds::eHalfPhiSector),
            cylinderBounds->get(Acts::CylinderBounds::eAveragePhi) +
                cylinderBounds->get(Acts::CylinderBounds::eHalfPhiSector),
            (cylinderBounds->get(Acts::CylinderBounds::eHalfPhiSector) -
             std::numbers::pi) < Acts::s_epsilon
                ? Acts::closed
                : Acts::open,
            Acts::AxisDirection::AxisPhi);
        bUtility += Acts::BinUtility(
            binning.second,
            -1 * cylinderBounds->get(Acts::CylinderBounds::eHalfLengthZ),
            cylinderBounds->get(Acts::CylinderBounds::eHalfLengthZ), Acts::open,
            Acts::AxisDirection::AxisZ);
      }
      if (annulusBounds != nullptr) {
        bUtility += Acts::BinUtility(
            binning.first, annulusBounds->get(Acts::AnnulusBounds::eMinPhiRel),
            annulusBounds->get(Acts::AnnulusBounds::eMaxPhiRel), Acts::open,
            Acts::AxisDirection::AxisPhi);
        bUtility += Acts::BinUtility(binning.second,
                                     static_cast<float>(annulusBounds->rMin()),
                                     static_cast<float>(annulusBounds->rMax()),
                                     Acts::open, Acts::AxisDirection::AxisR);
      }
      if (rectangleBounds != nullptr) {
        bUtility += Acts::BinUtility(
            binning.first, rectangleBounds->get(Acts::RectangleBounds::eMinX),
            rectangleBounds->get(Acts::RectangleBounds::eMaxX), Acts::open,
            Acts::AxisDirection::AxisX);
        bUtility += Acts::BinUtility(
            binning.second, rectangleBounds->get(Acts::RectangleBounds::eMinY),
            rectangleBounds->get(Acts::RectangleBounds::eMaxY), Acts::open,
            Acts::AxisDirection::AxisY);
      }
      if (trapezoidBounds != nullptr) {
        double halfLengthX = std::max(
            trapezoidBounds->get(Acts::TrapezoidBounds::eHalfLengthXnegY),
            trapezoidBounds->get(Acts::TrapezoidBounds::eHalfLengthXposY));
        bUtility += Acts::BinUtility(binning.first,
                                     static_cast<float>(-1 * halfLengthX),
                                     static_cast<float>(halfLengthX),
                                     Acts::open, Acts::AxisDirection::AxisX);
        bUtility += Acts::BinUtility(
            binning.second,
            -1 * trapezoidBounds->get(Acts::TrapezoidBounds::eHalfLengthY),
            trapezoidBounds->get(Acts::TrapezoidBounds::eHalfLengthY),
            Acts::open, Acts::AxisDirection::AxisY);
      }
    }
    return std::make_shared<Acts::ProtoSurfaceMaterial>(bUtility);
  }

  /// Readonly access to the BinningMap
  BinningMap& binningMap() { return m_binningMap; }

  /// set the binning map
  void setBinningMap(BinningMap binningMap) {
    m_binningMap = std::move(binningMap);
  }

 private:
  BinningMap m_binningMap;

  VolumeMaterialMap m_volumeMaterialMap;

  bool m_clearSurfaceMaterial{true};
  bool m_clearVolumeMaterial{true};

  std::unique_ptr<const Logger> m_logger;

  const Logger& logger() const { return *m_logger; }
};
}  // namespace Acts
