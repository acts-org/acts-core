include(FetchContent)

add_library(
    ActsPluginDD4hep
    SHARED
    src/ConvertDD4hepDetector.cpp
    src/DD4hepBlueprintFactory.cpp
    src/DD4hepBinningHelpers.cpp
    src/DD4hepDetectorStructure.cpp
    src/DD4hepMaterialHelpers.cpp
    src/DD4hepDetectorElement.cpp
    src/DD4hepDetectorSurfaceFactory.cpp
    src/DD4hepLayerBuilder.cpp
    src/DD4hepLayerStructure.cpp
    src/DD4hepVolumeBuilder.cpp
    src/DD4hepFieldAdapter.cpp
)

target_include_directories(
    ActsPluginDD4hep
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(ActsPluginDD4hep PUBLIC ActsCore ActsPluginTGeo)

if(${DD4hep_VERSION} VERSION_LESS 1.11)
    target_include_directories(ActsPluginDD4hep PUBLIC ${DD4hep_INCLUDE_DIRS})
    target_link_libraries(ActsPluginDD4hep PUBLIC ${DD4hep_DDCORE_LIBRARY})
else()
    target_link_libraries(ActsPluginDD4hep PUBLIC DD4hep::DDCore DD4hep::DDRec)
endif()

install(
    TARGETS ActsPluginDD4hep
    EXPORT ActsPluginDD4hepTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY include/Acts DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
