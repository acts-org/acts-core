# benchmarks tests are not registered with a common target
# add an benchmark executable w/ default dependencies
macro(add_benchmark _name)
    # automatically prefix the target name
    set(_target "ActsBenchmark${_name}")
    add_executable(${_target} ${ARGN})
    # define required BOOST_TEST_... macros here to ensure consistent names
    target_compile_definitions(
        ${_target}
        PRIVATE "-DBOOST_TEST_DYN_LINK" "-DBOOST_TEST_MODULE=${_target}"
    )
    target_include_directories(${_target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(
        ${_target}
        PRIVATE
            ActsCore
            ActsTestsCommonHelpers
            Boost::program_options
            Boost::unit_test_framework
    )
endmacro()

add_benchmark(AtlasStepper AtlasStepperBenchmark.cpp)
add_benchmark(BoundaryTolerance BoundaryToleranceBenchmark.cpp)
add_benchmark(BinUtility BinUtilityBenchmark.cpp)
add_benchmark(EigenStepper EigenStepperBenchmark.cpp)
add_benchmark(SolenoidField SolenoidFieldBenchmark.cpp)
add_benchmark(SurfaceIntersection SurfaceIntersectionBenchmark.cpp)
add_benchmark(RayFrustum RayFrustumBenchmark.cpp)
add_benchmark(AnnulusBounds AnnulusBoundsBenchmark.cpp)
add_benchmark(StraightLineStepper StraightLineStepperBenchmark.cpp)
add_benchmark(SympyStepper SympyStepperBenchmark.cpp)
add_benchmark(Stepper StepperBenchmark.cpp)
add_benchmark(SourceLink SourceLinkBenchmark.cpp)
