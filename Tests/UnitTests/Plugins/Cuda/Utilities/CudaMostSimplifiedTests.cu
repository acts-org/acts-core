// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Plugins/Cuda/Cuda.hpp"

#include <Eigen/Dense>
#include <cuda_profiler_api.h>

template <typename AFloat, int row, int col>
__global__ void MatrixLoadStore(const Eigen::Matrix<AFloat, row, col>* input,
                                Eigen::Matrix<AFloat, row, col>* output) {
  for (int i = 0; i < col; i++) {
    output[blockIdx.x](threadIdx.x, i) = input[blockIdx.x](threadIdx.x, i);
  }
}

namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(Utilities)
BOOST_AUTO_TEST_CASE(CUDAOBJ_TEST) {
  const int vecDim = 100;  // Vector imension
  const int nVec = 128;    // Number of vectors

  dim3 gridSize(1, 1, 1);
  dim3 blockSize(vecDim, 1, 1);
  int bufSize;

  bufSize = gridSize.x * blockSize.x;
  Eigen::Matrix<float, vecDim, nVec> inMat_cpu[bufSize];
  for (int i = 0; i < bufSize; i++) {
    inMat_cpu[i] = Eigen::Matrix<float, vecDim, nVec>::Random();
  }

  cudaProfilerStart();

  CudaVector<Eigen::Matrix<float, vecDim, nVec>> inMat_cuda(bufSize, inMat_cpu,
                                                            bufSize, 0);
  CudaVector<Eigen::Matrix<float, vecDim, nVec>> outMat_cuda(bufSize);
  MatrixLoadStore<float, vecDim, nVec>
      <<<gridSize, blockSize>>>(inMat_cuda.get(), outMat_cuda.get());
  CpuVector<Eigen::Matrix<float, vecDim, nVec>> outMat_cpu(bufSize,
                                                           &outMat_cuda);

  cudaProfilerStop();

  BOOST_REQUIRE_EQUAL(inMat_cpu[0], *outMat_cpu.get(0));
}
BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
