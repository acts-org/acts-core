add_executable(
    ActsUnitTestSeedFinderCuda2
    main.cpp
    CommandLineArguments.hpp
    CommandLineArguments.cpp
    ReadSeedFile.hpp
    ReadSeedFile.cpp
    TestSpacePoint.hpp
    TestSpacePoint.cpp
    TestDeviceCuts.hpp
    TestDeviceCuts.cu
    TestHostCuts.hpp
    TestHostCuts.cpp
)
target_link_libraries(
    ActsUnitTestSeedFinderCuda2
    PRIVATE Boost::program_options ActsCore ActsPluginCuda2
)
set_target_properties(
    ActsUnitTestSeedFinderCuda2
    PROPERTIES CUDA_SEPARABLE_COMPILATION ON
)
