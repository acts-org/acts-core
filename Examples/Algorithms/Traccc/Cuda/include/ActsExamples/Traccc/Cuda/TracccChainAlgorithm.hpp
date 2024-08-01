// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

// Traccc plugin include(s)
#include "ActsExamples/Traccc/Common/TracccChainAlgorithmBase.hpp"
#include "ActsExamples/Traccc/Cuda/Types.hpp"
#include "traccc/cuda/utils/stream.hpp"
#include "traccc/utils/algorithm.hpp"

// VecMem include(s).
#include <vecmem/containers/vector.hpp>
#include <vecmem/memory/binary_page_memory_resource.hpp>
#include <vecmem/memory/cuda/device_memory_resource.hpp>
#include <vecmem/memory/memory_resource.hpp>
#include <vecmem/utils/cuda/async_copy.hpp>

namespace ActsExamples::Traccc::Cuda {


class TracccChainAlgorithm final : public Common::TracccChainAlgorithmBase {
public:

/// Construct the traccc algorithm.
///
/// @param cfg is the algorithm configuration
/// @param lvl is the logging level
TracccChainAlgorithm(Config cfg, Acts::Logging::Level lvl);

/// Run the algorithm.
///
/// @param ctx is the algorithm context with event information
/// @return a process code indication success or failure
ProcessCode execute(const AlgorithmContext& ctx) const override;

private:
    using HostTypes = typename ActsExamples::Chain::Cuda::Types<typename FieldType::view_t>;
    using CudaTypes = typename ActsExamples::Chain::Cuda::Types<typename FieldType::view_t>;

    typename CudaTypes::ClusterizationAlgorithmType clusterizationAlgorithm;
    typename CudaTypes::SpacepointFormationAlgorithmType spacepointFormationAlgorithm;
    typename CudaTypes::SeedingAlgorithmType seedingAlgorithm;
    typename CudaTypes::TrackParametersEstimationAlgorithmType trackParametersEstimationAlgorithm;
    typename CudaTypes::FindingAlgorithmType findingAlgorithm;
    typename CudaTypes::FittingAlgorithmType fittingAlgorithm;
    typename HostTypes::AmbiguityResolutionAlgorithmType ambiguityResolutionAlgorithm;

    const unsigned short targetCellsPerPartition = 10; // Which value?

    /// CUDA stream to use
    traccc::cuda::stream stream;
    /// Device memory resource
    vecmem::cuda::device_memory_resource deviceMemoryResource;
    /// Device caching memory resource
    vecmem::binary_page_memory_resource cachedDeviceMemoryResource(deviceMemoryResource);
    /// (Asynchronous) Memory copy object
    mutable vecmem::cuda::async_copy asyncCopy(stream.cudaStream());

    traccc::cuda::measurement_sorting_algorithm measurementSorting(asyncCopy, stream);

    /// Buffer holding the detector's payload on the device
    typename DectectorHostType::buffer_type deviceDetector;
    /// View of the detector's payload on the device
    typename DectectorHostType::view_type deviceDetectorView;
};

}  // namespace ActsExamples::Traccc::Host