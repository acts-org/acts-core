set(library_sources src/RootMaterialTrack.cpp)

add_library(ActsPluginRoot SHARED ${library_sources})

target_include_directories(
    ActsPluginRoot
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(
    ActsPluginRoot
    PUBLIC ActsCore ROOT::Core ROOT::RIO ROOT::ROOTDataFrame
)

install(
    TARGETS ActsPluginRoot
    EXPORT ActsPluginRootTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY include/Acts DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
