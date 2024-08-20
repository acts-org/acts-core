add_library(
    ActsExamplesDetectorGeneric
    SHARED
    src/BuildGenericDetector.cpp
    src/GenericDetector.cpp
    src/GenericDetectorElement.cpp
)
target_include_directories(
    ActsExamplesDetectorGeneric
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(ActsExamplesDetectorGeneric PUBLIC ActsCore)
target_link_libraries(ActsExamplesDetectorGeneric PUBLIC ActsExamplesFramework)

install(
    TARGETS ActsExamplesDetectorGeneric
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
