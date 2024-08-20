include(FetchContent)

add_library(
    ActsPluginGeant4
    SHARED
    src/Geant4Converters.cpp
    src/Geant4DetectorElement.cpp
    src/Geant4DetectorSurfaceFactory.cpp
    src/Geant4PhysicalVolumeSelectors.cpp
)
target_include_directories(
    ActsPluginGeant4
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_include_directories(
    ActsPluginGeant4
    SYSTEM
    PRIVATE ${Geant4_INCLUDE_DIRS}
)
target_link_libraries(ActsPluginGeant4 PUBLIC ActsCore ${Geant4_LIBRARIES})

install(
    TARGETS ActsPluginGeant4
    EXPORT ActsPluginGeant4Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY include/Acts DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
