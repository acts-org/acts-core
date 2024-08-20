add_library(
    ActsExamplesDetectorTelescope
    SHARED
    src/TelescopeDetector.cpp
    src/TelescopeDetectorElement.cpp
    src/BuildTelescopeDetector.cpp
)
target_include_directories(
    ActsExamplesDetectorTelescope
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(
    ActsExamplesDetectorTelescope
    PUBLIC ActsCore ActsExamplesFramework
)

install(
    TARGETS ActsExamplesDetectorTelescope
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
