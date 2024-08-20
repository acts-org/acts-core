add_library(ActsPluginLegacy INTERFACE)
target_compile_features(ActsPluginLegacy INTERFACE cxx_std_17)
target_include_directories(
    ActsPluginLegacy
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(ActsPluginLegacy INTERFACE Boost::boost Eigen3::Eigen)

install(
    TARGETS ActsPluginLegacy
    EXPORT ActsPluginLegacyTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY include/Acts DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
