// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/ITrackingVolumeBuilder.hpp"
#include "Acts/Geometry/ITrackingVolumeHelper.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Acts {

class IVolumeMaterial;
class ISurfaceMaterial;
class ILayerBuilder;
class IConfinedTrackingVolumeBuilder;

/// @enum WrappingCondition
enum WrappingCondition {
  Undefined = 0,         ///< inconsistency detected
  Attaching = 1,         ///< attach the volumes
  Inserting = 2,         ///< insert the new volume
  Wrapping = 3,          ///< wrap the new volume around
  CentralInserting = 4,  ///< insert the new one into the center
  CentralWrapping = 5,   ///< wrap the new central volume around
  NoWrapping = 6         ///< no inner volume present - no wrapping needed
};

/// VolumeConfig struct to understand the layer config
struct VolumeConfig {
  bool present{false};                  ///< layers are present
  bool wrapping{false};                 ///< in what way they are binned
  double rMin;                          ///< min parameter r
  double rMax;                          ///< max parameter r
  double zMin;                          ///< min parameter z
  double zMax;                          ///< max parameter z
  LayerVector layers;                   ///< the layers you have
  MutableTrackingVolumeVector volumes;  ///< the confined volumes you have

  /// Default constructor
  VolumeConfig()
      : rMin(std::numeric_limits<double>::max()),
        rMax(std::numeric_limits<double>::lowest()),
        zMin(std::numeric_limits<double>::max()),
        zMax(std::numeric_limits<double>::lowest()),
        layers() {}

  /// Adapt to the dimensions of another config in Z
  /// it will take the maximum/minimum values and just overwrite them
  ///
  /// @param [in] lConfig is the config to which it should be adapded
  void adaptZ(const VolumeConfig& lConfig) {
    if (lConfig) {
      zMin = std::min(zMin, lConfig.zMin);
      zMax = std::max(zMax, lConfig.zMax);
    }
  }

  /// Adapt to the dimensions of another config in R
  /// it will take the maximum/minimum values and just overwrite them
  ///
  /// @param [in] lConfig is the config to which it should be adapded
  void adaptR(const VolumeConfig& lConfig) {
    if (lConfig) {
      rMin = std::min(rMin, lConfig.rMin);
      rMax = std::max(rMax, lConfig.rMax);
    }
  }

  /// Adapt to the dimensions of another config
  /// it will take the maximum/minimum values and just overwrite them
  ///
  /// @param [in] lConfig is the config to which it should be adapded
  void adapt(const VolumeConfig& lConfig) {
    adaptZ(lConfig);
    adaptR(lConfig);
  }

  /// Attach method - non-const
  /// it attaches the one volume config to the other one in Z
  /// this is the non-cost method, i.e. the mid point is used
  ///
  /// @param [in] lConfig is the config to which it should be attached
  /// @note lConfig will be changed
  void midPointAttachZ(VolumeConfig& lConfig) {
    if (lConfig.zMin >= zMax) {
      double zMid = 0.5 * (lConfig.zMin + zMax);
      lConfig.zMin = zMid;
      zMax = zMid;
    } else {
      double zMid = 0.5 * (zMin + lConfig.zMax);
      lConfig.zMax = zMid;
      zMin = zMid;
    }
  }

  /// Attach method - const
  /// it attaches the one volume config to the other one
  ///
  /// @param [in] lConfig is the confit to which it should be attached
  void attachZ(const VolumeConfig& lConfig) {
    if (lConfig.zMin >= zMax) {
      zMax = lConfig.zMin;
    } else {
      zMin = lConfig.zMax;
    }
  }

  /// Overlap check radially
  ///
  /// @param [in] vConfig is the config against which is checked
  /// @return boolean if the overlap in r exists
  bool overlapsInR(const VolumeConfig& vConfig) const {
    if (!present) {
      return false;
    }
    return std::max(rMin, vConfig.rMin) <= std::min(rMax, vConfig.rMax);
  }

  /// Overlap check longitudinally
  ///
  /// @param [in] vConfig is the config against which is checked
  /// @return boolean if the overlap in z exists
  bool overlapsInZ(const VolumeConfig& vConfig) const {
    if (!present) {
      return false;
    }
    return std::max(zMin, vConfig.zMin) <= std::min(zMax, vConfig.zMax);
  }

