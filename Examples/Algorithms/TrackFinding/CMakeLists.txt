add_library(
    ActsExamplesTrackFinding
    SHARED
    src/SeedingAlgorithm.cpp
    src/SeedingOrthogonalAlgorithm.cpp
    src/SpacePointMaker.cpp
    src/TrackFindingAlgorithm.cpp
    src/TrackFindingAlgorithmFunction.cpp
    src/HoughTransformSeeder.cpp
    src/TrackParamsEstimationAlgorithm.cpp
    src/MuonHoughSeeder.cpp
    src/GbtsSeedingAlgorithm.cpp
)

target_include_directories(
    ActsExamplesTrackFinding
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(
    ActsExamplesTrackFinding
    PUBLIC
        ActsCore
        ActsExamplesFramework
        ActsExamplesIoJson
        ActsExamplesMagneticField
    PRIVATE ROOT::Core ROOT::Geom ROOT::Graf ROOT::Hist ROOT::Gpad
)

# If Hashing examples are enabled, add them to the build
if(ACTS_BUILD_EXAMPLES_HASHING)
    target_sources(
        ActsExamplesTrackFinding
        PRIVATE src/SeedingAlgorithmHashing.cpp
    )

    target_link_libraries(ActsExamplesTrackFinding PUBLIC ActsPluginHashing)
endif()

install(
    TARGETS ActsExamplesTrackFinding
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
