add_library(
    ActsExamplesPropagation
    SHARED
    src/PropagationAlgorithm.cpp
    src/SimHitToSummaryConversion.cpp
)

target_include_directories(
    ActsExamplesPropagation
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(
    ActsExamplesPropagation
    PUBLIC ActsCore ActsExamplesFramework
)

install(
    TARGETS ActsExamplesPropagation
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