  /// Compatibility check full set
  ///
  /// @param [in] vConfig is the config against which is checked
  /// @return boolean if the current volume wraps the vConfig fully
  bool wraps(const VolumeConfig& vConfig) const {
    if ((zMax <= vConfig.zMin) || (zMin >= vConfig.zMax)) {
      return true;
    }
    return containsInR(vConfig);
  }

  /// Check if contained full set
  ///
  /// @param [in] vConfig is the config against which is checked
  bool contains(const VolumeConfig& vConfig) const {
    return (containsInR(vConfig) && containsInZ(vConfig));
  }

  /// Check if contained radially
  ///
  /// @param [in] vConfig is the config against which is checked
  bool containsInR(const VolumeConfig& vConfig) const {
    return (rMin >= vConfig.rMax);
  }

  /// Check if contained longitudinally
  ///
  /// @param [in] vConfig is the config against which is checked
  bool containsInZ(const VolumeConfig& vConfig) const {
    return (vConfig.zMin > zMin && vConfig.zMax < zMax);
  }

  /// Method for output formatting
  std::string toString() const {
    /// for screen output
    std::stringstream sl;
    sl << rMin << ", " << rMax << " / " << zMin << ", " << zMax;
    return sl.str();
  }

  /// Conversion operator to bool
  operator bool() const { return present; }
};

/// @brief The WrappingSetup that is happening here
struct WrappingConfig {
 public:
  /// the new volumes
  VolumeConfig nVolumeConfig;
  VolumeConfig cVolumeConfig;
  VolumeConfig pVolumeConfig;

  /// the combined volume
  VolumeConfig containerVolumeConfig;

  /// existing volume config with potential gaps
  VolumeConfig existingVolumeConfig;
  VolumeConfig fGapVolumeConfig;
  VolumeConfig sGapVolumeConfig;

  /// externally provided config, this can only change the
  /// the ncp volumes
  VolumeConfig externalVolumeConfig;

  // WrappingCondition
  WrappingCondition wCondition = Undefined;
  std::string wConditionScreen = "[left untouched]";

  /// constructor
  WrappingConfig() = default;

  /// configure the new Volume
  void configureContainerVolume() {
    // set the container to be present
    containerVolumeConfig.present = true;
    std::string wConditionAddon = "";
    // if we have more than one config present
    if ((nVolumeConfig && cVolumeConfig) || (cVolumeConfig && pVolumeConfig) ||
        (nVolumeConfig && pVolumeConfig)) {
      wCondition = Wrapping;
      wConditionScreen = "grouped to ";
    }
    // adapt the new volume config to the existing configs
    if (nVolumeConfig) {
      containerVolumeConfig.adapt(nVolumeConfig);
      wConditionScreen += "[n]";
    }
    if (cVolumeConfig) {
      containerVolumeConfig.adapt(cVolumeConfig);
      wConditionScreen += "[c]";
    }
    if (pVolumeConfig) {
      containerVolumeConfig.adapt(pVolumeConfig);
      wConditionScreen += "[p]";
    }
    // adapt the external one
    if (externalVolumeConfig) {
      containerVolumeConfig.adapt(externalVolumeConfig);
    }
    // attach the volume configs
    if (nVolumeConfig && cVolumeConfig) {
      nVolumeConfig.midPointAttachZ(cVolumeConfig);
    }
    if (cVolumeConfig && pVolumeConfig) {
      cVolumeConfig.midPointAttachZ(pVolumeConfig);
    }
    // adapt r afterwards
    // - easy if no existing volume
    // - possible if no central volume
    if (!existingVolumeConfig || !cVolumeConfig) {
      nVolumeConfig.adaptR(containerVolumeConfig);
      cVolumeConfig.adaptR(containerVolumeConfig);
      pVolumeConfig.adaptR(containerVolumeConfig);
    }
  }

