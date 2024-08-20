add_library(
    ActsExamplesIoNuclearInteractions
    SHARED
    src/RootNuclearInteractionParametersWriter.cpp
    src/detail/NuclearInteractionParametrisation.cpp
)
target_include_directories(
    ActsExamplesIoNuclearInteractions
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(
    ActsExamplesIoNuclearInteractions
    PUBLIC ActsCore ActsExamplesFramework Threads::Threads
    PRIVATE ROOT::Core ROOT::Hist ROOT::Tree
)

install(
    TARGETS ActsExamplesIoNuclearInteractions
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
