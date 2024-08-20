add_library(
    ActsExamplesDetectorDD4hep
    SHARED
    src/DD4hepDetector.cpp
    src/DD4hepGeometryService.cpp
)

target_include_directories(
    ActsExamplesDetectorDD4hep
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(
    ActsExamplesDetectorDD4hep
    PUBLIC ActsCore ActsPluginDD4hep ActsExamplesFramework
)

if(${DD4hep_VERSION} VERSION_LESS 1.11)
    target_include_directories(
        ActsExamplesDetectorDD4hep
        PUBLIC ${DD4hep_INCLUDE_DIRS}
    )
    target_link_libraries(
        ActsExamplesDetectorDD4hep
        PUBLIC ${DD4hep_DDCORE_LIBRARY}
    )
else()
    target_link_libraries(ActsExamplesDetectorDD4hep PUBLIC DD4hep::DDCore)
endif()

# not sure why this needs to be set, but dd4hep fails to compile otherwise
set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(
    ActsExamplesDetectorDD4hep
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
dd4hep_set_version(ActsExamplesDetectorDD4hep MAJOR 1 MINOR 0 PATCH 0)
dd4hep_generate_rootmap(ActsExamplesDetectorDD4hep)

install(
    TARGETS ActsExamplesDetectorDD4hep
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
