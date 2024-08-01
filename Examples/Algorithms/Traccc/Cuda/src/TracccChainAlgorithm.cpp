// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// Acts examples include(s)
#include "ActsExamples/Traccc/Cuda/TracccChainAlgorithm.hpp"

// System include(s).
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <type_traits>
#include <tuple>

#include <stdexcept>

ActsExamples::Traccc::Host::TracccChainAlgorithm::TracccChainAlgorithm(
    Config cfg, Acts::Logging::Level lvl)
    : ActsExamples::Traccc::Common::TracccChainAlgorithmBase(std::move(cfg), std::move(lvl)),
    clusterizationAlgorithm(vecmem::memory_resource{cachedDeviceMemoryResource, &hostMemoryResource}, asyncCopy, stream, targetCellsPerPartition),
    spacepointFormationAlgorithm(vecmem::memory_resource{cachedDeviceMemoryResource, &hostMemoryResource}, asyncCopy, stream),
    seedingAlgorithm(m_cfg.chainConfig->seedfinderConfig, m_cfg.chainConfig->spacepointGridConfig, m_cfg.chainConfig->seedfilterConfig, vecmem::memory_resource{cachedDeviceMemoryResource, &hostMemoryResource}, asyncCopy, stream),
    trackParametersEstimationAlgorithm(vecmem::memory_resource{cachedDeviceMemoryResource, &hostMemoryResource}, asyncCopy, stream),
    findingAlgorithm(m_cfg.chainConfig->findingConfig, vecmem::memory_resource{cachedDeviceMemoryResource, &hostMemoryResource}, asyncCopy, stream),
    fittingAlgorithm(m_cfg.chainConfig->fittingConfig, vecmem::memory_resource{cachedDeviceMemoryResource, &hostMemoryResource}, asyncCopy, stream),
    ambiguityResolutionAlgorithm(m_cfg.chainConfig->ambiguityResolutionConfig)
{}

ActsExamples::ProcessCode ActsExamples::Traccc::Cuda::TracccChainAlgorithm::execute(
const ActsExamples::AlgorithmContext& ctx) const {

  vecmem::host_memory_resource mr;

  typename HostTypes::ClusterizationAlgorithmType::output_type measurements{&mr};
  typename HostTypes::SpacepointFormationAlgorithmType::output_type spacepoints{&mr};
  typename HostTypes::SeedingAlgorithmType::output_type seeds{&mr};
  typename HostTypes::TrackParametersEstimationAlgorithmType::output_type params{&mr};
  typename HostTypes::FindingAlgorithmType::output_type trackCandidates{&mr};
  typename HostTypes::FittingAlgorithmType::output_type trackStates{&mr};
  typename HostTypes::AmbiguityResolutionAlgorithmType::output_type resolvedTrackStates{&mr};

  const auto cellsMap = m_inputCells(ctx);

  auto [cells, modules] = converter.convertCells(cellsMap, &mr);

  // Create device copy of input collections
  cell_collection_types::buffer cellsBuffer(cells.size(),
                                              cachedDeviceMemoryResource);
  asyncCopy(vecmem::get_data(cells), cellsBuffer)->ignore();
  cell_module_collection_types::buffer modulesBuffer(modules.size(), cachedDeviceMemoryResource);
  asyncCopy(vecmem::get_data(modules), modulesBuffer)->ignore();

  // Run the clusterization (asynchronously).
  measurements = clusterizationAlgorithm(cellsBuffer, modulesBuffer);
  measurementSorting(measurements);

  // Run the seed-finding (asynchronously).
  spacepoints = spacepointFormationAlgorithm(measurements, modulesBuffer);

  seeds = seedingAlgorithm(spacepoints);

  const typename FieldType::view_t fieldView(field);
  static_assert(std::is_same<FieldType, typename detray::bfield::const_field_t>::value, "Currently, traccc expects a constant field.");
  params = trackParameterEstimationAlgorithm(spacepoints, seeds, fieldView.at(0.f,0.f,0.f));

  // Create the buffer needed by track finding and fitting.
  auto navigationBuffer = detray::create_candidates_buffer(
      hostDetector,
      m_cfg.chainConfig.findingConfig.navigation_buffer_size_scaler *
          asyncCopy.get_size(params),
      cachedDeviceMemoryResource, &hostMemoryResource);

  // Run the track finding (asynchronously).
  trackCandidates = findingAlgorithm(deviceDetectorView, field, navigationBuffer, measurements, params);

  // Run the track fitting (asynchronously).
  trackStates = fittingAlgorithm(deviceDetectorView, field, navigationBuffer, trackCandidates);

  // Copy a limited amount of result data back to the host.
  output_type trackStatesHost{&hostMemoryResource};
  asyncCopy(track_states.headers, trackStates)->wait();

  const auto actsMeasurements = m_inputMeasurements(ctx);
  auto result = converter.convertTracks(resolvedTrackStates, measurements, actsMeasurements); 

  m_outputTracks(ctx, std::move(result));
  return ActsExamples::ProcessCode::SUCCESS;
}
