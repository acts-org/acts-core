add_library(
    ActsExamplesAmbiguityResolution
    SHARED
    src/GreedyAmbiguityResolutionAlgorithm.cpp
    src/ScoreBasedAmbiguityResolutionAlgorithm.cpp
)

target_include_directories(
    ActsExamplesAmbiguityResolution
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(
    ActsExamplesAmbiguityResolution
    PUBLIC ActsCore ActsExamplesFramework ActsPluginJson
)

install(
    TARGETS ActsExamplesAmbiguityResolution
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
