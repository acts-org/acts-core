add_library(
    ActsExamplesVertexing
    SHARED
    src/AdaptiveMultiVertexFinderAlgorithm.cpp
    src/IterativeVertexFinderAlgorithm.cpp
    src/VertexFitterAlgorithm.cpp
    src/SingleSeedVertexFinderAlgorithm.cpp
    src/TruthVertexSeeder.cpp
)
target_include_directories(
    ActsExamplesVertexing
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(
    ActsExamplesVertexing
    PUBLIC ActsCore ActsExamplesFramework
    PRIVATE ActsExamplesTruthTracking
)

install(
    TARGETS ActsExamplesVertexing
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
