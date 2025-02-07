# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

add_executable(ActsUnitTestSeedFinder SeedFinderTest.cpp)
target_link_libraries(ActsUnitTestSeedFinder PRIVATE ActsCore Boost::boost)

add_unittest(EstimateTrackParamsFromSeed EstimateTrackParamsFromSeedTest.cpp)
add_unittest(BinnedGroupTest BinnedGroupTest.cpp)
add_unittest(PathSeeder PathSeederTest.cpp)
add_unittest(HoughTransformTest HoughTransformTest.cpp)
add_unittest(UtilityFunctions UtilityFunctionsTests.cpp)
add_unittest(CandidatesForMiddleSp CandidatesForMiddleSpTests.cpp)
