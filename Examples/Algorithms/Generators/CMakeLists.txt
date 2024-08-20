add_library(
    ActsExamplesGenerators
    SHARED
    ActsExamples/Generators/EventGenerator.cpp
    ActsExamples/Generators/ParametricParticleGenerator.cpp
)
target_include_directories(
    ActsExamplesGenerators
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
target_link_libraries(
    ActsExamplesGenerators
    PUBLIC ActsCore ActsExamplesFramework
)

install(
    TARGETS ActsExamplesGenerators
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
