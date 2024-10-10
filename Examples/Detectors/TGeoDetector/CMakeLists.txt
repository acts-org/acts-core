add_library(
    ActsExamplesDetectorTGeo
    SHARED
    src/TGeoDetector.cpp
    src/TGeoITkModuleSplitter.cpp
)

target_include_directories(
    ActsExamplesDetectorTGeo
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(
    ActsExamplesDetectorTGeo
    PUBLIC
        ActsCore
        ActsPluginTGeo
        ActsPluginJson
        ActsExamplesFramework
        ActsExamplesDetectorGeneric
        ActsExamplesITkModuleSplitting
)

install(
    TARGETS ActsExamplesDetectorTGeo
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/ActsExamples DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