  /// wrap, insert, attach
  void wrapInsertAttach() {
    // action is only needed if an existing volume
    // is present
    if (existingVolumeConfig) {
      // 0 - simple attachment case
      if (!cVolumeConfig) {
        // check if it can be easily attached
        if (nVolumeConfig && nVolumeConfig.zMax < existingVolumeConfig.zMin) {
          nVolumeConfig.attachZ(existingVolumeConfig);
          // will attach the new volume(s)
          wCondition = Attaching;
          wConditionScreen = "[n attached]";
        }
        if (pVolumeConfig && pVolumeConfig.zMin > existingVolumeConfig.zMax) {
          pVolumeConfig.attachZ(existingVolumeConfig);
          // will attach the new volume(s)
          wCondition = Attaching;
          wConditionScreen = "[p attached]";
        }
        // see if inner glue volumes are needed
        if (containerVolumeConfig.rMin > existingVolumeConfig.rMin) {
          nVolumeConfig.rMin = existingVolumeConfig.rMin;
          pVolumeConfig.rMin = existingVolumeConfig.rMin;
        } else {
          fGapVolumeConfig.present = true;
          // get the zMin/zMax boundaries
          fGapVolumeConfig.adaptZ(existingVolumeConfig);
          fGapVolumeConfig.rMin = containerVolumeConfig.rMin;
          fGapVolumeConfig.rMax = existingVolumeConfig.rMin;
        }
        // see if outer glue volumes are needed
        if (containerVolumeConfig.rMax < existingVolumeConfig.rMax) {
          nVolumeConfig.rMax = existingVolumeConfig.rMax;
          pVolumeConfig.rMax = existingVolumeConfig.rMax;
        } else {
          sGapVolumeConfig.present = true;
          // get the zMin/zMax boundaries
          sGapVolumeConfig.adaptZ(existingVolumeConfig);
          sGapVolumeConfig.rMin = existingVolumeConfig.rMax;
          sGapVolumeConfig.rMax = containerVolumeConfig.rMax;
        }
      } else {
        // full wrapping or full insertion case
        if (existingVolumeConfig.rMax < containerVolumeConfig.rMin) {
          // Full wrapping case
          // - set the rMin
          nVolumeConfig.rMin = existingVolumeConfig.rMax;
          cVolumeConfig.rMin = existingVolumeConfig.rMax;
          pVolumeConfig.rMin = existingVolumeConfig.rMax;
          // - set the rMax
          nVolumeConfig.rMax = containerVolumeConfig.rMax;
          cVolumeConfig.rMax = containerVolumeConfig.rMax;
          pVolumeConfig.rMax = containerVolumeConfig.rMax;
          // will wrap the new volume(s) around existing
          wCondition = Wrapping;
          wConditionScreen = "[fully wrapped]";
        } else if (existingVolumeConfig.rMin > containerVolumeConfig.rMax) {
          // full insertion case
          // set the rMax
          nVolumeConfig.rMax = existingVolumeConfig.rMin;
          cVolumeConfig.rMax = existingVolumeConfig.rMin;
          pVolumeConfig.rMax = existingVolumeConfig.rMin;
          // set the rMin
          nVolumeConfig.rMin = containerVolumeConfig.rMin;
          cVolumeConfig.rMin = containerVolumeConfig.rMin;
          pVolumeConfig.rMin = containerVolumeConfig.rMin;
          // will insert the new volume(s) into existing
          wCondition = Inserting;
          wConditionScreen = "[fully inserted]";
        } else if (cVolumeConfig.wraps(existingVolumeConfig)) {
          // central wrapping case
          // set the rMax
          nVolumeConfig.rMax = containerVolumeConfig.rMax;
          cVolumeConfig.rMax = containerVolumeConfig.rMax;
          pVolumeConfig.rMax = containerVolumeConfig.rMax;
          // set the rMin
          nVolumeConfig.rMin = existingVolumeConfig.rMin;
          cVolumeConfig.rMin = existingVolumeConfig.rMax;
          pVolumeConfig.rMin = existingVolumeConfig.rMin;
          // set the Central Wrapping
          wCondition = CentralWrapping;
          wConditionScreen = "[centrally wrapped]";
        } else if (existingVolumeConfig.wraps(cVolumeConfig)) {
          // central insertion case
          // set the rMax
          nVolumeConfig.rMax = containerVolumeConfig.rMax;
          cVolumeConfig.rMax = existingVolumeConfig.rMin;
          pVolumeConfig.rMax = containerVolumeConfig.rMax;
          // set the rMin
          nVolumeConfig.rMin = containerVolumeConfig.rMin;
          cVolumeConfig.rMin = containerVolumeConfig.rMin;
          pVolumeConfig.rMin = containerVolumeConfig.rMin;
          // set the Central Wrapping
          wCondition = CentralWrapping;
          wConditionScreen = "[centrally inserted]";
        } else if ((existingVolumeConfig.rMax > containerVolumeConfig.rMin &&
                    existingVolumeConfig.rMin < containerVolumeConfig.rMin) ||
                   (existingVolumeConfig.rMax > containerVolumeConfig.rMax &&
                    existingVolumeConfig.rMin < containerVolumeConfig.rMax)) {
          // The volumes are overlapping this shouldn't be happening return an
          // error
          throw std::invalid_argument(
              "Volumes are overlapping, this shouldn't be happening. Please "
              "check your geometry building.");
        }

        // check if gaps are needed
        //
        // the gap reference is either the container for FULL wrapping,
        // insertion
        // or it is the centralVolume for central wrapping, insertion
        VolumeConfig referenceVolume =
            (wCondition == Wrapping || wCondition == Inserting)
                ? containerVolumeConfig
                : cVolumeConfig;
        // - at the negative sector
        if (existingVolumeConfig.zMin > referenceVolume.zMin) {
          fGapVolumeConfig.present = true;
          fGapVolumeConfig.adaptR(existingVolumeConfig);
          fGapVolumeConfig.zMin = referenceVolume.zMin;
          fGapVolumeConfig.zMax = existingVolumeConfig.zMin;
        } else {
          // adapt lower z boundary
          if (nVolumeConfig) {
            nVolumeConfig.zMin = existingVolumeConfig.zMin;
          } else if (cVolumeConfig) {
            cVolumeConfig.zMin = existingVolumeConfig.zMin;
          }
        }
        // - at the positive sector
        if (existingVolumeConfig.zMax < referenceVolume.zMax) {
          sGapVolumeConfig.present = true;
          sGapVolumeConfig.adaptR(existingVolumeConfig);
          sGapVolumeConfig.zMin = existingVolumeConfig.zMax;
          sGapVolumeConfig.zMax = referenceVolume.zMax;
        } else {
          // adapt higher z boundary
          if (pVolumeConfig) {
            pVolumeConfig.zMax = existingVolumeConfig.zMax;
          } else if (cVolumeConfig) {
            cVolumeConfig.zMax = existingVolumeConfig.zMax;
          }
        }
      }
    }
    return;
  }

