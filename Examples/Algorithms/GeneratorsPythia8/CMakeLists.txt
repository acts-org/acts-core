add_library(
    ActsExamplesGeneratorsPythia8
    SHARED
    ActsExamples/Generators/Pythia8ProcessGenerator.cpp
)
target_include_directories(
    ActsExamplesGeneratorsPythia8
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
target_link_libraries(
    ActsExamplesGeneratorsPythia8
    PUBLIC ActsCore ActsExamplesFramework ActsExamplesGenerators
    PRIVATE Pythia8
)

install(
    TARGETS ActsExamplesGeneratorsPythia8
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
