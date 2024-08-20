add_library(ActsPluginEDM4hep SHARED src/EDM4hepUtil.cpp)

target_include_directories(
    ActsPluginEDM4hep
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(
    ActsPluginEDM4hep
    PUBLIC ActsCore ActsPluginPodio EDM4HEP::edm4hep
)

install(
    TARGETS ActsPluginEDM4hep
    EXPORT ActsPluginEDM4hepTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY include/Acts DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