  /// Method for output formatting
  std::string toString() const {
    // for screen output
    std::stringstream sl;
    if (containerVolumeConfig) {
      sl << "New container built with       configuration: "
         << containerVolumeConfig.toString() << '\n';
    }
    // go through the new ones first
    if (nVolumeConfig) {
      sl << " - n: Negative Endcap, current configuration: "
         << nVolumeConfig.toString() << '\n';
    }
    if (cVolumeConfig) {
      sl << " - c: Barrel, current          configuration: "
         << cVolumeConfig.toString() << '\n';
    }
    if (pVolumeConfig) {
      sl << " - p: Negative Endcap, current configuration: "
         << pVolumeConfig.toString() << '\n';
    }
    if (existingVolumeConfig) {
      sl << "Existing volume with           configuration: "
         << existingVolumeConfig.toString() << '\n';
      if (fGapVolumeConfig) {
        sl << " - g1: First gap volume,       configuration : "
           << fGapVolumeConfig.toString() << '\n';
      }
      if (sGapVolumeConfig) {
        sl << " - g2: Second gap volume,      configuration : "
           << sGapVolumeConfig.toString() << '\n';
      }
      if (wCondition != Undefined) {
        sl << "WrappingCondition = " << wCondition << '\n';
      }
    }
    return sl.str();
  }
};

/// @class CylinderVolumeBuilder
///
/// A volume builder to be used for building a concentrical cylindrical volumes
///  - a) configured volume
///  - b) wrapping around a cylindrical/disk layer config
///
/// All are optionally wrapped around a given volume which has to by a cylinder
/// volume and which has to be center at z == 0
///
/// To receive the tracking volume it is possible to also hand over a triple of
/// layers, which is a C++ tuple of three pointers to layer vectors (defined in
/// the ITrackingVolumeBuilder). This functionality is needed for a possible
/// translation of an geometry existing in another format. The first entry
/// represents the layers of the negative endcap, the second the layers of the
/// barrel and the third the layers of the positive endcap. If the one of these
/// pointers is a nullptr no layers will be created for this volume
///
/// For the endcap region it is possible to check for a ring layout,
/// in which case an attempt to split into individual ring volumes is done
class CylinderVolumeBuilder : public ITrackingVolumeBuilder {
 public:
  /// @struct Config
  /// Nested configuration struct for this CylinderVolumeBuilder
  struct Config {
    /// The tracking volume helper for construction
    std::shared_ptr<const ITrackingVolumeHelper> trackingVolumeHelper = nullptr;
    /// The string based identification
    std::string volumeName = "";
    /// The world material
    std::shared_ptr<const IVolumeMaterial> volumeMaterial = nullptr;
    /// Build the volume to the beam line
    bool buildToRadiusZero = false;
    /// Check for endcap ring layout
    bool checkRingLayout = false;
    /// Tolerance for endcap ring association
    double ringTolerance = 0 * UnitConstants::mm;
    /// Builder to construct layers within the volume
    std::shared_ptr<const ILayerBuilder> layerBuilder = nullptr;
    /// Builder to construct confined volumes within the volume
    std::shared_ptr<const IConfinedTrackingVolumeBuilder> ctVolumeBuilder =
        nullptr;
    /// Additional envelope in R to create rMin, rMax
    std::pair<double, double> layerEnvelopeR = {1. * UnitConstants::mm,
                                                1. * UnitConstants::mm};
    /// the additional envelope in Z to create zMin, zMax
    double layerEnvelopeZ = 1. * UnitConstants::mm;

