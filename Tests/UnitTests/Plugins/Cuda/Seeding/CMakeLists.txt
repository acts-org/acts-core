# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

add_executable(ActsUnitTestSeedFinderCuda SeedFinderCudaTest.cpp)
target_link_libraries(
    ActsUnitTestSeedFinderCuda
    PRIVATE ${unittest_extra_libraries} Boost::boost
)
