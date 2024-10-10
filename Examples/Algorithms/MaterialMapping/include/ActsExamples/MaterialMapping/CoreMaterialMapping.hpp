// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Material/MaterialMapper.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/MaterialMapping/IMaterialWriter.hpp"

namespace ActsExamples {

/// @class CoreMaterialMapping
///
/// @brief Initiates and executes material mapping using the MaterialMapper
/// from the core component of ACTS
///
/// By construction, the material mapping needs inter-event information
/// to build the material maps of accumulated single particle views.
/// However, running it in one single event, puts enormous pressure onto
/// the I/O structure.
///
/// It therefore saves the mapping state/cache as a private member variable
/// and is designed to be executed in a single threaded mode.
class CoreMaterialMapping : public IAlgorithm {
 public:
  /// @class nested Config class
  /// of the MaterialMapping algorithm
  struct Config {
    /// Input collection
    std::string inputMaterialTracks = "material_tracks";

    /// The actually mapped material tracks
    std::string mappedMaterialTracks = "mapped_material_tracks";

    /// Theunmapped part of the material tracks
    std::string unmappedMaterialTracks = "unmapped_material_tracks";

    /// The ACTS material mapper from the core component
    std::shared_ptr<Acts::MaterialMapper> materialMapper = nullptr;

    /// The writer of the material
    std::vector<std::shared_ptr<IMaterialWriter>> materiaMaplWriters{};
  };

  /// Constructor
  ///
  /// @param cfg The configuration struct carrying the used tools
  /// @param level The output logging level
  CoreMaterialMapping(const Config& cfg,
                      Acts::Logging::Level level = Acts::Logging::INFO);

  /// Destructor
  /// - it also writes out the file
  ~CoreMaterialMapping() override;

  /// Framework execute method
  ///
  /// @param context The algorithm context for event consistency
  ActsExamples::ProcessCode execute(
      const AlgorithmContext& context) const override;

  /// Readonly access to the config
  const Config& config() const { return m_cfg; }

 private:
  Config m_cfg;  //!< internal config object

  std::unique_ptr<Acts::MaterialMapper::State> m_mappingState{nullptr};

  ReadDataHandle<std::unordered_map<std::size_t, Acts::RecordedMaterialTrack>>
      m_inputMaterialTracks{this, "InputMaterialTracks"};

  WriteDataHandle<std::unordered_map<std::size_t, Acts::RecordedMaterialTrack>>
      m_outputMappedMaterialTracks{this, "OutputMappedMaterialTracks"};

  WriteDataHandle<std::unordered_map<std::size_t, Acts::RecordedMaterialTrack>>
      m_outputUnmappedMaterialTracks{this, "OutputUnmappedMaterialTracks"};
};

}  // namespace ActsExamples
