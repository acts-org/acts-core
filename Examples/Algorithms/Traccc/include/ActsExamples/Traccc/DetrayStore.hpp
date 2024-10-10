// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Plugins/Detray/DetrayConversionUtils.hpp"
#include "Acts/Plugins/Detray/DetrayConverter.hpp"

#include <memory>

#include <vecmem/memory/host_memory_resource.hpp>
#include <vecmem/memory/memory_resource.hpp>

namespace ActsExamples {

// The Detray host store that is used to store the detector
// and the associated memory resource
template <typename memory_source_t>
struct DetrayStore {
  // Constructor from arguments
  DetrayStore(std::shared_ptr<memory_source_t> mSource,
              Acts::DetrayHostDetector&& det)
      : memoryResource(std::move(mSource)), detector(std::move(det)) {}

  // The memory resource
  std::shared_ptr<memory_source_t> memoryResource = nullptr;
  // The detray detector instance
  Acts::DetrayHostDetector detector;

  // Create a Detray detector and store it with its memory Source in
  ///
  /// @param gctx the geometry context
  /// @param detector the detector to be converted
  /// @param options the conversion options
  static inline std::shared_ptr<const DetrayStore<memory_source_t>> create(
      const Acts::GeometryContext& gctx,
      const Acts::Experimental::Detector& detector,
      const Acts::DetrayConverter::Options& options) {
    auto memoryResource = std::make_shared<memory_source_t>();
    auto DetrayHostDetector = Acts::DetrayConverter().convert<>(
        gctx, detector, *memoryResource, options);

    return std::make_shared<DetrayStore<memory_source_t>>(
        memoryResource, std::move(DetrayHostDetector));
  }
};

using DetrayHostStore = DetrayStore<vecmem::host_memory_resource>;

}  // namespace ActsExamples
