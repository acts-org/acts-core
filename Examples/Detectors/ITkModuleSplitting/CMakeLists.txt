add_library(ActsExamplesITkModuleSplitting INTERFACE)
target_include_directories(
    ActsExamplesITkModuleSplitting
    INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

install(DIRECTORY include/ActsExamples DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
