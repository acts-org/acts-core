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
using AtomicAccessorFilter =
    sycl::accessor<uint32_t, 1, sycl::access::mode::read>;
/// Functor performing Triplet Filter
template<class AtomicAccessorType>
class TripletFilter {

    public:
    /// Constructor
    TripletFilter(const uint32_t numTripletFilterThreads,
                  vecmem::data::vector_view<uint32_t> sumBotMidView,
                  const uint32_t firstMiddle,
                  vecmem::data::vector_view<uint32_t> indMidBotCompView,
                  vecmem::data::vector_view<uint32_t> indBotDupletView,
                  vecmem::data::vector_view<uint32_t> sumBotTopCombView,
                  vecmem::data::jagged_vector_view<uint32_t> midTopDupletView,
                  vecmem::data::vector_view<detail::DeviceTriplet> curvImpactView,
                  vecmem::data::vector_view<const detail::DeviceSpacePoint> topSPsView,
                  vecmem::data::vector_view<const detail::DeviceSpacePoint> middleSPsView,
                  vecmem::data::vector_view<const detail::DeviceSpacePoint> bottomSPsView,
                  const AtomicAccessorFilter& countTripletsAcc,
                  vecmem::data::vector_view<detail::SeedData> seedArrayView,
                  const DeviceSeedfinderConfig& config,
                  const DeviceExperimentCuts& cuts)
        : m_numTripletFilterThreads(numTripletFilterThreads),
          m_sumBotMidView(sumBotMidView),
          m_firstMiddle(firstMiddle),
          m_indMidBotCompView(indMidBotCompView),
          m_indBotDupletView(indBotDupletView),
          m_sumBotTopCombView(sumBotTopCombView),
          m_midTopDupletView(midTopDupletView),
          m_curvImpactView(curvImpactView),
          m_topSPsView(topSPsView),
          m_middleSPsView(middleSPsView),
          m_bottomSPsView(bottomSPsView),
          m_countTripletsAcc(countTripletsAcc),
          m_seedArrayView(seedArrayView),
          m_config(config),
          m_cuts(cuts) {}

    /// Operator performing filtering 
    void operator()(cl::sycl::nd_item<1> item) const {
        if (item.get_global_linear_id() < m_numTripletFilterThreads) {
            vecmem::device_vector<uint32_t>
                    sumBotMidPrefix(m_sumBotMidView),
                    deviceIndMidBot(m_indMidBotCompView),
                    deviceIndBotDuplets(m_indBotDupletView),
                    sumBotTopCombPrefix(m_sumBotTopCombView);
            vecmem::jagged_device_vector<uint32_t>
                    midTopDuplets(m_midTopDupletView);
            const auto idx = sumBotMidPrefix[m_firstMiddle] +
                                item.get_global_linear_id();
            const auto mid = deviceIndMidBot[idx];
            const auto bot = deviceIndBotDuplets[idx];
            const auto sumCombUptoFirstMiddle = 
                sumBotTopCombPrefix[m_firstMiddle];
            const auto tripletBegin =
                sumBotTopCombPrefix[mid] - sumCombUptoFirstMiddle +
                (idx - sumBotMidPrefix[mid]) * midTopDuplets.at(mid).size();
            const auto tripletEnd =
                tripletBegin + m_countTripletsAcc[idx];
            const vecmem::device_vector
                    <detail::DeviceTriplet>
                        deviceCurvImpactConst(m_curvImpactView);
            for (auto i1 = tripletBegin; i1 < tripletEnd; ++i1) {
                const auto current = deviceCurvImpactConst[i1];
                const auto top = current.topSPIndex;

                const auto invHelixDiameter = current.curvature;
                const auto lowerLimitCurv =
                    invHelixDiameter -
                    m_config.deltaInvHelixDiameter;
                const auto upperLimitCurv =
                    invHelixDiameter +
                    m_config.deltaInvHelixDiameter;
                const vecmem::device_vector
                    <const detail::DeviceSpacePoint>
                                topSPs(m_topSPsView);
                const auto currentTop_r = topSPs[top].r;
                auto weight = -(current.impact *
                                m_config.impactWeightFactor);

                uint32_t compatCounter = 0;
                // By default compatSeedLimit is 2 -> 2 is
                // currently hard coded, because variable length
                // arrays are not supported in SYCL kernels.
                float compatibleSeedR[2];
                for (auto i2 = tripletBegin;
                    i2 < tripletEnd &&
                    compatCounter < m_config.compatSeedLimit;
                    ++i2) {
                    const auto other = deviceCurvImpactConst[i2];

                    const auto otherCurv = other.curvature;
                    const auto otherTop_r =
                        topSPs[other.topSPIndex].r;
                    const float deltaR =
                        cl::sycl::abs(currentTop_r - otherTop_r);
                    if (deltaR >= m_config.filterDeltaRMin &&
                        otherCurv >= lowerLimitCurv &&
                        otherCurv <= upperLimitCurv) {
                        uint32_t c = 0;
                        for (;
                            c < compatCounter &&
                            cl::sycl::abs(compatibleSeedR[c] - otherTop_r) >=
                                m_config.filterDeltaRMin;
                            ++c) {
                        }
                        if (c == compatCounter) {
                            compatibleSeedR[c] = otherTop_r;
                            ++compatCounter;
                        }
                    }
                }
                weight +=
                    compatCounter * m_config.compatSeedWeight;
                const vecmem::device_vector
                    <const detail::DeviceSpacePoint>
                                middleSPs(m_middleSPsView),
                                bottomSPs(m_bottomSPsView);
                const auto bottomSP = bottomSPs[bot];
                const auto middleSP = middleSPs[mid];
                const auto topSP = topSPs[top];

                weight +=
                    m_cuts.seedWeight(bottomSP, middleSP, topSP);

                if (m_cuts.singleSeedCut(weight, bottomSP, middleSP,
                                            topSP)) {
                    detail::SeedData D;
                    D.bottom = bot;
                    D.top = top;
                    D.middle = mid;
                    D.weight = weight;
                    vecmem::device_vector<detail::SeedData>
                                            seedArray(m_seedArrayView);
                    seedArray.push_back(D);
                }
            }
        }
    }
    private:
    const uint32_t m_numTripletFilterThreads;
    vecmem::data::vector_view<uint32_t> m_sumBotMidView;
    const uint32_t m_firstMiddle;
    vecmem::data::vector_view<uint32_t> m_indMidBotCompView;
    vecmem::data::vector_view<uint32_t> m_indBotDupletView;
    vecmem::data::vector_view<uint32_t> m_sumBotTopCombView;
    vecmem::data::jagged_vector_view<uint32_t> m_midTopDupletView;
    vecmem::data::vector_view<detail::DeviceTriplet> m_curvImpactView;
    vecmem::data::vector_view<const detail::DeviceSpacePoint> m_topSPsView;
    vecmem::data::vector_view<const detail::DeviceSpacePoint> m_middleSPsView;
    vecmem::data::vector_view<const detail::DeviceSpacePoint> m_bottomSPsView;
    AtomicAccessorFilter m_countTripletsAcc;
    vecmem::data::vector_view<detail::SeedData> m_seedArrayView;
    DeviceSeedfinderConfig m_config;
    DeviceExperimentCuts m_cuts;
}; // struct TripletFilter
} // namespace Acts::Sycl::detail