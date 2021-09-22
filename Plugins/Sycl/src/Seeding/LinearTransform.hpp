// This file is part of the Acts project.
//
// Copyright (C) 2020-2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

// SYCL plugin include(s).
#include "Acts/Plugins/Sycl/Seeding/detail/Types.hpp"

#include "../Utilities/Arrays.hpp"
#include "SpacePointType.hpp"

// SYCL include(s).
#include <CL/sycl.hpp>

// System include(s).
#include <cassert>
#include <cstdint>

// VecMem include(s).
#include "vecmem/containers/data/vector_view.hpp"
#include "vecmem/containers/device_vector.hpp"

namespace Acts::Sycl::detail {

/// Functor performing a linear coordinate transformation on spacepoint pairs
template <SpacePointType OtherSPType>
class LinearTransform {
  // Sanity check(s).
  static_assert((OtherSPType == SpacePointType::Bottom) ||
                    (OtherSPType == SpacePointType::Top),
                "Class must be instantiated with either "
                "Acts::Sycl::detail::SpacePointType::Bottom or "
                "Acts::Sycl::detail::SpacePointType::Top");

 public:
  /// Constructor with all the necessary arguments
  LinearTransform(vecmem::data::vector_view<const DeviceSpacePoint> middleSPs,
                  vecmem::data::vector_view<const DeviceSpacePoint> otherSPs,
                  vecmem::data::vector_view<uint32_t> middleIndexLUT,
                  vecmem::data::vector_view<uint32_t> otherIndexLUT, uint32_t nEdges,
                  vecmem::data::vector_view<detail::DeviceLinEqCircle> resultArray)
      : m_middleSPs(middleSPs),
        m_otherSPs(otherSPs),
        m_middleIndexLUT(middleIndexLUT),
        m_otherIndexLUT(otherIndexLUT),
        m_nEdges(nEdges),
        m_resultArray(resultArray) {}

  /// Operator performing the coordinate linear transformation
  void operator()(cl::sycl::nd_item<1> item) const {
    // Get the index to operate on.
    const auto idx = item.get_global_linear_id();
    if (idx >= m_nEdges) {
      return;
    }

    // Translate this one index into indices in the spacepoint arrays.
    // Note that using asserts with the CUDA backend of dpc++ is not working
    // quite correctly at the moment. :-( So these checks may need to be
    // disabled if you need to build for an NVidia backend in Debug mode.
    vecmem::device_vector<uint32_t> 
                   middleIndexLUT(m_middleIndexLUT);
    const uint32_t middleIndex = middleIndexLUT[idx];
    assert(middleIndex < m_middleSPs.size());
    (void)m_middleSPs.size();
    vecmem::device_vector<uint32_t> 
                  otherIndexLUT(m_otherIndexLUT);
    const uint32_t otherIndex = otherIndexLUT[idx];
    assert(otherIndex < m_otherSPs.size());
    (void)m_otherSPs.size();

    // Create a copy of the spacepoint objects for the current thread. On
    // dedicated GPUs this provides a better performance than accessing
    // variables one-by-one from global device memory.
    const vecmem::device_vector<const DeviceSpacePoint> middleSPs(m_middleSPs);
    const DeviceSpacePoint middleSP = middleSPs[middleIndex];
    const vecmem::device_vector<const DeviceSpacePoint> otherSPs(m_otherSPs);
    const DeviceSpacePoint otherSP = otherSPs[otherIndex];

    // Calculate some "helper variables" for the coordinate linear
    // transformation.
    const float cosPhiM = middleSP.x / middleSP.r;
    const float sinPhiM = middleSP.y / middleSP.r;

    const float deltaX = otherSP.x - middleSP.x;
    const float deltaY = otherSP.y - middleSP.y;
    const float deltaZ = otherSP.z - middleSP.z;

    const float x = deltaX * cosPhiM + deltaY * sinPhiM;
    const float y = deltaY * cosPhiM - deltaX * sinPhiM;
    const float iDeltaR2 = 1.f / (deltaX * deltaX + deltaY * deltaY);

    // Create the result object.
    DeviceLinEqCircle result;
    result.iDeltaR = cl::sycl::sqrt(iDeltaR2);
    result.cotTheta = deltaZ * result.iDeltaR;
    if constexpr (OtherSPType == SpacePointType::Bottom) {
      result.cotTheta = -(result.cotTheta);
    }
    result.zo = middleSP.z - middleSP.r * result.cotTheta;
    result.u = x * iDeltaR2;
    result.v = y * iDeltaR2;
    result.er =
        ((middleSP.varZ + otherSP.varZ) +
         (result.cotTheta * result.cotTheta) * (middleSP.varR + otherSP.varR)) *
        iDeltaR2;

    // Store the result in the result vector
    vecmem::device_vector<detail::DeviceLinEqCircle>
                          resultArray(m_resultArray);
    resultArray[idx] = result;
    return;
  }

 private:
  /// Pointer to the middle spacepoints (in global device memory)
  vecmem::data::vector_view<const DeviceSpacePoint> m_middleSPs;
  /// Pointer to the "other" (bottom or top) spacepoints (in global device mem.)
  vecmem::data::vector_view<const DeviceSpacePoint> m_otherSPs;

  /// Look-Up Table from the iteration index to the middle spacepoint index
  vecmem::data::vector_view<uint32_t> m_middleIndexLUT;
  /// Loop-Up Table from the iteration index to the "other" spacepoint index
  vecmem::data::vector_view<uint32_t> m_otherIndexLUT;

  /// Total number of elements in the result array
  uint32_t m_nEdges;

  /// The result array in device global memory
  vecmem::data::vector_view<detail::DeviceLinEqCircle> m_resultArray;

};  // class LinearTransform

}  // namespace Acts::Sycl::detail
