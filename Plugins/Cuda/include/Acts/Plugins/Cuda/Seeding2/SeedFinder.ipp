// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

// CUDA plugin include(s).
#include "Acts/Plugins/Cuda/Seeding2/Details/CountDublets.hpp"
#include "Acts/Plugins/Cuda/Seeding2/Details/FindDublets.hpp"
#include "Acts/Plugins/Cuda/Seeding2/Details/FindTriplets.hpp"
#include "Acts/Plugins/Cuda/Seeding2/Details/Types.hpp"
#include "Acts/Plugins/Cuda/Utilities/Arrays.hpp"
#include "Acts/Plugins/Cuda/Utilities/Info.hpp"
#include "Acts/Plugins/Cuda/Utilities/MemoryManager.hpp"

// Acts include(s).
#include "Acts/Seeding/CandidatesForMiddleSp.hpp"
#include "Acts/Seeding/InternalSeed.hpp"
#include "Acts/Seeding/InternalSpacePoint.hpp"

// System include(s).
#include <algorithm>
#include <cstring>
#include <vector>

namespace Acts::Cuda {

template <typename external_spacepoint_t>
SeedFinder<external_spacepoint_t>::SeedFinder(
    Acts::SeedFinderConfig<external_spacepoint_t> commonConfig,
    const Acts::SeedFinderOptions& seedFinderOptions,
    const SeedFilterConfig& seedFilterConfig,
    const TripletFilterConfig& tripletFilterConfig, int device,
    std::unique_ptr<const Logger> incomingLogger)
    : m_commonConfig(commonConfig),
      m_seedFinderOptions(seedFinderOptions),
      m_seedFilterConfig(seedFilterConfig),
      m_tripletFilterConfig(tripletFilterConfig),
      m_device(device),
      m_logger(std::move(incomingLogger)) {
  if (not m_commonConfig.isInInternalUnits)
    throw std::runtime_error(
        "SeedFinderConfig not in ACTS internal units in "
        "Cuda/Seeding2/SeedFinder");
  if (not m_seedFinderOptions.isInInternalUnits)
    throw std::runtime_error(
        "SeedFinderConfig not in ACTS internal units in "
        "Cuda/Seeding2/SeedFinder");
  if (not m_seedFilterConfig.isInInternalUnits)
    throw std::runtime_error(
        "SeedFilterConfig not in ACTS internal units in "
        "Cuda/Seeding2/SeedFinder");
  if (std::isnan(m_commonConfig.deltaRMaxTopSP)) {
    throw std::runtime_error("Value of deltaRMaxTopSP was not initialised");
  }
  if (std::isnan(m_commonConfig.deltaRMinTopSP)) {
    throw std::runtime_error("Value of deltaRMinTopSP was not initialised");
  }
  if (std::isnan(m_commonConfig.deltaRMaxBottomSP)) {
    throw std::runtime_error("Value of deltaRMaxBottomSP was not initialised");
  }
  if (std::isnan(m_commonConfig.deltaRMinBottomSP)) {
    throw std::runtime_error("Value of deltaRMinBottomSP was not initialised");
  }

  // Tell the user what CUDA device will be used by the object.
  if (static_cast<std::size_t>(m_device) < Info::instance().devices().size()) {
    ACTS_DEBUG("Will be using device:\n"
               << Info::instance().devices()[m_device]);
  } else {
    ACTS_FATAL("Invalid CUDA device requested");
    throw std::runtime_error("Invalid CUDA device requested");
  }
}

template <typename external_spacepoint_t>
template <typename sp_range_t>
std::vector<Seed<external_spacepoint_t>>
SeedFinder<external_spacepoint_t>::createSeedsForGroup(
    Acts::SpacePointData& spacePointData,
    Acts::CylindricalSpacePointGrid<external_spacepoint_t>& grid,
    const sp_range_t& bottomSPs, const std::size_t middleSPs,
    const sp_range_t& topSPs) const {
  // Create an empty vector, to be returned already early on when no seeds can
  // be constructed.
  std::vector<Seed<external_spacepoint_t>> outputVec;

  //---------------------------------
  // Matrix Flattening
  //---------------------------------

  // Create more convenient vectors out of the space point containers.
  auto spVecMaker = [&grid](const sp_range_t& spRange) {
    std::vector<Acts::InternalSpacePoint<external_spacepoint_t>*> result;
    for (std::size_t idx : spRange) {
      auto& collection = grid.at(idx);
      for (auto& sp : collection) {
        result.push_back(sp.get());
      }
    }
    return result;
  };

  std::vector<Acts::InternalSpacePoint<external_spacepoint_t>*> bottomSPVec(
      spVecMaker(bottomSPs));
  std::vector<Acts::InternalSpacePoint<external_spacepoint_t>*> topSPVec(
      spVecMaker(topSPs));

  std::vector<Acts::InternalSpacePoint<external_spacepoint_t>*> middleSPVec;
  {
    auto& collection = grid.at(middleSPs);
    for (auto& sp : collection) {
      middleSPVec.push_back(sp.get());
    }
  }

  // If either one of them is empty, we have nothing to find.
  if ((middleSPVec.size() == 0) || (bottomSPVec.size() == 0) ||
      (topSPVec.size() == 0)) {
    return outputVec;
  }

  // Create helper objects for storing information about the spacepoints on the
  // host in single memory blobs.
  auto bottomSPArray = make_host_array<Details::SpacePoint>(bottomSPVec.size());
  auto middleSPArray = make_host_array<Details::SpacePoint>(middleSPVec.size());
  auto topSPArray = make_host_array<Details::SpacePoint>(topSPVec.size());

  // Fill these memory blobs.
  auto fillSPArray = [](Details::SpacePoint* array, const auto& spVec) {
    for (std::size_t i = 0; i < spVec.size(); ++i) {
      array[i].x = spVec[i]->x();
      array[i].y = spVec[i]->y();
      array[i].z = spVec[i]->z();
      array[i].radius = spVec[i]->radius();
      array[i].varianceR = spVec[i]->varianceR();
      array[i].varianceZ = spVec[i]->varianceZ();
    }
  };
  fillSPArray(bottomSPArray.get(), bottomSPVec);
  fillSPArray(middleSPArray.get(), middleSPVec);
  fillSPArray(topSPArray.get(), topSPVec);

  // Copy the memory blobs to the device.
  auto bottomSPDeviceArray =
      make_device_array<Details::SpacePoint>(bottomSPVec.size());
  auto middleSPDeviceArray =
      make_device_array<Details::SpacePoint>(middleSPVec.size());
  auto topSPDeviceArray =
      make_device_array<Details::SpacePoint>(topSPVec.size());
  copyToDevice(bottomSPDeviceArray, bottomSPArray, bottomSPVec.size());
  copyToDevice(middleSPDeviceArray, middleSPArray, middleSPVec.size());
  copyToDevice(topSPDeviceArray, topSPArray, topSPVec.size());

  //---------------------------------
  // GPU Execution
  //---------------------------------

  // Matrices holding the counts of the viable bottom-middle and middle-top
  // pairs.
  auto dubletCountsHost = make_host_array<unsigned int>(middleSPVec.size());
  memset(dubletCountsHost.get(), 0, middleSPVec.size() * sizeof(unsigned int));
  auto middleBottomCounts = make_device_array<unsigned int>(middleSPVec.size());
  copyToDevice(middleBottomCounts, dubletCountsHost, middleSPVec.size());
  auto middleTopCounts = make_device_array<unsigned int>(middleSPVec.size());
  copyToDevice(middleTopCounts, dubletCountsHost, middleSPVec.size());

  // Matrices holding the indices of the viable bottom-middle and middle-top
  // pairs.
  auto middleBottomDublets =
      make_device_array<std::size_t>(middleSPVec.size() * bottomSPVec.size());
  auto middleTopDublets =
      make_device_array<std::size_t>(middleSPVec.size() * topSPVec.size());

  // Launch the dublet finding code.
  Details::findDublets(
      m_commonConfig.maxBlockSize, bottomSPVec.size(), bottomSPDeviceArray,
      middleSPVec.size(), middleSPDeviceArray, topSPVec.size(),
      topSPDeviceArray, m_commonConfig.deltaRMin, m_commonConfig.deltaRMax,
      m_commonConfig.cotThetaMax, m_commonConfig.collisionRegionMin,
      m_commonConfig.collisionRegionMax, middleBottomCounts,
      middleBottomDublets, middleTopCounts, middleTopDublets);

  // Count the number of dublets that we have to launch the subsequent steps
  // for.
  Details::DubletCounts dubletCounts =
      Details::countDublets(m_commonConfig.maxBlockSize, middleSPVec.size(),
                            middleBottomCounts, middleTopCounts);

  // If no dublets/triplet candidates have been found, stop here.
  if ((dubletCounts.nDublets == 0) || (dubletCounts.nTriplets == 0)) {
    return outputVec;
  }

  // Launch the triplet finding code on all of the previously found dublets.
  auto tripletCandidates = Details::findTriplets(
      Info::instance().devices()[m_device], m_commonConfig.maxBlockSize,
      dubletCounts, m_seedFilterConfig, m_tripletFilterConfig,
      bottomSPVec.size(), bottomSPDeviceArray, middleSPVec.size(),
      middleSPDeviceArray, topSPVec.size(), topSPDeviceArray,
      middleBottomCounts, middleBottomDublets, middleTopCounts,
      middleTopDublets, m_commonConfig.maxScatteringAngle2,
      m_commonConfig.sigmaScattering, m_seedFinderOptions.minHelixDiameter2,
      m_seedFinderOptions.pT2perRadius, m_commonConfig.impactMax);
  assert(tripletCandidates.size() == middleSPVec.size());

  // Perform the final step of the filtering.
  std::size_t middleIndex = 0;
  auto triplet_itr = tripletCandidates.begin();
  auto triplet_end = tripletCandidates.end();
  for (; triplet_itr != triplet_end; ++triplet_itr, ++middleIndex) {
    std::vector<typename CandidatesForMiddleSp<
        const InternalSpacePoint<external_spacepoint_t>>::value_type>
        candidates;

    auto& middleSP = *(middleSPVec[middleIndex]);
    for (const Details::Triplet& triplet : *triplet_itr) {
      assert(triplet.bottomIndex < bottomSPVec.size());
      auto& bottomSP = *(bottomSPVec[triplet.bottomIndex]);
      assert(triplet.topIndex < topSPVec.size());
      auto& topSP = *(topSPVec[triplet.topIndex]);
      candidates.emplace_back(bottomSP, middleSP, topSP, triplet.weight, 0,
                              false);
    }
    std::ranges::sort(
        candidates,
        CandidatesForMiddleSp<const InternalSpacePoint<external_spacepoint_t>>::
            descendingByQuality);
    std::size_t numQualitySeeds = 0;  // not used but needs to be fixed
    m_commonConfig.seedFilter->filterSeeds_1SpFixed(
        spacePointData, candidates, numQualitySeeds,
        std::back_inserter(outputVec));
  }

  // Free up all allocated device memory.
  MemoryManager::instance().reset(m_device);

  // Return the collected spacepoints.
  return outputVec;
}

template <typename external_spacepoint_t>
void SeedFinder<external_spacepoint_t>::setLogger(
    std::unique_ptr<const Logger> newLogger) {
  return m_logger.swap(newLogger);
}

}  // namespace Acts::Cuda
