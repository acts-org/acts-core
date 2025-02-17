add_library(ActsExamplesPropagation SHARED src/PropagationAlgorithm.cpp)

target_include_directories(
    ActsExamplesPropagation
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(
    ActsExamplesPropagation
    PUBLIC ActsCore ActsExamplesFramework
)

acts_compile_headers(ActsExamplesPropagation GLOB "include/**/*.hpp")

install(
    TARGETS ActsExamplesPropagation
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
