// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/MaterialMapping/IMaterialWriter.hpp"
#include <Acts/Definitions/Algebra.hpp>
#include <Acts/Geometry/GeometryIdentifier.hpp>
#include <Acts/Geometry/TrackingGeometry.hpp>
#include <Acts/Geometry/TrackingVolume.hpp>
#include <Acts/Material/IMaterialDecorator.hpp>
#include <Acts/Material/ISurfaceMaterial.hpp>
#include <Acts/Material/IVolumeMaterial.hpp>
#include <Acts/Surfaces/Surface.hpp>
#include <Acts/Utilities/Logger.hpp>

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

class TFile;

namespace Acts {
class ISurfaceMaterial;
class IVolumeMaterial;
class Layer;
class TrackingGeometry;
class TrackingVolume;

using SurfaceMaterialMap =
    std::map<GeometryIdentifier, std::shared_ptr<const ISurfaceMaterial>>;
using VolumeMaterialMap =
    std::map<GeometryIdentifier, std::shared_ptr<const IVolumeMaterial>>;
using DetectorMaterialMaps = std::pair<SurfaceMaterialMap, VolumeMaterialMap>;
}  // namespace Acts

namespace ActsExamples {

/// @brief Material decorator from Root format
///
/// This reads in material maps for surfaces and volumes
/// from a root file
class RootMaterialWriter : public IMaterialWriter {
 public:
  /// @class Config
  ///
  /// Configuration of the Writer
  struct Config {
    /// Steering to handle sensitive data
    bool processSensitives = true;

    /// Steering to handle approach data
    bool processApproaches = true;

    /// Steering to handle representing data
    bool processRepresenting = true;

    /// Steering to handle boundary data
    bool processBoundaries = true;

    /// Steering to handle volume data
    bool processVolumes = true;

    /// The name of the output surface tree
    std::string folderSurfaceNameBase = "SurfaceMaterial";
    /// The name of the output volume tree
    std::string folderVolumeNameBase = "VolumeMaterial";
    /// The volume identification string
    std::string voltag = "_vol";
    /// The boundary identification string
    std::string boutag = "_bou";
    /// The layer identification string
    std::string laytag = "_lay";
    /// The approach identification string
    std::string apptag = "_app";
    /// The sensitive identification string
    std::string sentag = "_sen";
    /// The bin number tag
    std::string ntag = "n";
    /// The value tag -> binning values: binZ, binR, binPhi, etc.
    std::string vtag = "v";
    /// The option tag -> binning options: open, closed
    std::string otag = "o";
    /// The range min tag: min value
    std::string mintag = "min";
    /// The range max tag: max value
    std::string maxtag = "max";
    /// The thickness tag
    std::string ttag = "t";
    /// The x0 tag
    std::string x0tag = "x0";
    /// The l0 tag
    std::string l0tag = "l0";
    /// The A tag
    std::string atag = "A";
    /// The Z tag
    std::string ztag = "Z";
    /// The rho tag
    std::string rhotag = "rho";
    /// The name of the output file
    std::string filePath = "material-maps.root";
    /// The file mode
    std::string fileMode = "RECREATE";
  };

  /// Constructor
  ///
  /// @param config The configuration struct
  /// @param level The log level
  RootMaterialWriter(const Config& config, Acts::Logging::Level level);

  /// Virtual destructor
  ~RootMaterialWriter() override;

  /// Write out the material map
  ///
  /// @param detMaterial is the SurfaceMaterial and VolumeMaterial maps
  void writeMaterial(const Acts::DetectorMaterialMaps& detMaterial) override;

  /// Write out the material map from Geometry
  ///
  /// @param tGeometry is the TrackingGeometry
  void write(const Acts::TrackingGeometry& tGeometry);

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 private:
  /// Collect the material from the tracking geometry
  ///
  /// @param tVolume The TrackingVolume for the material to be collected
  /// @param [in,out] detMatMap the map to be filled
  void collectMaterial(const Acts::TrackingVolume& tVolume,
                       Acts::DetectorMaterialMaps& detMatMap);

  /// Collect the material from the tracking geometry
  ///
  /// @param tLayer The TrackingVolume for the material to be collected
  /// @param [in,out] detMatMap the map to be filled
  void collectMaterial(const Acts::Layer& tLayer,
                       Acts::DetectorMaterialMaps& detMatMap);

  /// The config class
  Config m_cfg;

  /// The logger instance
  std::unique_ptr<const Acts::Logger> m_logger;

  /// Private access to the logging instance
  const Acts::Logger& logger() const { return *m_logger; }

  TFile* m_outputFile{nullptr};
};

}  // namespace ActsExamples
