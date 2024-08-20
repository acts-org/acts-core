add_library(
    ActsExamplesMuonSpectrometerMockupDetector
    SHARED
    src/MockupSectorBuilder.cpp
)
target_include_directories(
    ActsExamplesMuonSpectrometerMockupDetector
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(
    ActsExamplesMuonSpectrometerMockupDetector
    PUBLIC
        ActsCore
        ActsExamplesFramework
        ActsExamplesGeant4
        ActsExamplesDetectorGeant4
        ActsPluginGeant4
)

install(
    TARGETS ActsExamplesMuonSpectrometerMockupDetector
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
