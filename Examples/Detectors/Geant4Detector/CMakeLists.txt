add_library(ActsExamplesDetectorGeant4 SHARED src/Geant4Detector.cpp)
target_include_directories(
    ActsExamplesDetectorGeant4
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(
    ActsExamplesDetectorGeant4
    PUBLIC ActsCore ActsExamplesFramework ActsExamplesGeant4 ActsPluginGeant4
)

install(
    TARGETS ActsExamplesDetectorGeant4
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
