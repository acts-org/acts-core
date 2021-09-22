// This file is part of the Acts project.
//
// Copyright (C) 2020-2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// System include(s)
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <exception>
#include <functional>
#include <vector>
#include <memory>

// Acts include(s)
#include "Acts/Utilities/Logger.hpp"

// SYCL plugin include(s)
#include "Acts/Plugins/Sycl/Seeding/CreateSeedsForGroupSycl.hpp"
#include "Acts/Plugins/Sycl/Seeding/detail/Types.hpp"
#include "Acts/Plugins/Sycl/Utilities/CalculateNdRange.hpp"

#include "../Utilities/Arrays.hpp"
#include "DupletSearch.hpp"
#include "LinearTransform.hpp"
#include "TripletSearch.hpp"
#include "TripletFilter.hpp"

// VecMem include(s).
#include "vecmem/containers/data/jagged_vector_buffer.hpp"
#include "vecmem/containers/data/vector_buffer.hpp"
#include "vecmem/utils/sycl/copy.hpp"

// SYCL include
#include <CL/sycl.hpp>

namespace Acts::Sycl {
// Kernel classes in order of execution.
class ind_copy_bottom_kernel;
class ind_copy_top_kernel;
class triplet_search_kernel;
class filter_2sp_fixed_kernel;

void createSeedsForGroupSycl(
    QueueWrapper wrappedQueue,
    vecmem::memory_resource& resource,
    vecmem::memory_resource* device_resource,
    const detail::DeviceSeedfinderConfig& seedfinderConfig,
    const DeviceExperimentCuts& deviceCuts,
    vecmem::vector<detail::DeviceSpacePoint>& bottomSPs,
    vecmem::vector<detail::DeviceSpacePoint>& middleSPs,
    vecmem::vector<detail::DeviceSpacePoint>& topSPs,
    std::vector<std::vector<detail::SeedData>>& seeds) __attribute__((optnone)) {
  // Each vector stores data of space points in simplified
  // structures of float variables
  // M: number of middle space points
  // B: number of bottom space points
  // T: number of top space points
  const uint32_t M = middleSPs.size();
  const uint32_t B = bottomSPs.size();
  const uint32_t T = topSPs.size();

  // Up to the Nth space point, the sum of compatible bottom/top space points.
  // We need these for indexing other vectors later in the algorithm.
  // These are prefix sum arrays, with a leading zero.
  // Those will be created with either host or shared memory resource
  vecmem::vector<uint32_t> sumBotMidPrefix(&resource);
  sumBotMidPrefix.push_back(0);
  vecmem::vector<uint32_t> sumTopMidPrefix(&resource);
  sumTopMidPrefix.push_back(0);
  vecmem::vector<uint32_t> sumBotTopCombPrefix(&resource);
  sumBotTopCombPrefix.push_back(0);

  // After completing the duplet search, we'll have successfully contructed
  // two bipartite graphs for bottom-middle and top-middle space points.
  // We store the indices of the middle space points of the corresponding
  // edges.
  vecmem::vector<uint32_t> indMidBotComp(&resource);
  vecmem::vector<uint32_t> indMidTopComp(&resource);

  try {
    auto* q = wrappedQueue.getQueue();
    uint64_t globalBufferSize =
        q->get_device().get_info<cl::sycl::info::device::global_mem_size>();
    uint64_t maxWorkGroupSize =
        q->get_device().get_info<cl::sycl::info::device::max_work_group_size>();
    vecmem::sycl::copy copy(wrappedQueue.getQueue());

    // Calculate 2 dimensional range of bottom-middle duplet search kernel
    // We'll have a total of M*B threads globally, but we need to give the
    // nd_range the global dimensions so that they are an exact multiple of
    // the local dimensions. That's why we need this calculation.

    cl::sycl::nd_range<2> bottomDupletNDRange =
        calculate2DimNDRange(M, B, maxWorkGroupSize);
    cl::sycl::nd_range<2> topDupletNDRange =
        calculate2DimNDRange(M, T, maxWorkGroupSize);

    // Create views of the space point vectors. 
    // They will be constructed differently depending on the number of memory resources given.
    std::unique_ptr<vecmem::data::vector_buffer<detail::DeviceSpacePoint>> deviceBottomSPs,
                                                                           deviceTopSPs,
                                                                           deviceMiddleSPs;
    vecmem::data::vector_view<detail::DeviceSpacePoint> bottomSPsView,
                                                        topSPsView,
                                                        middleSPsView;
    if (!device_resource){
      bottomSPsView = vecmem::get_data(bottomSPs);
      topSPsView = vecmem::get_data(topSPs);
      middleSPsView = vecmem::get_data(middleSPs);
    } else {
      deviceBottomSPs = std::make_unique<vecmem::data::vector_buffer
                                          <detail::DeviceSpacePoint>>
                                              (B, *device_resource);
      deviceTopSPs = std::make_unique<vecmem::data::vector_buffer
                                          <detail::DeviceSpacePoint>>
                                              (T, *device_resource);
      deviceMiddleSPs = std::make_unique<vecmem::data::vector_buffer
                                          <detail::DeviceSpacePoint>>
                                              (M, *device_resource);

      copy(vecmem::get_data(bottomSPs), *deviceBottomSPs);
      copy(vecmem::get_data(topSPs), *deviceTopSPs);
      copy(vecmem::get_data(middleSPs), *deviceMiddleSPs);

      bottomSPsView = vecmem::get_data(*deviceBottomSPs);
      topSPsView = vecmem::get_data(*deviceTopSPs);
      middleSPsView = vecmem::get_data(*deviceMiddleSPs);
    }
    //*********************************************//
    // ********** DUPLET SEARCH - BEGIN ********** //
    //*********************************************//

    // Create the output data of the duplet search - jagged vectors.
    std::unique_ptr<vecmem::data::jagged_vector_buffer<uint32_t>> midBotDupletBuffer;
    std::unique_ptr<vecmem::data::jagged_vector_buffer<uint32_t>> midTopDupletBuffer;

    midBotDupletBuffer = std::make_unique<vecmem::data::jagged_vector_buffer<uint32_t>>
                                          (std::vector<std::size_t>(M, 0),
                                            std::vector<std::size_t>(M,B),
                                            (device_resource ? *device_resource : resource),
                                            (device_resource ? &resource : nullptr));
    midTopDupletBuffer = std::make_unique<vecmem::data::jagged_vector_buffer<uint32_t>>
                                              (std::vector<std::size_t>(M, 0),
                                                std::vector<std::size_t>(M,T),
                                                (device_resource ? *device_resource : resource),
                                                (device_resource ? &resource : nullptr));
    copy.setup(*midBotDupletBuffer);
    copy.setup(*midTopDupletBuffer);

    // Atomic accessor type used throughout the code.
    using AtomicAccessor =
        sycl::ONEAPI::atomic_accessor<uint32_t, 1,
                                      sycl::ONEAPI::memory_order::relaxed,
                                      sycl::ONEAPI::memory_scope::device>;

    // Perform the middle-bottom duplet search.
    auto middleBottomEvent = q->submit([&](cl::sycl::handler& h) {
      detail::DupletSearch<detail::SpacePointType::Bottom>
          kernel(middleSPsView, bottomSPsView,
                 *midBotDupletBuffer, seedfinderConfig);
      h.parallel_for<class DupletSearchBottomKernel>(bottomDupletNDRange,
                                                     kernel);
    });

    // Perform the middle-top duplet search.
    auto middleTopEvent = q->submit([&](cl::sycl::handler& h) {
      detail::DupletSearch<detail::SpacePointType::Top>
          kernel(middleSPsView, topSPsView,
                 *midTopDupletBuffer, seedfinderConfig);
      h.parallel_for<class DupletSearchTopKernel>(topDupletNDRange, kernel);
    });
    middleBottomEvent.wait_and_throw();
    middleTopEvent.wait_and_throw();
    //*********************************************//
    // *********** DUPLET SEARCH - END *********** //
    //*********************************************//

    // Get the sizes of the inner vectors of the jagged vector - number of compatible bottom/top SPs for each MiddleSP.
    auto countBotDuplets = copy.get_sizes(*midBotDupletBuffer);
    auto countTopDuplets = copy.get_sizes(*midTopDupletBuffer);
    // Construct prefix sum arrays of duplet counts.
    // These will later be used to index other arrays based on middle SP
    // indices.
    for (uint32_t i = 1; i < M + 1; ++i) {
      sumBotMidPrefix.push_back(
          sumBotMidPrefix.at(i - 1) + countBotDuplets[i - 1]);
      sumTopMidPrefix.push_back(
          sumTopMidPrefix.at(i - 1) + countTopDuplets[i - 1]);
      sumBotTopCombPrefix.push_back(
          sumBotTopCombPrefix.at(i - 1) +
          countBotDuplets[i - 1] *
          countTopDuplets[i - 1]);
    }
    // Number of edges for middle-bottom and middle-top duplet bipartite graphs.
    const uint64_t edgesBottom = sumBotMidPrefix[M];
    const uint64_t edgesTop = sumTopMidPrefix[M];
    // Number of possible compatible triplets. This is the sum of the
    // combination of the number of compatible bottom and compatible top duplets
    // per middle space point. (nb0*nt0 + nb1*nt1 + ... where nbk is the number
    // of comp. bot. SPs for the kth middle SP)
    const uint64_t edgesComb = sumBotTopCombPrefix[M];

    indMidBotComp.reserve(edgesBottom);
    indMidTopComp.reserve(edgesTop);

    // Fill arrays of middle SP indices of found duplets (bottom and top).
    for (uint32_t mid = 0; mid < M; ++mid) {
      std::fill_n(std::back_inserter(indMidBotComp), countBotDuplets[mid],
                  mid);
      std::fill_n(std::back_inserter(indMidTopComp), countTopDuplets[mid],
                  mid);
    }

    if (edgesBottom > 0 && edgesTop > 0) {
      // Calcualte global and local range of execution for edgesBottom number of
      // threads. Local range is the same as block size in CUDA.
      cl::sycl::nd_range<1> edgesBotNdRange =
          calculate1DimNDRange(edgesBottom, maxWorkGroupSize);

      // Global and local range of execution for edgesTop number of threads.
      cl::sycl::nd_range<1> edgesTopNdRange =
          calculate1DimNDRange(edgesTop, maxWorkGroupSize);

      // EXPLANATION OF INDEXING (fisrt part)
      /*
        (for bottom-middle duplets, but it is the same for middle-tops)
        In case we have 4 middle SP and 5 bottom SP, our temporary array of
        the compatible bottom duplet indices would look like this:
             ---------------------
        mid0 | 0 | 3 | 4 | 1 | - |    Indices in the columns correspond to
        mid1 | 3 | 2 | - | - | - |    bottom SP indices in the bottomSPs
        mid2 | - | - | - | - | - |    array. Threads are executed concurrently,
        mid3 | 4 | 2 | 1 | - | - |    so the order of indices is random.
             ---------------------
        We will refer to this structure as a bipartite graph, as it can be
        described by a graph of nodes for middle and bottom SPs, and edges
        between one middle and one bottom SP, but never two middle or two
        bottom SPs.
        We will flatten this matrix out, and store the indices the
        following way (this is indBotDupletBuffer):
        -------------------------------------
        | 0 | 3 | 4 | 1 | 3 | 2 | 4 | 2 | 1 |
        -------------------------------------
        Also the length of this array is equal to edgesBottom, which is 9 in
        this example. It is the number of the edges of the bottom-middle
        bipartite graph.
        To find out where the indices of bottom SPs start for a particular
        middle SP, we use prefix sum arrays.
        We know how many duplets were found for each middle SP (this is
        countBotDuplets).
        -----------------
        | 4 | 2 | 0 | 3 |
        -----------------
        We will make a prefix sum array of these counts, with a leading zero:
        (this is sumBotMidPrefix)
        ---------------------
        | 0 | 4 | 6 | 6 | 9 |
        ---------------------
        If we have the middle SP with index 1, then we know that the indices
        of the compatible bottom SPs are in the range (left closed, right
        open) [sumBotMidPrefix[1] , sumBotMidPrefix[2] ) of indBotDUpletBuffer. In this
        case, these indices are 3 and 2, so we'd use these to index
        views of bottomSPs to gather data about the bottom SP.
        To be able to get the indices of middle SPs in constant time inside
        kernels, we will also prepare arrays that store the indices of the
        middleSPs of the edges (indMidBotComp).
        -------------------------------------
        | 0 | 0 | 0 | 0 | 1 | 1 | 3 | 3 | 3 |
        -------------------------------------
        (For the same purpose, we could also do a binary search on the
        sumBotMidPrefix array, and we will do exactly that later, in the triplet
        search kernel.)
        We will execute the coordinate transformation on edgesBottom threads,
        or 9 in our example.
        The size of the array storing our transformed coordinates
        (linearBotBuffer) is also edgesBottom, the sum of bottom duplets we
        found so far.
      */

      // We store the indices of the BOTTOM/TOP space points of the edges of
      // the bottom-middle and top-middle bipartite duplet graphs. They index
      // the bottomSPs and topSPs vectors.

      // We store the indices of the MIDDLE space points of the edges of the
      // bottom-middle and top-middle bipartite duplet graphs.
      // They index the middleSP vector.
      // indMidBotComp;
      // indMidTopComp;

      // Partial sum arrays of deviceNumBot and deviceNum
      // Partial sum array of the combinations of compatible bottom and top
      // space points per middle space point.
      // Allocations for coordinate transformation.


      // Buffers for Flattening the jagged vectors
      std::unique_ptr<vecmem::data::vector_buffer<uint32_t>> indBotDupletBuffer;
      std::unique_ptr<vecmem::data::vector_buffer<uint32_t>> indTopDupletBuffer;

      indBotDupletBuffer = std::make_unique<vecmem::data::vector_buffer<uint32_t>>
                                        (edgesBottom,(device_resource ? *device_resource : resource));
      indTopDupletBuffer = std::make_unique<vecmem::data::vector_buffer<uint32_t>>
                                        (edgesTop,(device_resource ? *device_resource : resource));  

      copy.setup(*indBotDupletBuffer);
      copy.setup(*indTopDupletBuffer);

      // Pointers constructed in case the device memory resource was given.
      std::unique_ptr<vecmem::data::vector_buffer<uint32_t>> device_sumBotMidPrefix,
                                                             device_sumTopMidPrefix,
                                                             device_sumBotTopCombPrefix;
      // Vecmem views of the prefix sums used throughout the later code.
      vecmem::data::vector_view<uint32_t> sumBotMidView,
                                          sumTopMidView,
                                          sumBotTopCombView;

      // Same behaviour for the vectors of indices
      std::unique_ptr<vecmem::data::vector_buffer<uint32_t>> device_indMidBotComp,
                                                             device_indMidTopComp;
      vecmem::data::vector_view<uint32_t> indMidBotCompView,
                                          indMidTopCompView;
      // Copy indices from temporary matrices to final, optimal size vectors.
      // We will use these for easier indexing.
      {
        if (!device_resource){
          sumBotMidView = vecmem::get_data(sumBotMidPrefix);
          sumTopMidView = vecmem::get_data(sumTopMidPrefix);
          sumBotTopCombView = vecmem::get_data(sumBotTopCombPrefix);

          indMidBotCompView = vecmem::get_data(indMidBotComp);
          indMidTopCompView = vecmem::get_data(indMidTopComp);
        } else {
          device_sumBotMidPrefix = std::make_unique<vecmem::data::vector_buffer<uint32_t>>(M+1, *device_resource);
          device_sumTopMidPrefix = std::make_unique<vecmem::data::vector_buffer<uint32_t>>(M+1, *device_resource);
          device_sumBotTopCombPrefix = std::make_unique<vecmem::data::vector_buffer<uint32_t>>(M+1, *device_resource);

          device_indMidBotComp = std::make_unique<vecmem::data::vector_buffer<uint32_t>>(edgesBottom, *device_resource);
          device_indMidTopComp = std::make_unique<vecmem::data::vector_buffer<uint32_t>>(edgesTop, *device_resource);
          
          copy(vecmem::get_data(sumBotMidPrefix), *device_sumBotMidPrefix);
          copy(vecmem::get_data(sumTopMidPrefix), *device_sumTopMidPrefix);
          copy(vecmem::get_data(sumBotTopCombPrefix), *device_sumBotTopCombPrefix);
        
          copy(vecmem::get_data(indMidBotComp), *device_indMidBotComp);
          copy(vecmem::get_data(indMidTopComp), *device_indMidTopComp);

          sumBotMidView = vecmem::get_data(*device_sumBotMidPrefix);
          sumTopMidView = vecmem::get_data(*device_sumTopMidPrefix);
          sumBotTopCombView = vecmem::get_data(*device_sumBotTopCombPrefix);

          indMidBotCompView = vecmem::get_data(*device_indMidBotComp);
          indMidTopCompView = vecmem::get_data(*device_indMidTopComp);
        }
        auto midBotDupletView = vecmem::get_data(*midBotDupletBuffer);
        auto indBotDupletView = vecmem::get_data(*indBotDupletBuffer);
        q->submit([&](cl::sycl::handler& h) {
          h.parallel_for<ind_copy_bottom_kernel>(
              edgesBotNdRange, [=](cl::sycl::nd_item<1> item) {
                auto idx = item.get_global_linear_id();
                if (idx < edgesBottom) {
                  vecmem::device_vector<uint32_t>
                         deviceIndMidBot(indMidBotCompView),
                         sumBotMidPrefix(sumBotMidView),
                         indBotDuplets(indBotDupletView);
                  vecmem::jagged_device_vector<const uint32_t>
                      midBotDuplets(midBotDupletView);
                  auto mid = deviceIndMidBot[idx];
                  auto ind = midBotDuplets[mid][idx - sumBotMidPrefix[mid]];                                      
                  indBotDuplets[idx] = ind;
                }
              });
        });
        auto midTopDupletView = vecmem::get_data(*midTopDupletBuffer);
        auto indTopDupletView = vecmem::get_data(*indTopDupletBuffer);
        q->submit([&](cl::sycl::handler& h) {
          h.parallel_for<ind_copy_top_kernel>(
              edgesTopNdRange, [=](cl::sycl::nd_item<1> item) {
                auto idx = item.get_global_linear_id();
                if (idx < edgesTop) {
                  vecmem::device_vector<uint32_t>
                          deviceIndMidTop(indMidTopCompView),
                          sumTopMidPrefix(sumTopMidView),
                          indTopDuplets(indTopDupletView);
                  vecmem::jagged_device_vector<const uint32_t>
                      midTopDuplets(midTopDupletView);
                  auto mid = deviceIndMidTop[idx];
                  auto ind = midTopDuplets[mid][idx - sumTopMidPrefix[mid]];
                  indTopDuplets[idx] = ind;
                }
              });
        });
      }  // sync
      // Create the output data of the linear transform
      std::unique_ptr<vecmem::data::vector_buffer<detail::DeviceLinEqCircle>> linearBotBuffer;
      std::unique_ptr<vecmem::data::vector_buffer<detail::DeviceLinEqCircle>> linearTopBuffer;

      linearBotBuffer = std::make_unique<vecmem::data::vector_buffer<detail::DeviceLinEqCircle>>
                            (edgesBottom, (device_resource ? *device_resource : resource));
      linearTopBuffer = std::make_unique<vecmem::data::vector_buffer<detail::DeviceLinEqCircle>>
                            (edgesTop, (device_resource ? *device_resource : resource));          

      copy.setup(*linearBotBuffer);
      copy.setup(*linearTopBuffer);

      //************************************************//
      // *** LINEAR EQUATION TRANSFORMATION - BEGIN *** //
      //************************************************//

      // transformation of circle equation (x,y) into linear equation (u,v)
      // x^2 + y^2 - 2x_0*x - 2y_0*y = 0
      // is transformed into
      // 1 - 2x_0*u - 2y_0*v = 0

      // coordinate transformation middle-bottom pairs
      auto linB = q->submit([&](cl::sycl::handler& h) {
        detail::LinearTransform<detail::SpacePointType::Bottom> kernel(
            middleSPsView, bottomSPsView,
            indMidBotCompView, *indBotDupletBuffer, 
            edgesBottom, *linearBotBuffer);
        h.parallel_for<class TransformCoordBottomKernel>(edgesBotNdRange,
                                                         kernel);
      });

      // coordinate transformation middle-top pairs
      auto linT = q->submit([&](cl::sycl::handler& h) {
        detail::LinearTransform<detail::SpacePointType::Top> kernel(
            middleSPsView, topSPsView, 
            indMidTopCompView, *indTopDupletBuffer, 
            edgesTop, *linearTopBuffer);
        h.parallel_for<class TransformCoordTopKernel>(edgesTopNdRange, kernel);
      });

      //************************************************//
      // **** LINEAR EQUATION TRANSFORMATION - END **** //
      //************************************************//

      //************************************************//
      // *********** TRIPLET SEARCH - BEGIN *********** //
      //************************************************//

      // EXPLANATION OF INDEXING (second part)
      /*
        For the triplet search, we calculate the upper limit of constructible
        triplets.

        For this, we multiply the number of compatible bottom and compatible
        top SPs for each middle SP, and add these together.
        (nb0*nt0 + nb1*nt1 + ... where nbk is the number of compatible bottom
        SPs for the kth middle SP, similarly ntb is for tops)

        sumBotTopCombPrefix is a prefix sum array (of length M+1) of the
        calculated combinations.

        sumBotTopCombPrefix:
        ________________________________________________________
        |     |         |                   |     |  M         | M = number
        |  0  | nb0*nt0 | nb0*nt0 + nb1*nt1 | ... |  ∑ nbi+nti | of middle
        |_____|_________|___________________|_____|_i=0________| space points

        We will start kernels and reserve memory for these combinations but
        only so much we can fit into memory at once.

        We limit our memory usage to globalBufferSize/2, this is currently
        hard-coded, but it could be configured. Actually, it would be better
        to use a separate object that manages memory allocations and
        deallocations and we could ask it to lend us as much memory as it is
        happy to give.

        For later, let maxMemoryAllocation be maximum allocatable memory for
        triplet search.

        We start by adding up summing the combinations, until we arrive at a
        k which for:

        k+1
         ∑ nbi+nti > maxMemoryAllocation
        i=0
        (or k == M).

        So we know, that we need to start our first kernel for the first k
        middle SPs.

        Inside the triplet search kernel we start with a binary search, to
        find out which middle SP the thread corresponds to. Note, that
        sumBotTopCombPrefix is a monotone increasing series of values which
        allows us to do a binary search on it.

        Inside the triplet search kernel we count the triplets for fixed
        bottom and middle SP. This is deviceCountTriplets.

        The triplet filter kernel is calculated on threads equal to all possible
        bottom-middle combinations for the first k middle SPs, which are
        the sum of bottom-middle duplets. (For the next kernel it would be the
        bottom-middle combinations from the (k+1)th middle SP to another jth
        middle SP j<=M.)

        This will be numTripletFilterThreads =
            sumBotMidPrefix[lastMiddle] - sumBotMidPrefix[firstMiddle]

        If the triplet search and triplet filter kernel finished, we continue
        summing up possible triplet combinations from the (k+1)th middle SP.

        Inside the kernels we need to use offset because of this, to be able to
        map threads to space point indices.

        This offset is sumCombUptoFirstMiddle.
      */

      const auto maxMemoryAllocation =
          std::min(edgesComb,
                   globalBufferSize / uint64_t((sizeof(detail::DeviceTriplet) +
                                                sizeof(detail::SeedData)) *
                                               2));

      std::unique_ptr<vecmem::data::vector_buffer
                                  <detail::DeviceTriplet>>
                                            curvImpactBuffer;
      std::unique_ptr<vecmem::data::vector_buffer
                                  <detail::SeedData>>
                                            seedArrayBuffer;
     
      curvImpactBuffer = std::make_unique<vecmem::data::vector_buffer<detail::DeviceTriplet>>
                                  (maxMemoryAllocation, (device_resource ? *device_resource : resource));
      seedArrayBuffer = std::make_unique<vecmem::data::vector_buffer<detail::SeedData>>
                                  (maxMemoryAllocation, 0, (device_resource ? *device_resource : resource)); 
                                                                              
      copy.setup(*curvImpactBuffer);
      copy.setup(*seedArrayBuffer);
      // Reserve memory in advance for seed indices and weight
      // Other way around would allocating it inside the loop
      // -> less memory usage, but more frequent allocation and deallocation

      // Counting the seeds in the second kernel allows us to copy back the
      // right number of seeds, and no more.

      seeds.resize(M);
      std::vector<uint32_t> deviceCountTriplets(edgesBottom, 0);
      // Do the triplet search and triplet filter for 2 sp fixed for middle
      // space points in the interval [firstMiddle, lastMiddle).

      uint32_t lastMiddle = 0;
      for (uint32_t firstMiddle = 0; firstMiddle < M;
           firstMiddle = lastMiddle) {
        // Determine the interval [firstMiddle, lastMiddle) right end based
        // on memory requirements.
        while (lastMiddle + 1 <= M && (sumBotTopCombPrefix[lastMiddle + 1] -
                                           sumBotTopCombPrefix[firstMiddle] <
                                       maxMemoryAllocation)) {
          ++lastMiddle;
        }

        const auto numTripletSearchThreads =
            sumBotTopCombPrefix[lastMiddle] - sumBotTopCombPrefix[firstMiddle];

        if (numTripletSearchThreads == 0)
          continue;

        deviceCountTriplets.resize(edgesBottom, 0);
        copy.setup(*seedArrayBuffer);
        const auto numTripletFilterThreads =
            sumBotMidPrefix[lastMiddle] - sumBotMidPrefix[firstMiddle];

        // Nd_range with maximum block size for triplet search and filter.
        // (global and local range is already given)
        cl::sycl::nd_range<1> tripletSearchNDRange =
            calculate1DimNDRange(numTripletSearchThreads, maxWorkGroupSize);

        cl::sycl::nd_range<1> tripletFilterNDRange =
            calculate1DimNDRange(numTripletFilterThreads, maxWorkGroupSize);

        sycl::buffer<uint32_t> countTripletsBuf(deviceCountTriplets.data(),
                                                edgesBottom);
        
        auto tripletKernel = q->submit([&](cl::sycl::handler& h) {
          h.depends_on({linB, linT});
          AtomicAccessor countTripletsAcc(countTripletsBuf, h);
          detail::TripletSearch<AtomicAccessor>
              kernel(sumBotTopCombView, numTripletSearchThreads,
                     firstMiddle, lastMiddle, *midTopDupletBuffer, 
                     sumBotMidView, sumTopMidView,
                     *linearBotBuffer, *linearTopBuffer,
                     middleSPsView, *indTopDupletBuffer,
                     countTripletsAcc, seedfinderConfig, *curvImpactBuffer);
          h.parallel_for<class triplet_search_kernel>(
              tripletSearchNDRange, kernel); });
        {
          q->submit([&](cl::sycl::handler& h) {
            h.depends_on(tripletKernel);
            auto countTripletsAcc = countTripletsBuf.get_access<
                sycl::access::mode::read, sycl::access::target::global_buffer>(
                h);
            detail::TripletFilter<AtomicAccessor>
                kernel(numTripletFilterThreads, sumBotMidView,
                       firstMiddle, indMidBotCompView,
                       *indBotDupletBuffer, sumBotTopCombView,
                       *midTopDupletBuffer, *curvImpactBuffer, topSPsView,
                       middleSPsView, bottomSPsView,
                       countTripletsAcc, *seedArrayBuffer, 
                       seedfinderConfig, deviceCuts);
            h.parallel_for<class filter_2sp_fixed_kernel>(
                tripletFilterNDRange, kernel);
          });
        } // sync
        // Retrieve results from triplet search
        std::vector<detail::SeedData> seedArray;
        copy(*seedArrayBuffer, seedArray);

          for (auto & t : seedArray) {
            seeds[t.middle].push_back(t);
          }
      }

      //************************************************//
      // ************ TRIPLET SEARCH - END ************ //
      //************************************************//
    }

  } catch (cl::sycl::exception const& e) {
    ACTS_LOCAL_LOGGER(
        Acts::getDefaultLogger("SyclSeeding", Acts::Logging::INFO));
    ACTS_FATAL("Caught synchronous SYCL exception:\n" << e.what())
    throw;
  }
};
}  // namespace Acts::Sycl
