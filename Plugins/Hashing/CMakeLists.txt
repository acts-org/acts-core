add_library(ActsPluginHashing INTERFACE)
target_include_directories(
    ActsPluginHashing
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(ActsPluginHashing INTERFACE Annoy)

install(
    TARGETS ActsPluginHashing
    EXPORT ActsPluginHashingTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY include/Acts DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
