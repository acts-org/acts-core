add_executable(ActsUnitTestSeedFinderCuda SeedFinderCudaTest.cpp)
target_link_libraries(
    ActsUnitTestSeedFinderCuda
    PRIVATE ${unittest_extra_libraries} Boost::boost
)
