// This file is part of the Acts project.
//
// Copyright (C) 2020-2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

// Local include(s).
#include "Acts/Plugins/Sycl/Seeding/detail/Types.hpp"

#include "../Utilities/Arrays.hpp"
#include "SpacePointType.hpp"

// VecMem include(s).
#include "vecmem/containers/data/jagged_vector_view.hpp"
#include "vecmem/containers/data/vector_view.hpp"
#include "vecmem/containers/jagged_device_vector.hpp"
#include "vecmem/containers/device_vector.hpp"

// SYCL include(s).
#include <CL/sycl.hpp>

// System include(s).
#include <cstdint>

namespace Acts::Sycl::detail {

/// Functor taking care of finding viable spacepoint duplets
template <SpacePointType OtherSPType>
class DupletSearch {
  // Sanity check(s).
  static_assert((OtherSPType == SpacePointType::Bottom) ||
                    (OtherSPType == SpacePointType::Top),
                "Class must be instantiated with either "
                "Acts::Sycl::detail::SpacePointType::Bottom or "
                "Acts::Sycl::detail::SpacePointType::Top");

 public:
  /// Constructor with all the necessary arguments
  DupletSearch(vecmem::data::vector_view<const DeviceSpacePoint> middleSPs,
               vecmem::data::vector_view<const DeviceSpacePoint> otherSPs,
               vecmem::data::jagged_vector_view<uint32_t> middleOtherSPIndices,
               const DeviceSeedfinderConfig& config)
      : m_middleSPs(middleSPs),
        m_otherSPs(otherSPs),
        m_middleOtherSPIndices(middleOtherSPIndices),
        m_config(config) {}

  /// Operator performing the duplet search
  void operator()(cl::sycl::nd_item<2> item) const {
    // Get the indices of the spacepoints to evaluate.
    const auto middleIndex = item.get_global_id(0);
    const auto otherIndex = item.get_global_id(1);

    // We check whether this thread actually makes sense (within bounds).
    // The number of threads is usually a factor of 2, or 3*2^k (k \in N), etc.
    // Without this check we may index out of arrays.
    if ((middleIndex >= m_middleSPs.size()) ||
        (otherIndex >= m_otherSPs.size())) {
      return;
    }

    // Create a copy of the spacepoint objects for the current thread. On
    // dedicated GPUs this provides a better performance than accessing
    // variables one-by-one from global device memory.
    const vecmem::device_vector<const DeviceSpacePoint> middleSPs(m_middleSPs);
    const DeviceSpacePoint middleSP = middleSPs[middleIndex];
    const vecmem::device_vector<const DeviceSpacePoint> otherSPs(m_otherSPs);
    const DeviceSpacePoint otherSP = otherSPs[otherIndex];

    // Calculate the variables that the duplet quality selection are based on.
    // Note that the asserts of the functor make sure that 'OtherSPType' must be
    // either SpacePointType::Bottom or SpacePointType::Top.
    float deltaR = 0.0f, cotTheta = 0.0f;
    if constexpr (OtherSPType == SpacePointType::Bottom) {
      deltaR = middleSP.r - otherSP.r;
      cotTheta = (middleSP.z - otherSP.z) / deltaR;
    } else {
      deltaR = otherSP.r - middleSP.r;
      cotTheta = (otherSP.z - middleSP.z) / deltaR;
    }
    const float zOrigin = middleSP.z - middleSP.r * cotTheta;

    // Check if the duplet passes our quality requirements.
    if ((deltaR >= m_config.deltaRMin) && (deltaR <= m_config.deltaRMax) &&
        (cl::sycl::abs(cotTheta) <= m_config.cotThetaMax) &&
        (zOrigin >= m_config.collisionRegionMin) &&
        (zOrigin <= m_config.collisionRegionMax)) {
      // Create device vector based on the view and push to it
      vecmem::jagged_device_vector<uint32_t>
          middleOtherSPIndices(m_middleOtherSPIndices);
      middleOtherSPIndices.at(middleIndex).push_back(otherIndex);
    }
  }

 private:
  /// Middle spacepoints
  vecmem::data::vector_view<const DeviceSpacePoint> m_middleSPs;
  /// "Other" (bottom or top) spacepoints
  vecmem::data::vector_view<const DeviceSpacePoint> m_otherSPs;

  /// The 2D array storing the compatible middle-other spacepoint indices
  vecmem::data::jagged_vector_view<uint32_t> m_middleOtherSPIndices;

  /// Configuration for the seed finding
  DeviceSeedfinderConfig m_config;

};  // struct DupletSearch

}  // namespace Acts::Sycl::detail