    // The potential boundary material (MB) options - there are 6 at maximum
    /// -------------------- MB (outer [1]) ---------------
    /// | MB [2]  NEC  MB [3] |  B |  MB [4]  PEC  MB [5] |
    /// -------------------- MB (inner [0]) ---------------
    std::array<std::shared_ptr<const ISurfaceMaterial>, 6> boundaryMaterial{
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
  };

  /// Constructor
  ///
  /// @param [in] cvbConfig is the configuration struct to steer the builder
  /// @param [in] logger logging instance
  CylinderVolumeBuilder(const Config& cvbConfig,
                        std::unique_ptr<const Logger> logger = getDefaultLogger(
                            "CylinderVolumeBuilder", Logging::INFO));

  /// Destructor
  ~CylinderVolumeBuilder() override;

  /// CylinderVolumeBuilder main call method
  ///
  /// @param [in] gctx geometry context for which this cylinder volume is built
  /// @param [in] existingVolume is an (optional) volume to be included
  /// @param [in] externalBounds are (optional) external confinement
  ///             constraints
  /// @return a mutable pointer to a new TrackingVolume which includes the
  ///         optionally provided existingVolume consistently for further
  ///         processing
  MutableTrackingVolumePtr trackingVolume(
      const GeometryContext& gctx, TrackingVolumePtr existingVolume = nullptr,
      std::shared_ptr<const VolumeBounds> externalBounds =
          nullptr) const override;

  /// Set configuration method
  ///
  /// @param [in] cvbConfig is the new configuration to be set
  void setConfiguration(const Config& cvbConfig);

  /// Get configuration method
  ///
  /// @return a copy of the config object
  Config getConfiguration() const;

  /// set logging instance
  ///
  /// @param [in] newLogger is the logging instance to be set
  void setLogger(std::unique_ptr<const Logger> newLogger);

  /// Analyze the config to gather needed dimension
  ///
  /// @param [in] gctx the geometry context for this building
  /// @param [in] lVector is the vector of layers that are parsed
  /// @param [in] mtvVector Vector of mutable tracking volumes to analyze
  ///
  /// @return a VolumeConfig representing this layer
  VolumeConfig analyzeContent(
      const GeometryContext& gctx, const LayerVector& lVector,
      const MutableTrackingVolumeVector& mtvVector) const;

 private:
  /// Configuration struct
  Config m_cfg;

  /// Private access to the logger
  ///
  /// @return a const reference to the logger
  const Logger& logger() const { return *m_logger; }

  /// the logging instance
  std::unique_ptr<const Logger> m_logger;

  /// Helper method check the layer containment,
  /// both for inside / outside.
  ///
  /// @param [in] gctx the geometry context for this building
  /// @param [in] layerConfig is the VolumeConfig to be tested
  ///        the wrapping flag may be set
  /// @param [in] insideConfig is the inside volume in order to
  ///        check the wrapping
  /// @param [in] volumeConfig is the volume to be tested
  /// @param [in] sign distinguishes inside/outside testing
  ///
  /// @return boolean that indicates the test result
  bool checkLayerContainment(const GeometryContext& gctx,
                             VolumeConfig& layerConfig,
                             const VolumeConfig& insideConfig,
                             const VolumeConfig& volumeConfig, int sign) const;
};

/// Return the configuration object
inline CylinderVolumeBuilder::Config CylinderVolumeBuilder::getConfiguration()
    const {
  return m_cfg;
}

}  // namespace Acts
