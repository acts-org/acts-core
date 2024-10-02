// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Seeding/CandidatesForMiddleSp.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <type_traits>

namespace Acts {

template <typename external_spacepoint_t>
SeedFinder<external_spacepoint_t, Acts::Cuda>::SeedFinder(
    const Acts::SeedFinderConfig<external_spacepoint_t>& config,
    const Acts::SeedFinderOptions& options)
    : m_config(config), m_options(options) {
  if (not m_config.isInInternalUnits)
    throw std::runtime_error(
        "SeedFinderConfig not in ACTS internal units in "
        "Cuda/Seeding/SeedFinder");
  if (not m_options.isInInternalUnits)
    throw std::runtime_error(
        "SeedFinderOptions not in ACTS internal units in "
        "Cuda/Seeding/SeedFinder");
  if (std::isnan(m_config.deltaRMaxTopSP)) {
    throw std::runtime_error("Value of deltaRMaxTopSP was not initialised");
  }
  if (std::isnan(m_config.deltaRMinTopSP)) {
    throw std::runtime_error("Value of deltaRMinTopSP was not initialised");
  }
  if (std::isnan(m_config.deltaRMaxBottomSP)) {
    throw std::runtime_error("Value of deltaRMaxBottomSP was not initialised");
  }
  if (std::isnan(m_config.deltaRMinBottomSP)) {
    throw std::runtime_error("Value of deltaRMinBottomSP was not initialised");
  }
}

// CUDA seed finding
template <typename external_spacepoint_t>
template <typename sp_range_t>
std::vector<Seed<external_spacepoint_t>>
SeedFinder<external_spacepoint_t, Acts::Cuda>::createSeedsForGroup(
    Acts::SpacePointData& spacePointData,
    Acts::CylindricalSpacePointGrid<external_spacepoint_t>& grid,
    const sp_range_t& bottomSPs, const std::size_t middleSPs,
    const sp_range_t& topSPs) const {
  std::vector<Seed<external_spacepoint_t>> outputVec;

  // Get SeedFinderConfig values
  CudaScalar<float> deltaRMin_cuda(&m_config.deltaRMin);
  CudaScalar<float> deltaRMax_cuda(&m_config.deltaRMax);
  CudaScalar<float> cotThetaMax_cuda(&m_config.cotThetaMax);
  CudaScalar<float> collisionRegionMin_cuda(&m_config.collisionRegionMin);
  CudaScalar<float> collisionRegionMax_cuda(&m_config.collisionRegionMax);
  CudaScalar<float> maxScatteringAngle2_cuda(&m_config.maxScatteringAngle2);
  CudaScalar<float> sigmaScattering_cuda(&m_config.sigmaScattering);
  CudaScalar<float> minHelixDiameter2_cuda(&m_options.minHelixDiameter2);
  CudaScalar<float> pT2perRadius_cuda(&m_options.pT2perRadius);
  CudaScalar<float> impactMax_cuda(&m_config.impactMax);
  const auto seedFilterConfig = m_config.seedFilter->getSeedFilterConfig();
  CudaScalar<float> deltaInvHelixDiameter_cuda(
      &seedFilterConfig.deltaInvHelixDiameter);
  CudaScalar<float> impactWeightFactor_cuda(
      &seedFilterConfig.impactWeightFactor);
  CudaScalar<float> filterDeltaRMin_cuda(&seedFilterConfig.deltaRMin);
  CudaScalar<float> compatSeedWeight_cuda(&seedFilterConfig.compatSeedWeight);
  CudaScalar<std::size_t> compatSeedLimit_cuda(
      &seedFilterConfig.compatSeedLimit);
  CpuScalar<std::size_t> compatSeedLimit_cpu(&compatSeedLimit_cuda);
  //---------------------------------
  // Algorithm 0. Matrix Flattening
  //---------------------------------

  std::vector<Acts::InternalSpacePoint<external_spacepoint_t>*> middleSPvec;
  std::vector<Acts::InternalSpacePoint<external_spacepoint_t>*> bottomSPvec;
  std::vector<Acts::InternalSpacePoint<external_spacepoint_t>*> topSPvec;

  // Get the size of spacepoints
  int nSpM(0);
  int nSpB(0);
  int nSpT(0);

  {
    auto& sp_collection = grid.at(middleSPs);
    for (auto& sp : sp_collection) {
      nSpM++;
      middleSPvec.push_back(sp.get());
    }
  }
  for (auto idx : bottomSPs) {
    auto& sp_collection = grid.at(idx);
    for (auto& sp : sp_collection) {
      nSpB++;
      bottomSPvec.push_back(sp.get());
    }
  }
  for (std::size_t idx : topSPs) {
    auto& sp_collection = grid.at(idx);
    for (auto& sp : sp_collection) {
      nSpT++;
      topSPvec.push_back(sp.get());
    }
  }

  CudaScalar<int> nSpM_cuda(&nSpM);
  CudaScalar<int> nSpB_cuda(&nSpB);
  CudaScalar<int> nSpT_cuda(&nSpT);

  if (nSpM == 0 || nSpB == 0 || nSpT == 0)
    return outputVec;

  // Matrix flattening
  CpuMatrix<float> spMmat_cpu(nSpM, 6);  // x y z r varR varZ
  CpuMatrix<float> spBmat_cpu(nSpB, 6);
  CpuMatrix<float> spTmat_cpu(nSpT, 6);

  auto fillMatrix = [](auto& mat, auto& id, auto sp) {
    mat.set(id, 0, sp->x());
    mat.set(id, 1, sp->y());
    mat.set(id, 2, sp->z());
    mat.set(id, 3, sp->radius());
    mat.set(id, 4, sp->varianceR());
    mat.set(id, 5, sp->varianceZ());
    id++;
  };

  int mIdx(0);
  for (auto sp : middleSPvec) {
    fillMatrix(spMmat_cpu, mIdx, sp);
  }
  int bIdx(0);
  for (auto sp : bottomSPvec) {
    fillMatrix(spBmat_cpu, bIdx, sp);
  }
  int tIdx(0);
  for (auto sp : topSPvec) {
    fillMatrix(spTmat_cpu, tIdx, sp);
  }

  CudaMatrix<float> spMmat_cuda(nSpM, 6, &spMmat_cpu);
  CudaMatrix<float> spBmat_cuda(nSpB, 6, &spBmat_cpu);
  CudaMatrix<float> spTmat_cuda(nSpT, 6, &spTmat_cpu);
  //------------------------------------
  //  Algorithm 1. Doublet Search (DS)
  //------------------------------------

  CudaScalar<int> nSpMcomp_cuda(new int{0});
  CudaScalar<int> nSpBcompPerSpMMax_cuda(new int{0});
  CudaScalar<int> nSpTcompPerSpMMax_cuda(new int{0});
  CudaVector<int> nSpBcompPerSpM_cuda(nSpM);
  nSpBcompPerSpM_cuda.zeros();
  CudaVector<int> nSpTcompPerSpM_cuda(nSpM);
  nSpTcompPerSpM_cuda.zeros();
  CudaVector<int> McompIndex_cuda(nSpM);
  CudaMatrix<int> BcompIndex_cuda(nSpB, nSpM);
  CudaMatrix<int> TcompIndex_cuda(nSpT, nSpM);
  CudaMatrix<int> tmpBcompIndex_cuda(nSpB, nSpM);
  CudaMatrix<int> tmpTcompIndex_cuda(nSpT, nSpM);

  dim3 DS_BlockSize = m_config.maxBlockSize;
  dim3 DS_GridSize(nSpM, 1, 1);

  searchDoublet(DS_GridSize, DS_BlockSize, nSpM_cuda.get(), spMmat_cuda.get(),
                nSpB_cuda.get(), spBmat_cuda.get(), nSpT_cuda.get(),
                spTmat_cuda.get(), deltaRMin_cuda.get(), deltaRMax_cuda.get(),
                cotThetaMax_cuda.get(), collisionRegionMin_cuda.get(),
                collisionRegionMax_cuda.get(), nSpMcomp_cuda.get(),
                nSpBcompPerSpMMax_cuda.get(), nSpTcompPerSpMMax_cuda.get(),
                nSpBcompPerSpM_cuda.get(), nSpTcompPerSpM_cuda.get(),
                McompIndex_cuda.get(), BcompIndex_cuda.get(),
                tmpBcompIndex_cuda.get(), TcompIndex_cuda.get(),
                tmpTcompIndex_cuda.get());

  CpuScalar<int> nSpMcomp_cpu(&nSpMcomp_cuda);
  CpuScalar<int> nSpBcompPerSpMMax_cpu(&nSpBcompPerSpMMax_cuda);
  CpuScalar<int> nSpTcompPerSpMMax_cpu(&nSpTcompPerSpMMax_cuda);
  CpuVector<int> nSpBcompPerSpM_cpu(nSpM, &nSpBcompPerSpM_cuda);
  CpuVector<int> nSpTcompPerSpM_cpu(nSpM, &nSpTcompPerSpM_cuda);
  CpuVector<int> McompIndex_cpu(nSpM, &McompIndex_cuda);

  //--------------------------------------
  //  Algorithm 2. Transform coordinate
  //--------------------------------------

  CudaMatrix<float> spMcompMat_cuda(*nSpMcomp_cpu.get(), 6);
  CudaMatrix<float> spBcompMatPerSpM_cuda(*nSpBcompPerSpMMax_cpu.get(),
                                          (*nSpMcomp_cpu.get()) * 6);
  CudaMatrix<float> spTcompMatPerSpM_cuda(*nSpTcompPerSpMMax_cpu.get(),
                                          (*nSpMcomp_cpu.get()) * 6);
  CudaMatrix<float> circBcompMatPerSpM_cuda(*nSpBcompPerSpMMax_cpu.get(),
                                            (*nSpMcomp_cpu.get()) * 6);
  CudaMatrix<float> circTcompMatPerSpM_cuda(*nSpTcompPerSpMMax_cpu.get(),
                                            (*nSpMcomp_cpu.get()) * 6);

  dim3 TC_GridSize(*nSpMcomp_cpu.get(), 1, 1);
  dim3 TC_BlockSize = m_config.maxBlockSize;

  transformCoordinate(
      TC_GridSize, TC_BlockSize, nSpM_cuda.get(), spMmat_cuda.get(),
      McompIndex_cuda.get(), nSpB_cuda.get(), spBmat_cuda.get(),
      nSpBcompPerSpMMax_cuda.get(), BcompIndex_cuda.get(), nSpT_cuda.get(),
      spTmat_cuda.get(), nSpTcompPerSpMMax_cuda.get(), TcompIndex_cuda.get(),
      spMcompMat_cuda.get(), spBcompMatPerSpM_cuda.get(),
      circBcompMatPerSpM_cuda.get(), spTcompMatPerSpM_cuda.get(),
      circTcompMatPerSpM_cuda.get());

  //------------------------------------------------------
  //  Algorithm 3. Triplet Search (TS) & Seed filtering
  //------------------------------------------------------

  const int nTrplPerSpMLimit =
      m_config.nAvgTrplPerSpBLimit * (*nSpBcompPerSpMMax_cpu.get());
  CudaScalar<int> nTrplPerSpMLimit_cuda(&nTrplPerSpMLimit);

  CudaScalar<int> nTrplPerSpBLimit_cuda(&m_config.nTrplPerSpBLimit);
  CpuScalar<int> nTrplPerSpBLimit_cpu(
      &nTrplPerSpBLimit_cuda);  // need to be USM

  CudaVector<int> nTrplPerSpM_cuda(*nSpMcomp_cpu.get());
  nTrplPerSpM_cuda.zeros();
  CudaMatrix<Triplet> TripletsPerSpM_cuda(nTrplPerSpMLimit,
                                          *nSpMcomp_cpu.get());
  CpuVector<int> nTrplPerSpM_cpu(*nSpMcomp_cpu.get(), true);
  nTrplPerSpM_cpu.zeros();
  CpuMatrix<Triplet> TripletsPerSpM_cpu(nTrplPerSpMLimit, *nSpMcomp_cpu.get(),
                                        true);
  cudaStream_t cuStream;
  ACTS_CUDA_ERROR_CHECK(cudaStreamCreate(&cuStream));

  for (int i_m = 0; i_m <= *nSpMcomp_cpu.get(); i_m++) {
    cudaStreamSynchronize(cuStream);

    // Search Triplet
    if (i_m < *nSpMcomp_cpu.get()) {
      int mIndex = *McompIndex_cpu.get(i_m);
      int* nSpBcompPerSpM = nSpBcompPerSpM_cpu.get(mIndex);
      int* nSpTcompPerSpM = nSpTcompPerSpM_cpu.get(mIndex);

      dim3 TS_GridSize(*nSpBcompPerSpM, 1, 1);
      dim3 TS_BlockSize =
          dim3(fmin(m_config.maxBlockSize, *nSpTcompPerSpM), 1, 1);

      searchTriplet(
          TS_GridSize, TS_BlockSize, nSpTcompPerSpM_cpu.get(mIndex),
          nSpTcompPerSpM_cuda.get(mIndex), nSpMcomp_cuda.get(),
          spMcompMat_cuda.get(i_m, 0), nSpBcompPerSpMMax_cuda.get(),
          BcompIndex_cuda.get(0, i_m), circBcompMatPerSpM_cuda.get(0, 6 * i_m),
          nSpTcompPerSpMMax_cuda.get(), TcompIndex_cuda.get(0, i_m),
          spTcompMatPerSpM_cuda.get(0, 6 * i_m),
          circTcompMatPerSpM_cuda.get(0, 6 * i_m),
          // Seed finder config
          maxScatteringAngle2_cuda.get(), sigmaScattering_cuda.get(),
          minHelixDiameter2_cuda.get(), pT2perRadius_cuda.get(),
          impactMax_cuda.get(), nTrplPerSpMLimit_cuda.get(),
          nTrplPerSpBLimit_cpu.get(), nTrplPerSpBLimit_cuda.get(),
          deltaInvHelixDiameter_cuda.get(), impactWeightFactor_cuda.get(),
          filterDeltaRMin_cuda.get(), compatSeedWeight_cuda.get(),
          compatSeedLimit_cpu.get(), compatSeedLimit_cuda.get(),
          // output
          nTrplPerSpM_cuda.get(i_m), TripletsPerSpM_cuda.get(0, i_m),
          &cuStream);
      nTrplPerSpM_cpu.copyD2H(nTrplPerSpM_cuda.get(i_m), 1, i_m, &cuStream);

      TripletsPerSpM_cpu.copyD2H(TripletsPerSpM_cuda.get(0, i_m),
                                 nTrplPerSpMLimit, nTrplPerSpMLimit * i_m,
                                 &cuStream);
    }

    if (i_m > 0) {
      const auto m_experimentCuts = m_config.seedFilter->getExperimentCuts();
      std::vector<typename CandidatesForMiddleSp<
          const InternalSpacePoint<external_spacepoint_t>>::value_type>
          candidates;

      for (int i = 0; i < *nTrplPerSpM_cpu.get(i_m - 1); i++) {
        auto& triplet = *TripletsPerSpM_cpu.get(i, i_m - 1);
        int mIndex = *McompIndex_cpu.get(i_m - 1);
        int bIndex = triplet.bIndex;
        int tIndex = triplet.tIndex;

        auto& bottomSP = *bottomSPvec[bIndex];
        auto& middleSP = *middleSPvec[mIndex];
        auto& topSP = *topSPvec[tIndex];
        if (m_experimentCuts != nullptr) {
          // add detector specific considerations on the seed weight
          triplet.weight +=
              m_experimentCuts->seedWeight(bottomSP, middleSP, topSP);
          // discard seeds according to detector specific cuts (e.g.: weight)
          if (!m_experimentCuts->singleSeedCut(triplet.weight, bottomSP,
                                               middleSP, topSP)) {
            continue;
          }
        }

        float Zob = 0;  // It is not used in the seed filter but needs to be
                        // fixed anyway...

        candidates.emplace_back(bottomSP, middleSP, topSP, triplet.weight, Zob,
                                false);
      }

      std::ranges::sort(candidates,
                        CandidatesForMiddleSp<const InternalSpacePoint<
                            external_spacepoint_t>>::descendingByQuality);
      std::size_t numQualitySeeds = 0;  // not used but needs to be fixed
      m_config.seedFilter->filterSeeds_1SpFixed(spacePointData, candidates,
                                                numQualitySeeds,
                                                std::back_inserter(outputVec));
    }
  }
  ACTS_CUDA_ERROR_CHECK(cudaStreamDestroy(cuStream));
  return outputVec;
}
}  // namespace Acts
