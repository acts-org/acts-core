include(ActsTargetLinkLibrariesSystem)

set(ActsExamplesFramework_SOURCES)

add_library(
    ActsExamplesFramework
    SHARED
    src/EventData/Measurement.cpp
    src/EventData/MeasurementCalibration.cpp
    src/EventData/ScalingCalibrator.cpp
    src/EventData/SimParticle.cpp
    src/Framework/IAlgorithm.cpp
    src/Framework/SequenceElement.cpp
    src/Framework/WhiteBoard.cpp
    src/Framework/RandomNumbers.cpp
    src/Framework/Sequencer.cpp
    src/Framework/DataHandle.cpp
    src/Framework/BufferedReader.cpp
    src/Utilities/EventDataTransforms.cpp
    src/Utilities/Paths.cpp
    src/Utilities/Options.cpp
    src/Utilities/Helpers.cpp
    src/Validation/DuplicationPlotTool.cpp
    src/Validation/EffPlotTool.cpp
    src/Validation/FakeRatePlotTool.cpp
    src/Validation/ResPlotTool.cpp
    src/Validation/TrackClassification.cpp
    src/Validation/TrackSummaryPlotTool.cpp
)

target_include_directories(
    ActsExamplesFramework
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(
    ActsExamplesFramework
    PUBLIC
        ActsCore
        ActsFatras
        ActsPluginFpeMonitoring
        Boost::boost
        ROOT::Core
        ROOT::Hist
    PRIVATE std::filesystem
)
target_compile_definitions(
    ActsExamplesFramework
    PRIVATE BOOST_FILESYSTEM_NO_DEPRECATED
)

# newer DD4hep version require TBB and search internally for TBB in
# config-only mode. to avoid mismatches we explicitly search using
# config-only mode first to be sure that we find the same version.
find_package(TBB ${_acts_tbb_version} CONFIG)
if(NOT TBB_FOUND)
    # no version check possible when using the find module
    find_package(TBB ${_acts_tbb_version} MODULE REQUIRED)
endif()
target_link_libraries(ActsExamplesFramework PUBLIC TBB::tbb)

install(
    TARGETS ActsExamplesFramework
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/ActsExamples DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

add_subdirectory_if(ML ACTS_BUILD_PLUGIN_ONNX)
