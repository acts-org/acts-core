add_library(
    ActsExamplesIoRoot
    SHARED
    src/RootMeasurementWriter.cpp
    src/RootMaterialDecorator.cpp
    src/RootMaterialWriter.cpp
    src/RootMaterialTrackReader.cpp
    src/RootMaterialTrackWriter.cpp
    src/RootParticleWriter.cpp
    src/RootParticleReader.cpp
    src/RootPropagationStepsWriter.cpp
    src/RootPropagationSummaryWriter.cpp
    src/RootSeedWriter.cpp
    src/RootSimHitWriter.cpp
    src/RootSimHitReader.cpp
    src/RootSpacepointWriter.cpp
    src/RootTrackParameterWriter.cpp
    src/RootTrackStatesWriter.cpp
    src/RootTrackSummaryReader.cpp
    src/RootTrackSummaryWriter.cpp
    src/RootBFieldWriter.cpp
    src/RootAthenaNTupleReader.cpp
    src/RootVertexReader.cpp
    src/RootVertexWriter.cpp
    src/RootAthenaDumpReader.cpp
    src/RootNuclearInteractionParametersWriter.cpp
    src/detail/NuclearInteractionParametrisation.cpp
    src/TrackFinderPerformanceWriter.cpp
    src/SeedingPerformanceWriter.cpp
    src/TrackFinderNTupleWriter.cpp
    src/TrackFitterPerformanceWriter.cpp
    src/VertexNTupleWriter.cpp
)
target_include_directories(
    ActsExamplesIoRoot
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(
    ActsExamplesIoRoot
    PUBLIC
        ActsCore
        ActsExamplesDigitization
        ActsExamplesFramework
        ActsExamplesPropagation
        ActsExamplesTruthTracking
        ActsExamplesMaterialMapping
        Threads::Threads
    PRIVATE ROOT::Core ROOT::Hist ROOT::Tree
)
target_compile_definitions(ActsExamplesIoRoot PUBLIC ACTS_EXAMPLES_WITH_ROOT)

root_generate_dictionary(
  ActsExamplesIoRootDict MODULE ActsExamplesIoRoot LINKDEF LinkDef.hpp
)
set_target_properties(ActsExamplesIoRootDict PROPERTIES CXX_CLANG_TIDY "")
target_compile_options(ActsExamplesIoRootDict PRIVATE "-Wno-old-style-cast")

install(TARGETS ActsExamplesIoRoot LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
